#include "pch.hpp"
#include "Partition.hpp"
#include "RawVolume.hpp"
#include "Ex.hpp"

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
  mPosBoot = partition.partitionOffset() + offset;
  auto bootSector = mRawVolume->readSectors( mPosBoot, 1 );

  BPB const& bpb{ *reinterpret_cast<BPB const*>( bootSector.data() + 0x0b ) };

  if ( bpb.nfats < 1 )
    throw Ex{} << "No File Allocation Tables defined";

  mLogicalSectorSize = bpb.bps / RawVolume::RAW_SECTOR_SIZE; //in physical 
  mClusterSize = mLogicalSectorSize * bpb.spc;
  mFATSize = mLogicalSectorSize * bpb.spf;
  mDirSize = bpb.ndirs * sizeof( TOSDir ) / RawVolume::RAW_SECTOR_SIZE;

  mPosFat = mPosBoot + bpb.res * mLogicalSectorSize;
  mPosDir = mPosFat + mFATSize * bpb.nfats;
  mPosData = mPosDir + mDirSize;
}

