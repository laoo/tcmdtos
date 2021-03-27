#include "pch.hpp"
#include "DirectoryEntry.hpp"
#include "Partition.hpp"

namespace
{
template<size_t SIZE>
static bool match( std::array<char, SIZE> const & left, std::array<char, SIZE> const & right )
{
  for ( size_t i = 0; i < SIZE; ++i )
  {
    if ( left[i] == '?' || right[i] == '?' )
      continue;
    if ( left[i] != right[i] )
      return false;
  }

  return true;
}

bool extractNameExt( std::string_view src, std::array<char, 11> & nameExt )
{
  std::fill( nameExt.begin(), nameExt.end(), ' ' );

  size_t pos = 0;
  for ( auto c : src )
  {
    if ( c == '.' )
    {
      nameExt[8] = nameExt[9] = nameExt[10] = ' ';
      pos = 8;
    }
    else if ( c == '*' )
    {
      while ( pos < nameExt.size() )
        nameExt[pos++] = '?';
    }
    else if ( pos < nameExt.size() )
    {
      nameExt[pos++] = c;
    }
  }

  return true;
}

bool extractNameExt( std::string_view src, std::array<char, 11> & nameExt, std::string_view & rest )
{
  auto backSlash = std::find( src.cbegin(), src.cend(), '\\' );

  if ( extractNameExt( std::string_view{ src.data(), (size_t)std::distance( src.cbegin(), backSlash ) }, nameExt ) )
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

}

DirectoryEntry::DirectoryEntry( std::shared_ptr<Partition> partition ) : mPartition{ std::move( partition ) }, mParent{}, mYear{}, mMonth{}, mDay{}, mHour{}, mMinute{}, mSecond{}, mSize{}, mCluster{}, mSector{}, mOffset{}, mAttrib{ ATTR_DIRECTORY }, mNameExt{}
{
  std::fill( mNameExt.begin(), mNameExt.end(), ' ' );

  int number = mPartition->number();

  mNameExt[0] = '0' + ( number / 10 ) % 10;
  mNameExt[1] = '0' + number % 10;

  switch ( mPartition->type() )
  {
  case PInfo::Type::GEM:
    std::copy_n( "GEM", 3, mNameExt.begin() + 8 );
    break;
  case PInfo::Type::BGM:
    std::copy_n( "BGM", 3, mNameExt.begin() + 8 );
    break;
  default:
    break;
  }
}

DirectoryEntry::DirectoryEntry( std::shared_ptr<Partition> partition, TOSDir const & dir, uint32_t sector, uint32_t offset, std::shared_ptr<DirectoryEntry const> parent ) :
  mPartition{ std::move( partition ) }, mParent{ std::move( parent ) }, mYear{}, mMonth{}, mDay{}, mHour{}, mMinute{}, mSecond{}, mSize{}, mCluster{}, mSector{ sector }, mOffset{ offset }, mAttrib{}, mNameExt{}
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
  std::copy( dir.fnameExt.cbegin(), dir.fnameExt.cend(), mNameExt.begin() );
  if ( mNameExt[0] == 0x05 )
    mNameExt[0] = (char)0xe5;
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
  std::string_view sv{ mNameExt.data(), 8 };
  auto pos = sv.find_last_not_of( ' ' );
  if ( pos == std::string_view::npos )
    return sv;
  else
    return std::string_view{ mNameExt.data(), pos + 1 };
}

std::string_view DirectoryEntry::getExt() const
{
  std::string_view sv{ mNameExt.data() + 8, 3 };
  auto pos = sv.find_last_not_of( ' ' );
  if ( pos == std::string_view::npos )
    return {};
  else
    return std::string_view{ mNameExt.data() + 8, pos + 1 };
}

std::vector<std::shared_ptr<DirectoryEntry>> DirectoryEntry::find( std::string_view namesv ) const
{
  std::array<char, 11> nameExt{};
  std::string_view rest;

  if ( extractNameExt( namesv, nameExt, rest ) == false )
    return {};

  std::vector<std::shared_ptr<DirectoryEntry>> result;

  for ( auto const& dir : listDir() )
  {
    if ( !rest.empty() && !dir->isDirectory() )
      continue;

    if ( !match( nameExt, dir->mNameExt ) )
      continue;

    if ( rest.empty() )
    {
      result.push_back( dir );
    }
    else
    {
      for ( auto r : dir->find( rest ) )
      {
        result.push_back( std::move( r ) );
      }
    }
  }

  return result;
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
  return mPartition->unlink( shared_from_this() );
}




