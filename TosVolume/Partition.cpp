#include "pch.hpp"
#include "Partition.hpp"
#include "RawVolume.hpp"
#include "Ex.hpp"
#include "Log.hpp"
#include "FAT.hpp"

bool PInfo::exists() const
{
  return ( statusAndName & 1 ) != 0;
}

PInfo::Type PInfo::type() const
{
  std::string_view name{ reinterpret_cast<char const *>( &statusAndName ) + 1, 3 };

  if ( name == "GEM" )
  {
    return Type::GEM;
  }
  else if ( name == "BGM" )
  {
    return Type::BGM;
  }
  else if ( name == "XGM" )
  {
    return Type::XGM;
  }
  else
  {
    return Type::UNKNOWN;
  }
}

uint32_t PInfo::partitionOffset() const
{
  return _byteswap_ulong( offset );
}

uint32_t PInfo::partitionSize() const
{
  return _byteswap_ulong( size );
}

Partition::Partition( PInfo const & partition, uint32_t offset, std::shared_ptr<RawVolume> rawVolume ) : mRawVolume{ std::move( rawVolume ) }, mFAT{}, mType{ partition.type() }
{
  assert( mRawVolume );

  mBootPos = partition.partitionOffset() + offset;
  auto bootSector = mRawVolume->readSectors( mBootPos, 1 );

  BPB const& bpb{ *reinterpret_cast<BPB const*>( bootSector.data() + 0x0b ) };

  if ( bpb.nfats < 1 )
    throw Ex{} << "No File Allocation Tables defined";

  mLogicalSectorSize = bpb.bps / RawVolume::RAW_SECTOR_SIZE; //in physical 
  mClusterSize = mLogicalSectorSize * bpb.spc;
  mFATSize = mLogicalSectorSize * bpb.spf;
  mDirSize = bpb.ndirs * sizeof( TOSDir ) / RawVolume::RAW_SECTOR_SIZE;

  mFATPos = mBootPos + bpb.res * mLogicalSectorSize;
  mDirPos = mFATPos + mFATSize * bpb.nfats;
  mDataPos = mDirPos + mDirSize - 2 * mClusterSize; //two entries in FAT are reserved
  mClusterEnd = ( bpb.nsects - ( mDataPos - mBootPos ) ) / mClusterSize;

  mFAT = std::make_shared<FAT>( mFATPos, mFATSize, mClusterEnd );
  mFAT->load( *mRawVolume );
}

std::string Partition::getLabel()
{
  for ( auto const & dir : rootDir()->listDir() )
  {
    if ( dir->isLabel() )
    {
      std::string label;
      dir->nameWithExt( std::back_inserter( label ) );
      return label;
    }
  }

  return {};
}

std::shared_ptr<DirectoryEntry> Partition::rootDir()
{
  return std::make_shared<DirectoryEntry>( shared_from_this() );
}

cppcoro::generator<std::shared_ptr<DirectoryEntry>> Partition::listDir( std::shared_ptr<DirectoryEntry const> parent )
{
  if ( auto startCluster = parent->getCluster() )
  {
    std::string clusterBuf;
    clusterBuf.resize( mClusterSize * RawVolume::RAW_SECTOR_SIZE );

    for ( uint16_t cluster : mFAT->fileClusters( startCluster ) )
    {
      uint32_t sector = mDataPos + mClusterSize * cluster;
      mRawVolume->readSectors( sector, mClusterSize, std::span<uint8_t>{ (uint8_t *)clusterBuf.data(), clusterBuf.size() } );

      size_t dirsInCluster = clusterBuf.size() / sizeof( TOSDir );

      for ( uint32_t i = 0; i < dirsInCluster; ++i )
      {
        TOSDir const * dir = reinterpret_cast<TOSDir const *>( clusterBuf.data() + i * sizeof( TOSDir ) );

        if ( dir->fname[0] == 0 )
          co_return;

        if ( dir->fname[0] != (char)0xe5 )
        {
          co_yield std::make_shared<DirectoryEntry>( shared_from_this(), *dir, sector, i * sizeof( TOSDir ), parent );
        }
      }
    }
  }
  else //root
  {
    auto rootDir = mRawVolume->readSectors( mDirPos, mDirSize );
    size_t dirs = mDirSize * RawVolume::RAW_SECTOR_SIZE / sizeof( TOSDir );
    
    for ( uint32_t i = 0; i < dirs; ++i )
    {
      TOSDir const* dir = reinterpret_cast<TOSDir const*>( rootDir.data() + i * sizeof( TOSDir ) );
    
      if ( dir->fname[0] == 0 )
        co_return;
    
      if ( dir->fname[0] != (char)0xe5 )
      {
        co_yield std::make_shared<DirectoryEntry>( shared_from_this(), *dir, mDirPos, i * sizeof( TOSDir ) );
      }
    }
  }
}

cppcoro::generator<std::span<char const>> Partition::read( std::shared_ptr<DirectoryEntry const> dir ) const
{
  if ( auto startCluster = dir->getCluster() )
  {
    auto size = dir->getSizeInBytes();

    std::string clusterBuf;
    clusterBuf.resize( mClusterSize * RawVolume::RAW_SECTOR_SIZE );

    uint32_t count{ 1 };
    for ( uint16_t cluster : mFAT->fileClusters( startCluster ) )
    {
      mRawVolume->readSectors( mDataPos + mClusterSize * cluster, mClusterSize, std::span<uint8_t>{ (uint8_t *)clusterBuf.data(), clusterBuf.size() } );

      co_yield std::span<char const>{ (char const *)clusterBuf.data(), ( count++ * clusterBuf.size() < size ) ? clusterBuf.size() : ( size % clusterBuf.size() ) };
    }
  }
  else
  {
    co_return;
  }
}

void Partition::unlink( std::shared_ptr<DirectoryEntry> dir )
{
  if ( auto startCluster = dir->getCluster() )
  {
    auto transaction = mFAT->freeClusters( startCluster );

    auto [sector, offset] = dir->getLocationInPartition();
    transaction.add( sector, offset, std::vector<uint8_t>( 1, 0xe5 ) );

    transaction.commit( *mRawVolume );
  }
}

PInfo::Type Partition::type() const
{
  return mType;
}
