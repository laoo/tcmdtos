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

Partition::Partition( PInfo const & partition, uint32_t offset, std::shared_ptr<RawVolume> rawVolume ) : mRawVolume{ std::move( rawVolume ) }
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
  mDataPos = mDirPos + mDirSize;

  mFAT = std::make_shared<FAT>( mFATPos, mFATSize );
  mFAT->load( *mRawVolume );
}

std::string Partition::getLabel() const
{
  for ( auto const & dir : rootDir() )
  {
    auto name = dir.getName();
    if ( dir.isLabel() )
    {
      auto name = dir.getName();
      auto ext = dir.getExt();
      std::string label{ name.data(), name.size() };
      if ( !ext.empty() )
      {
        label.append( "." );
        label.append( ext, 0, ext.size() );
      }

      return label;
    }
  }

  return {};
}

cppcoro::generator<DirectoryEntry> Partition::rootDir() const
{
  auto rootDir = mRawVolume->readSectors( mDirPos, mDirSize );
  size_t dirs = mDirSize * RawVolume::RAW_SECTOR_SIZE / sizeof( TOSDir );

  for ( size_t i = 0; i < dirs; ++i )
  {
    TOSDir const* dir = reinterpret_cast<TOSDir const*>( rootDir.data() + i * sizeof( TOSDir ) );

    if ( dir->fname[0] != 0 )
    {
      if ( dir->fname[0] != (char)0xe5 )
      {
        co_yield DirectoryEntry{ *dir };
      }
    }
    else
    {
      co_return;
    }
  }
}

