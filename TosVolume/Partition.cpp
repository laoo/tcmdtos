#include "pch.hpp"
#include "Partition.hpp"
#include "RawVolume.hpp"
#include "Ex.hpp"
#include "Log.hpp"
#include "FAT.hpp"
#include "File.hpp"
#include "DirEntry.hpp"
#include "Dir.hpp"

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

Partition::Partition( int number, PInfo const & partition, uint32_t offset, std::shared_ptr<RawVolume> rawVolume ) : mNumber{ number }, mRawVolume{ std::move( rawVolume ) }, mFAT{}, mType{ partition.type() }
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

  mFAT = std::make_shared<FAT>( mFATPos, mFATSize, mClusterEnd, mClusterSize, mDataPos );
  mFAT->load( *mRawVolume );
}

std::shared_ptr<Dir> Partition::rootDir()
{
  return std::make_shared<Dir>( rootDirFile() );
}

std::shared_ptr<BaseFile> Partition::rootDirFile() const
{
  auto pTOS = std::make_shared<TOSDir>();

  pTOS->fnameExt[0] = '0' + ( number() / 10 ) % 10;
  pTOS->fnameExt[1] = '0' + number() % 10;

  switch ( type() )
  {
  case PInfo::Type::GEM:
    std::copy_n( "GEM", 3, pTOS->fnameExt.begin() + 8 );
    break;
  case PInfo::Type::BGM:
    std::copy_n( "BGM", 3, pTOS->fnameExt.begin() + 8 );
    break;
  default:
    assert( false );
    break;
  }
  pTOS->attrib = DirEntry::ATTR_DIRECTORY;

  auto dirEntry = std::make_shared<DirEntry>( pTOS, std::shared_ptr<Dir>{} );

  return std::make_shared<RootDirFile>( mRawVolume, mFAT, dirEntry, mDirPos, mDirSize );
}


//bool Partition::unlink( std::shared_ptr<DirectoryEntry> dir, WriteTransaction * trans )
//{
//  WriteTransaction transaction{};
//
//  if ( dir->isDirectory() )
//  {
//    unlink( dir, &transaction );
//  }
//  if ( auto startCluster = dir->getCluster() )
//  {
//    transaction += mFAT->freeClusters( startCluster );
//    removeDirectoryEntry( dir, transaction );
//  }
//
//  if ( trans )
//  {
//    *trans += transaction;
//  }
//  else
//  {
//    transaction.commit( *mRawVolume );
//  }
//
//  return true;
//}
//
//void Partition::removeDirectoryEntry( std::shared_ptr<DirectoryEntry> dir, WriteTransaction & trans )
//{
//  auto [sector, offset] = dir->getLocationInPartition();
//  trans.add( sector, offset, std::vector<uint8_t>( 1, 0xe5 ) );
//}
//
//bool Partition::unlink( std::vector<std::shared_ptr<DirectoryEntry>> dirs )
//{
//  WriteTransaction transaction;
//
//  for ( auto dir : dirs )
//  {
//    unlink( dir, &transaction );
//  }
//
//  transaction.commit( *mRawVolume );
//  return true;
//}

PInfo::Type Partition::type() const
{
  return mType;
}

int Partition::number() const
{
  return mNumber;
}
