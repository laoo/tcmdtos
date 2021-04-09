#include "pch.hpp"
#include "TosVolume.hpp"
#include "RawVolume.hpp"
#include "Ex.hpp"
#include "Dir.hpp"
#include "TOSDir.hpp"
#include "DirEntry.hpp"
#include "WriteTransaction.hpp"

namespace
{

bool createTOSDirPrototype( std::filesystem::path const & path, TOSDir & out )
{

  std::shared_ptr<void> handle{ CreateFile( path.generic_wstring().c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL ), &CloseHandle };

  if ( handle.get() == INVALID_HANDLE_VALUE )
  {
    int x = GetLastError();
    return false;
  }

  BY_HANDLE_FILE_INFORMATION fileInfo;

  if ( !GetFileInformationByHandle( handle.get(), &fileInfo ) )
    return false;

  if ( fileInfo.nFileSizeHigh > 0 )
    return false;

  out.setSizeInBytes( fileInfo.nFileSizeLow );

  auto strPath = path.string();
  char buf[MAX_PATH];
  GetShortPathNameA( strPath.c_str(), buf, MAX_PATH );
  std::filesystem::path shortPath{ buf };
  auto filename = shortPath.filename().string();

  out.setNameExt( std::string_view{ filename } );
  out.setDirectory( ( fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 );
  out.setHidden( ( fileInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) != 0 );
  out.setReadOnly( ( fileInfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY ) != 0 );
  out.setSystem( ( fileInfo.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ) != 0 );

  SYSTEMTIME sysTime;
  if ( !FileTimeToSystemTime( &fileInfo.ftLastWriteTime, &sysTime ) )
    return false;

  out.setYear( sysTime.wYear );
  out.setMonth( sysTime.wMonth );
  out.setDay( sysTime.wDay );
  out.setHour( sysTime.wHour );
  out.setMinute( sysTime.wMinute );
  out.setSecond( sysTime.wSecond );

  return true;
}

bool createTOSDirPrototypes( std::filesystem::path const & parentFolder, std::filesystem::path path, std::vector<TOSDir> & out )
{
  auto parent = path.parent_path();
  if ( parent.empty() )
    return false;

  if ( std::filesystem::equivalent( parentFolder, parent ) )
  {
    out.push_back( {} );
    return createTOSDirPrototype( path, out.back() );
  }
  else
  {
    if ( createTOSDirPrototypes( parentFolder, parent, out ) )
    {
      out.push_back( {} );
      return createTOSDirPrototype( path, out.back() );
    }
    else
    {
      return false;
    }
  }
}

}

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

cppcoro::generator<std::shared_ptr<DirEntry>> TosVolume::find( std::string_view fullPath ) const
{
  auto [p, right] = findPartition( fullPath );

  if ( p )
    return p->rootDir()->find( right );
  else
    return {};
}

bool TosVolume::add( std::string_view dstFolder, std::string_view srcFolder, std::string_view path )
{
  auto [p, right] = findPartition( dstFolder );

  if ( !p )
    return false;

  if ( path.empty() )
    return false;

  if ( path.back() == '\\' )
    path = { path.data(), path.size() - 1 };

  std::filesystem::path fullSrcPath{ srcFolder };
  fullSrcPath /= path;

  std::vector<TOSDir> dirs;

  if ( !createTOSDirPrototypes( std::filesystem::path{ srcFolder }, fullSrcPath, dirs ) )
    return false;

  if ( dirs.back().isDirectory() )
  {
    p->add( dstFolder, dirs, []() {
      return std::span<uint8_t const>{};
    } );
  }
  else
  {
    std::ifstream fin{ fullSrcPath, std::ios::binary };
    std::array<uint8_t, RawVolume::RAW_SECTOR_SIZE> buf;

    p->add( right, dirs, [&]()
    {
      if ( fin.good() )
      {
        fin.read( (char *)buf.data(), buf.size() );
        return std::span<uint8_t const>{ buf.data(), (size_t)fin.gcount() };
      }
      else
      {
        return std::span<uint8_t const>{};
      }
    } );
  }

  return true;
}

//bool TosVolume::mkdir( std::string_view fullPath )
//{
//  auto [p, right] = findPartition( fullPath );
//
//  if ( p )
//  {
//    WriteTransaction trans{};
//    auto dir = p->rootDir();
//    if ( dir->mkdirs( trans, right ) )
//    {
//      trans.commit( *mRawVolume );
//    }
//  }
//  else
//  {
//    return false;
//  }
//}

bool TosVolume::unlink( std::string_view fullPath ) const
{
  auto [p, right] = findPartition( fullPath );

  if ( p )
  {
    WriteTransaction trans{};

    auto dir = p->rootDir();
    for ( auto e : dir->find( right ) )
    {
      e->unlink( trans );
    }

    trans.commit( *mRawVolume );
    return true;
  }
  else
  {
    return false;
  }
}

std::pair<std::shared_ptr<Partition>, std::string_view> TosVolume::findPartition( std::string_view path ) const
{
  auto backSlash = std::find( path.cbegin(), path.cend(), '\\' );

  if ( backSlash == path.cend() )
    return {};

  std::string_view left{ path.data(), (size_t)std::distance( path.cbegin(), backSlash ) };
  std::string_view right{ &*( backSlash + 1 ), (size_t)std::distance( backSlash + 1, path.end() ) };

  if ( left.size() < 2 )
    return {};

  int partitionNumber = stoi( std::string{ left.cbegin(), left.cbegin() + 2 } );

  if ( partitionNumber < 0 || partitionNumber >= mPartitions.size() )
    return {};

  return { mPartitions[partitionNumber], right };
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

