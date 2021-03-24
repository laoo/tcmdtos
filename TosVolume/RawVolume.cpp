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

std::span<uint8_t const,512> RawVolume::readSector( uint32_t sector )
{
  OVERLAPPED overlapped = {};

  int64_t offset = sector * LOGICAL_SECTOR_SIZE;

  if ( offset >= mSectorOffset.QuadPart && offset < mSectorOffset.QuadPart + (int64_t)mSector.size() )
  {
    return std::span<uint8_t const, LOGICAL_SECTOR_SIZE>{ mSector.data() + ( offset - mSectorOffset.QuadPart ), LOGICAL_SECTOR_SIZE };
  }

  assert( mSector.size() >= LOGICAL_SECTOR_SIZE );

  mSectorOffset.QuadPart = offset & mOffsetMask;
  
  overlapped.Offset = mSectorOffset.LowPart;
  overlapped.OffsetHigh = mSectorOffset.HighPart;

  if ( !ReadFile( mHandle, mSector.data(), (DWORD)mSector.size(), NULL, &overlapped ) )
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

  return std::span<uint8_t const, LOGICAL_SECTOR_SIZE>{ mSector.data() + ( offset - mSectorOffset.QuadPart ), LOGICAL_SECTOR_SIZE };
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

  mSector.resize( LOGICAL_SECTOR_SIZE );
  mOffsetMask = ~( LOGICAL_SECTOR_SIZE - 1 );
  mSectorOffset.QuadPart = std::numeric_limits<LONGLONG>::max();
}

