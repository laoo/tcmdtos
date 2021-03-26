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
  std::string result;
  result.resize( count * RAW_SECTOR_SIZE );

  readSectors( sector, count, std::span<uint8_t>{ (uint8_t*)result.data(), result.size() } );

  return result;
}

void RawVolume::readSectors( uint32_t sector, uint32_t count, std::span<uint8_t> destination )
{
  LARGE_INTEGER sectorOffset;
  sectorOffset.QuadPart = sector * RAW_SECTOR_SIZE;

  OVERLAPPED overlapped = {};
  overlapped.Offset = sectorOffset.LowPart;
  overlapped.OffsetHigh = sectorOffset.HighPart;

  if ( destination.size() != count * RAW_SECTOR_SIZE )
    throw Ex{};

  if ( !ReadFile( mHandle, destination.data(), (DWORD)destination.size(), NULL, &overlapped ) )
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
}

void RawVolume::writeSectors( uint32_t sector, uint32_t count, std::span<uint8_t const> source )
{
  LARGE_INTEGER sectorOffset;
  sectorOffset.QuadPart = sector * RAW_SECTOR_SIZE;

  OVERLAPPED overlapped = {};
  overlapped.Offset = sectorOffset.LowPart;
  overlapped.OffsetHigh = sectorOffset.HighPart;

  if ( source.size() != count * RAW_SECTOR_SIZE )
    throw Ex{};

  if ( !WriteFile( mHandle, source.data(), (DWORD)source.size(), NULL, &overlapped ) )
  {
    auto err = GetLastError();
    if ( err != ERROR_IO_PENDING )
    {
      throw Ex{} << "Error initiating write to file: " << err;
    }
  }

  DWORD bytesCount;

  if ( !GetOverlappedResult( mHandle, &overlapped, &bytesCount, TRUE ) )
  {
    throw Ex{} << "Error writing to file: " << GetLastError();
  }
}

void RawVolume::writeFragment( uint32_t sector, uint32_t offset, std::span<uint8_t const> data )
{
  LARGE_INTEGER sectorOffset;
  sectorOffset.QuadPart = sector * RAW_SECTOR_SIZE + offset;

  OVERLAPPED overlapped = {};
  overlapped.Offset = sectorOffset.LowPart;
  overlapped.OffsetHigh = sectorOffset.HighPart;

  if ( !WriteFile( mHandle, data.data(), (DWORD)data.size(), NULL, &overlapped ) )
  {
    auto err = GetLastError();
    if ( err != ERROR_IO_PENDING )
    {
      throw Ex{} << "Error initiating write to file: " << err;
    }
  }

  DWORD bytesCount;

  if ( !GetOverlappedResult( mHandle, &overlapped, &bytesCount, TRUE ) )
  {
    throw Ex{} << "Error writing to file: " << GetLastError();
  }
}

RawVolume::RawVolume( wchar_t volume )
{
}

RawVolume::RawVolume( std::filesystem::path const & path )
{
  mHandle = CreateFile( path.generic_wstring().c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL );

  if ( mHandle == INVALID_HANDLE_VALUE )
  {
    throw Ex{} << "Error opening input/output file: " << GetLastError();
  }
}

RawVolume::~RawVolume()
{
  CloseHandle( mHandle );
}

