#include "pch.hpp"
#include "DirEntry.hpp"
#include "Dir.hpp"
#include "TOSDir.hpp"
#include "File.hpp"
#include "FAT.hpp"
#include "Ex.hpp"
#include "WriteTransaction.hpp"

DirEntry::DirEntry( std::shared_ptr<TOSDir> tos, std::shared_ptr<Dir> dir ) : mTOS{ std::move( tos ) }, mDir{ std::move( dir ) }
{
  if ( !mTOS )
    throw Ex{} << "Empty TOS directory entry";
}

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

void DirEntry::unlink( WriteTransaction & transaction )
{
  if ( isDirectory() )
  {
    auto dir = openDir();
    for ( auto e : dir->list() )
    {
      e->unlink( transaction );
    }
  }

  auto fat = mDir->baseFile()->fat();
  fat->freeClusters( transaction, mTOS->scluster );
  transaction.transaction( mDir->baseFile() );
  mTOS->fnameExt[0] = (char)0xe5;
}

uint32_t DirEntry::getYear() const
{
  return mTOS->getYear();
}

uint32_t DirEntry::getMonth() const
{
  return mTOS->getMonth();
}

uint32_t DirEntry::getDay() const
{
  return mTOS->getDay();
}

uint32_t DirEntry::getHour() const
{
  return mTOS->getHour();
}

uint32_t DirEntry::getMinute() const
{
  return mTOS->getMinute();
}

uint32_t DirEntry::getSecond() const
{
  return mTOS->getSecond();
}

uint16_t DirEntry::getCluster() const
{
  return mTOS->getCluster();
}

uint32_t DirEntry::getSizeInBytes() const
{
  return mTOS->getSizeInBytes();
}

std::string_view DirEntry::getName() const
{
  return mTOS->getName();
}

std::string_view DirEntry::getExt() const
{
  return mTOS->getExt();
}

std::string DirEntry::nameWithExt() const
{
  std::string result;
  auto name = getName();
  auto ext = getExt();

  result.append( name );
  if ( !ext.empty() )
  {
    result.append( "." );
    result.append( ext );
  }

  return result;
}

std::array<char, 11> const& DirEntry::getNameExtArray() const
{
  return mTOS->getNameExtArray();
}

bool DirEntry::isReadOnly() const
{
  return mTOS->isReadOnly();
}

bool DirEntry::isHidden() const
{
  return mTOS->isHidden();
}

bool DirEntry::isSystem() const
{
  return mTOS->isSystem();
}

bool DirEntry::isLabel() const
{
  return mTOS->isLabel();
}

bool DirEntry::isDirectory() const
{
  return mTOS->isDirectory();
}

bool DirEntry::isNew() const
{
  return mTOS->isNew();
}

