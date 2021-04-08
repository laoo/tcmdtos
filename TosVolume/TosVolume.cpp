#include "pch.hpp"
#include "TosVolume.hpp"
#include "RawVolume.hpp"
#include "Ex.hpp"
#include "Dir.hpp"
#include "DirEntry.hpp"
#include "WriteTransaction.hpp"

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

std::span<std::shared_ptr<Partition>const> TosVolume::partitions() const
{
  return { mPartitions.data(), mPartitions.size() };
}

cppcoro::generator<std::shared_ptr<DirEntry>> TosVolume::find( char const * path ) const
{
  std::string_view sv{ path };

  auto backSlash = std::find( sv.cbegin(), sv.cend(), '\\' );

  if ( backSlash == sv.cend() )
    return {};

  auto p = findPartition( { path, (size_t)std::distance( sv.cbegin(), backSlash ) } );
  if ( !p )
    return {};

  return p->rootDir()->find( { &*(backSlash + 1), (size_t)std::distance( backSlash + 1, sv.end() ) } );
}

bool TosVolume::unlink( char const * path ) const
{
  std::string_view sv{ path };

  auto backSlash = std::find( sv.cbegin(), sv.cend(), '\\' );

  if ( backSlash == sv.cend() )
    return false;

  auto p = findPartition( { path, (size_t)std::distance( sv.cbegin(), backSlash ) } );
  if ( !p )
    return false;

  WriteTransaction trans{};

  auto dir = p->rootDir();
  for ( auto e : dir->find( { &*( backSlash + 1 ), (size_t)std::distance( backSlash + 1, sv.end() ) } ) )
  {
    e->unlink( trans );
  }

  trans.commit( *mRawVolume );
  return true;
}

std::shared_ptr<Partition> TosVolume::findPartition( std::string_view path ) const
{
  if ( path.size() < 2 )
    return {};

  int partitionNumber = stoi( std::string{ path.cbegin(), path.cbegin() + 2 } );

  if ( partitionNumber < 0 || partitionNumber >= mPartitions.size() )
    return {};

  return mPartitions[partitionNumber];
}

void TosVolume::parseRootSector()
{
  auto rootSector = mRawVolume->readSectors( 0, 1 );

  std::span< PInfo const, 4> infos{ reinterpret_cast<PInfo const *>( rootSector.data() + pinfoOffset ), 4 };

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
  mPartitions.push_back( std::make_shared<Partition>( (int)mPartitions.size(), partition, offset, mRawVolume ) );
}

void TosVolume::parseBGMPartition( PInfo const & partition, uint32_t offset )
{
  mPartitions.push_back( std::make_shared<Partition>( (int)mPartitions.size(), partition, offset, mRawVolume ) );
}

void TosVolume::parseXGMPartition( PInfo const & partition, uint32_t offset )
{
  for ( ;; )
  {
    auto extendedRootSector = mRawVolume->readSectors( offset, 1 );

    std::span< PInfo const, 4> infos{ reinterpret_cast<PInfo const *>( extendedRootSector.data() + pinfoOffset ), 4 };

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

