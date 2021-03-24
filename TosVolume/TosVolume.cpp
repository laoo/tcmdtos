#include "pch.hpp"
#include "TosVolume.hpp"
#include "RawVolume.hpp"
#include "Ex.hpp"

TosVolume::TosVolume( std::filesystem::path const & path ) : mRawVolume{ RawVolume::openImageFile( path ) }
{
  if ( !mRawVolume )
    throw Ex{} << "Error opening image file " << path.string();

  try
  {
    parseRootSector();
  }
  catch ( Ex const & ex )
  {
    throw Ex{} << "Error parsing image file " << path.string() << ": " << ex.what();
  }
}

TosVolume::TosVolume()
{
}

void TosVolume::parseRootSector()
{

  auto rootSector = mRawVolume->readSector( 0 );

  PInfo infos[4];

  std::memcpy( &infos, reinterpret_cast<PInfo const *>( rootSector.data() + pinfoOffset ), sizeof( PInfo ) * 4 );

  for ( auto const & info : infos )
  {
    if ( info.exists() )
    {
      switch ( info.type() )
      {
      case PInfo::Type::GEM:
        parseGEMPartition( info );
        break;
      case PInfo::Type::BGM:
        parseBGMPartition( info );
        break;
      case PInfo::Type::XGM:
        parseXGMPartition( info, info.partitionOffset() );
        break;
      default:
        throw Ex{};
      }
    }
  }

  if ( mPartitions.empty() )
    throw Ex{} << "No partitions defined";
}

void TosVolume::parseGEMPartition( PInfo const & partition, uint32_t offset )
{
  mPartitions.push_back( std::make_shared<Partition>( partition, offset, mRawVolume ) );
}

void TosVolume::parseBGMPartition( PInfo const & partition, uint32_t offset )
{
  mPartitions.push_back( std::make_shared<Partition>( partition, offset, mRawVolume ) );
}

void TosVolume::parseXGMPartition( PInfo const & partition, uint32_t offset )
{
  for ( ;; )
  {
    auto extendedRootSector = mRawVolume->readSector( offset );

    PInfo infos[4];

    std::memcpy( &infos, reinterpret_cast<PInfo const *>( extendedRootSector.data() + pinfoOffset ), sizeof( PInfo ) * 4 );

    auto it = std::find_if( std::begin( infos ), std::end( infos ), []( PInfo const & info )
    {
      return info.exists();
    } );

    if ( it == std::end( infos ) )
      return;

    switch ( it->type() )
    {
    case PInfo::Type::GEM:
      parseGEMPartition( *it, offset );
      break;
    case PInfo::Type::BGM:
      parseBGMPartition( *it, offset );
      break;
    case PInfo::Type::XGM:
      throw Ex{} << "Unexpected XGM partition";
      break;
    default:
      throw Ex{};
    }

    it += 1;

    if ( it == std::end( infos ) || !it->exists() )
      return;

    if ( it->type() == PInfo::Type::XGM )
    {
      offset += it->partitionOffset();
    }
    else
    {
      throw Ex{} << "XGM partition expected";
    }
  }
}

