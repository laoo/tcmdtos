#include "pch.hpp"
#include "DirEntry.hpp"
#include "Dir.hpp"
#include "File.hpp"
#include "Ex.hpp"

char * DirEntry::nameWithExt( char * it ) const
{
  for ( auto c : getName() )
  {
    *it++ = c;
  }
  auto ext = getExt();
  if ( !ext.empty() )
  {
    *it++ = '.';
    for ( auto c : ext )
    {
      *it++ = c;
    }
  }
  return it;
}

char * DirEntry::fullPath( char * it ) const
{
  if ( mDir )
  {
    it = mDir->fullPath( it );
  }
  it = nameWithExt( it );
  if ( isDirectory() )
    *it++ = '\\';
  return it;
}

std::shared_ptr<Dir> DirEntry::openDir()
{
  if ( isDirectory() )
    return std::make_shared<Dir>( openFile() );
  else return {};
}

std::shared_ptr<BaseFile> DirEntry::openFile()
{
  if ( mDir )
    return std::make_shared<File>( mDir->baseFile()->rawVolume(), mDir->baseFile()->fat(), shared_from_this() );
  else
    return {};
}

DirEntry::DirEntry( std::shared_ptr<TOSDir> tos, std::shared_ptr<Dir> dir ) : mTOS{ std::move( tos ) }, mDir{ std::move( dir ) }
{
  if ( !mTOS )
    throw Ex{} << "Empty TOS directory entry";
}

uint32_t DirEntry::getYear() const
{
  return ( ( mTOS->fdate & 0b1111111000000000 ) >> 9 ) + 1980;
}

uint32_t DirEntry::getMonth() const
{
  return ( mTOS->fdate & 0b0000000111100000 ) >> 5;
}

uint32_t DirEntry::getDay() const
{
  return ( mTOS->fdate & 0b0000000000011111 );
}

uint32_t DirEntry::getHour() const
{
  return ( mTOS->ftime & 0b1111100000000000 ) >> 11;
}

uint32_t DirEntry::getMinute() const
{
  return ( mTOS->ftime & 0b0000011111100000 ) >> 5;
}

uint32_t DirEntry::getSecond() const
{
  return ( mTOS->ftime & 0b0000000000011111 ) * 2;
}

uint16_t DirEntry::getCluster() const
{
  return mTOS->scluster;
}

uint32_t DirEntry::getSizeInBytes() const
{
  return mTOS->fsize;
}

std::string_view DirEntry::getName() const
{
  std::string_view sv{ mTOS->fnameExt.data(), 8 };
  auto pos = sv.find_last_not_of( ' ' );
  if ( pos == std::string_view::npos )
    return sv;
  else
    return std::string_view{ mTOS->fnameExt.data(), pos + 1 };
}

std::string_view DirEntry::getExt() const
{
  std::string_view sv{ mTOS->fnameExt.data() + 8, 3 };
  auto pos = sv.find_last_not_of( ' ' );
  if ( pos == std::string_view::npos )
    return {};
  else
    return std::string_view{ mTOS->fnameExt.data() + 8, pos + 1 };
}

std::array<char, 11> const& DirEntry::getNameExtArray() const
{
  return mTOS->fnameExt;
}

bool DirEntry::isReadOnly() const
{
  return ( mTOS->attrib & ATTR_READ_ONLY ) != 0;
}

bool DirEntry::isHidden() const
{
  return ( mTOS->attrib & ATTR_HIDDEN ) != 0;
}

bool DirEntry::isSystem() const
{
  return ( mTOS->attrib & ATTR_SYSTEM ) != 0;
}

bool DirEntry::isLabel() const
{
  return ( mTOS->attrib & ATTR_LABEL ) != 0;
}

bool DirEntry::isDirectory() const
{
  return ( mTOS->attrib & ATTR_DIRECTORY ) != 0;
}

bool DirEntry::isNew() const
{
  return ( mTOS->attrib & ATTR_NEW ) != 0;
}
