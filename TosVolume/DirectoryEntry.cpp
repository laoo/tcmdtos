#include "pch.hpp"
#include "DirectoryEntry.hpp"
#include "Partition.hpp"

DirectoryEntry::DirectoryEntry( std::shared_ptr<Partition> partition ) : mPartition{ std::move( partition ) }, mParent{}, mYear{}, mMonth{}, mDay{}, mHour{}, mMinute{}, mSecond{}, mSize{}, mCluster{}, mAttrib{}, mName{}, mExt{ ATTR_DIRECTORY }
{
  std::fill( mName.begin(), mName.end(), ' ' );
  std::fill( mExt.begin(), mExt.end(), ' ' );
}

DirectoryEntry::DirectoryEntry( std::shared_ptr<Partition> partition, TOSDir const & dir, uint32_t sector, uint32_t offset, std::shared_ptr<DirectoryEntry const> parent ) :
  mPartition{ std::move( partition ) }, mParent{ std::move( parent ) }, mYear{}, mMonth{}, mDay{}, mHour{}, mMinute{}, mSecond{}, mSize{}, mCluster{}, mSector{ sector }, mOffset{ offset }, mAttrib{}, mName{}, mExt{}
{
  mSize = dir.fsize;
  mCluster = dir.scluster;
  mYear =  ( ( dir.fdate & 0b1111111000000000 ) >> 9 ) + 1980;
  mMonth =   ( dir.fdate & 0b0000000111100000 ) >> 5;
  mDay =     ( dir.fdate & 0b0000000000011111 );
  mHour    = ( dir.ftime & 0b1111100000000000 ) >> 11;
  mMinute =  ( dir.ftime & 0b0000011111100000 ) >> 5;
  mSecond =  ( dir.ftime & 0b0000000000011111 );
  mAttrib = dir.attrib;
  std::copy( dir.fname.cbegin(), dir.fname.cend(), mName.begin() );
  std::copy( dir.fext.cbegin(), dir.fext.cend(), mExt.begin() );
  if ( mName[0] == 0x05 )
    mName[0] = (char)0xe5;
}

uint32_t DirectoryEntry::getYear() const
{
  return mYear;
}

uint32_t DirectoryEntry::getMonth() const
{
  return mMonth;
}

uint32_t DirectoryEntry::getDay() const
{
  return mDay;
}

uint32_t DirectoryEntry::getHour() const
{
  return mHour;
}

uint32_t DirectoryEntry::getMinute() const
{
  return mMinute;
}

uint32_t DirectoryEntry::getSecond() const
{
  return mSecond * 2;
}

uint16_t DirectoryEntry::getCluster() const
{
  return mCluster;
}

uint32_t DirectoryEntry::getSizeInBytes() const
{
  return mSize;
}

bool DirectoryEntry::isReadOnly() const
{
  return ( mAttrib & ATTR_READ_ONLY ) != 0;
}

bool DirectoryEntry::isHidden() const
{
  return ( mAttrib & ATTR_HIDDEN ) != 0;
}

bool DirectoryEntry::isSystem() const
{
  return ( mAttrib & ATTR_SYSTEM ) != 0;
}

bool DirectoryEntry::isLabel() const
{
  return ( mAttrib & ATTR_LABEL ) != 0;
}

bool DirectoryEntry::isDirectory() const
{
  return ( mAttrib & ATTR_DIRECTORY ) != 0;
}

bool DirectoryEntry::isNew() const
{
  return ( mAttrib & ATTR_NEW ) != 0;
}

std::string_view DirectoryEntry::getName() const
{
  std::string_view sv{ mName.data(), mName.size() };
  auto pos = sv.find_last_not_of( ' ' );
  if ( pos == std::string_view::npos )
    return sv;
  else
    return std::string_view{ mName.data(), pos + 1 };
}

std::string_view DirectoryEntry::getExt() const
{
  std::string_view sv{ mExt.data(), mExt.size() };
  auto pos = sv.find_last_not_of( ' ' );
  if ( pos == std::string_view::npos )
    return {};
  else
    return std::string_view{ mExt.data(), pos + 1 };
}

std::shared_ptr<DirectoryEntry> DirectoryEntry::find( std::string_view namesv ) const
{
  std::array<char, 8> name{};
  std::array<char, 3> ext{};
  std::string_view rest;

  if ( extractNameExt( namesv, name, ext, rest ) == false )
    return {};

  for ( auto const& dir : listDir() )
  {
    if ( std::mismatch( name.cbegin(), name.cend(), dir->mName.cbegin() ).first != name.cend() )
      continue;
    if ( std::mismatch( ext.cbegin(), ext.cend(), dir->mExt.cbegin() ).first != ext.cend() )
      continue;

    if ( rest.empty() )
      return dir;
    else
      return dir->find( rest );
  }

  return {};
}

cppcoro::generator<std::shared_ptr<DirectoryEntry>> DirectoryEntry::listDir() const
{
  return mPartition->listDir( shared_from_this() );
}

cppcoro::generator<std::span<char const>> DirectoryEntry::read() const
{
  return mPartition->read( shared_from_this() );
}

std::pair<uint32_t, uint32_t> DirectoryEntry::getLocationInPartition() const
{
  return { mSector, mOffset };
}

bool DirectoryEntry::unlink()
{
  if ( isDirectory() )
    return false;

  mPartition->unlink( shared_from_this() );
  return true;
}

bool DirectoryEntry::extractNameExt( std::string_view src, std::array<char, 8> & name, std::array<char, 3> & ext, std::string_view & rest )
{
  auto backSlash = std::find( src.cbegin(), src.cend(), '\\' );

  if ( extractNameExt( std::string_view{ src.data(), (size_t)std::distance( src.cbegin(), backSlash ) }, name, ext ) )
  {
    if ( backSlash == src.cend() )
      rest = {};
    else
    {
      rest = std::string_view{ &*backSlash + 1, (size_t)std::distance( backSlash + 1, src.cend() ) };
    }

    return true;
  }
  else
  {
    return false;
  }
}


bool DirectoryEntry::extractNameExt( std::string_view src, std::array<char, 8> & name, std::array<char, 3> & ext )
{
  auto dotIt = std::find( src.cbegin(), src.cend(), '.' );
  auto nameSize = std::distance( src.cbegin(), dotIt );
  if ( nameSize > 8 )
    return false;

  if ( dotIt == src.cend() )
  {
    std::fill( name.begin(), name.end(), ' ' );
    std::fill( ext.begin(), ext.end(), ' ' );
    std::copy( src.cbegin(), dotIt, name.begin() );
  }
  else
  {
    auto extSize = std::distance( dotIt + 1, src.cend() );
    if ( extSize > 3 )
      return false;

    std::fill( name.begin(), name.end(), ' ' );
    std::fill( ext.begin(), ext.end(), ' ' );

    std::copy( src.cbegin(), dotIt, name.begin() );
    std::copy( dotIt + 1, src.cend(), ext.begin() );
  }

  return true;
}


