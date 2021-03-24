#include "pch.hpp"
#include "RawVolume.hpp"
#include "Ex.hpp"
#include "Log.hpp"

cppcoro::generator<std::shared_ptr<RawVolume>> RawVolume::enumeratePhysicalVolumes()
{
  return cppcoro::generator<std::shared_ptr<RawVolume>>();
}

std::shared_ptr<RawVolume> RawVolume::openImageFile( std::filesystem::path const & path )
{
  try
  {
    return std::shared_ptr<RawVolume>( new RawVolume{ path } );
  }
  catch ( Ex const & ex )
  {
    L_ERROR << ex.what();
    return {};
  }
}

std::string RawVolume::readSectors( uint32_t sector, uint32_t count )
{
  LARGE_INTEGER sectorOffset;
  sectorOffset.QuadPart = sector * RAW_SECTOR_SIZE;

  OVERLAPPED overlapped = {};
  overlapped.Offset = sectorOffset.LowPart;
  overlapped.OffsetHigh = sectorOffset.HighPart;

  std::string result;
  result.resize( count * RAW_SECTOR_SIZE );


  if ( !ReadFile( mHandle, result.data(), (DWORD)result.size(), NULL, &overlapped ) )
  {
    auto err = GetLastError();
    if ( err != ERROR_IO_PENDING )
    {
      throw Ex{} << "Error initiating read from input file: " << err;
    }
  }

  DWORD bytesCount;

  if ( !GetOverlappedResult( mHandle, &overlapped, &bytesCount, TRUE ) )
  {
    throw Ex{} << "Error reading from input file: " << GetLastError();
  }

  return result;
}

RawVolume::RawVolume( wchar_t volume )
{
}

RawVolume::RawVolume( std::filesystem::path const & path )
{
  mHandle = CreateFile( path.generic_wstring().c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL );

  if ( mHandle == INVALID_HANDLE_VALUE )
  {
    throw Ex{} << "Error opening input file: " << GetLastError();
  }
}

