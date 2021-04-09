#include "pch.hpp"
#include "TOSDir.hpp"
#include "Utils.hpp"

uint32_t TOSDir::getYear() const
{
  return ( ( fdate & MASK_YEAR ) >> 9 ) + 1980;
}

uint32_t TOSDir::getMonth() const
{
  return ( fdate & MASK_MONTH ) >> 5;
}

uint32_t TOSDir::getDay() const
{
  return ( fdate & MASK_DAY );
}

uint32_t TOSDir::getHour() const
{
  return ( ftime & MASK_HOUR ) >> 11;
}

uint32_t TOSDir::getMinute() const
{
  return ( ftime & MASK_MINUTE ) >> 5;
}

uint32_t TOSDir::getSecond() const
{
  return ( ftime & MASK_SECOND ) * 2;
}

uint16_t TOSDir::getCluster() const
{
  return scluster;
}

uint32_t TOSDir::getSizeInBytes() const
{
  return fsize;
}

std::string_view TOSDir::getName() const
{
  std::string_view sv{ fnameExt.data(), 8 };
  auto pos = sv.find_last_not_of( ' ' );
  if ( pos == std::string_view::npos )
    return sv;
  else
    return std::string_view{ fnameExt.data(), pos + 1 };
}

std::string_view TOSDir::getExt() const
{
  std::string_view sv{ fnameExt.data() + 8, 3 };
  auto pos = sv.find_last_not_of( ' ' );
  if ( pos == std::string_view::npos )
    return {};
  else
    return std::string_view{ fnameExt.data() + 8, pos + 1 };
}

std::string TOSDir::nameWithExt() const
{
  return std::string();
}

std::array<char, 11> const & TOSDir::getNameExtArray() const
{
  return fnameExt;
}

bool TOSDir::isReadOnly() const
{
  return ( attrib & ATTR_READ_ONLY ) != 0;
}

bool TOSDir::isHidden() const
{
  return ( attrib & ATTR_HIDDEN ) != 0;
}

bool TOSDir::isSystem() const
{
  return ( attrib & ATTR_SYSTEM ) != 0;
}

bool TOSDir::isLabel() const
{
  return ( attrib & ATTR_LABEL ) != 0;
}

bool TOSDir::isDirectory() const
{
  return ( attrib & ATTR_DIRECTORY ) != 0;
}

bool TOSDir::isNew() const
{
  return ( attrib & ATTR_NEW ) != 0;
}

void TOSDir::setNameExt( std::string_view src )
{
  extractNameExt( src, fnameExt );
}

void TOSDir::setYear( uint32_t value )
{
  fdate &= ~MASK_YEAR;
  fdate |=  MASK_YEAR & ( ( value - 1980 ) << 9 );
}

void TOSDir::setMonth( uint32_t value )
{
  fdate &= ~MASK_MONTH;
  fdate |=  MASK_MONTH & ( value << 5 );
}

void TOSDir::setDay( uint32_t value )
{
  fdate &= ~MASK_DAY;
  fdate |=  MASK_DAY & value;
}

void TOSDir::setHour( uint32_t value )
{
  ftime &= ~MASK_HOUR;
  ftime |=  MASK_HOUR & ( value << 11 );
}

void TOSDir::setMinute( uint32_t value )
{
  ftime &= ~MASK_MINUTE;
  ftime |=  MASK_MINUTE & ( value << 5 );
}

void TOSDir::setSecond( uint32_t value )
{
  ftime &= ~MASK_SECOND;
  ftime |=  MASK_SECOND & ( value >> 1 );
}

void TOSDir::setCluster( uint16_t value )
{
  scluster = value;
}

void TOSDir::setSizeInBytes( uint32_t value )
{
  fsize = value;
}

void TOSDir::setReadOnly( bool value )
{
  attrib &= ~ATTR_READ_ONLY;
  attrib |= value ? ATTR_READ_ONLY : 0;
}

void TOSDir::setHidden( bool value )
{
  attrib &= ~ATTR_HIDDEN;
  attrib |= value ? ATTR_HIDDEN : 0;
}

void TOSDir::setSystem( bool value )
{
  attrib &= ~ATTR_SYSTEM;
  attrib |= value ? ATTR_SYSTEM : 0;
}

void TOSDir::setLabel( bool value )
{
  attrib &= ~ATTR_LABEL;
  attrib |= value ? ATTR_LABEL : 0;
}

void TOSDir::setDirectory( bool value )
{
  attrib &= ~ATTR_DIRECTORY;
  attrib |= value ? ATTR_DIRECTORY : 0;
}

void TOSDir::setNew( bool value )
{
  attrib &= ~ATTR_NEW;
  attrib |= value ? ATTR_NEW : 0;
}
