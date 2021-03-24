#include "pch.hpp"
#include "DirectoryEntry.hpp"
#include "Partition.hpp"

DirectoryEntry::DirectoryEntry( TOSDir const & dir )
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
    mName[0] = 0xe5;
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
  return mSecond;
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
    return sv;
  else
    return std::string_view{ mExt.data(), pos + 1 };
}


