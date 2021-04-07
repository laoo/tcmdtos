#include "pch.hpp"
#include "File.hpp"
#include "RawVolume.hpp"
#include "DirEntry.hpp"
#include "FAT.hpp"

BaseFile::BaseFile( std::shared_ptr<RawVolume> rawVolume, std::shared_ptr<FAT> fat, std::shared_ptr<DirEntry> dirEntry ) : mRawVolume{ std::move( rawVolume ) }, mFAT{ std::move( fat ) }, mDirEntry{ std::move( dirEntry ) }
{
}

std::shared_ptr<DirEntry> BaseFile::dirEntry() const
{
  return mDirEntry;
}

char * BaseFile::fullPath( char * it ) const
{
  assert( mDirEntry );
  return mDirEntry->fullPath( it );
}

std::shared_ptr<RawVolume> BaseFile::rawVolume() const
{
  return mRawVolume;
}

std::shared_ptr<FAT> BaseFile::fat() const
{
  return mFAT;
}

RootDirFile::RootDirFile( std::shared_ptr<RawVolume> rawVolume, std::shared_ptr<FAT> fat, std::shared_ptr<DirEntry> dirEntry, uint32_t dirPos, uint32_t dirSize )
  : BaseFile{ std::move( rawVolume ), std::move( fat ), std::move( dirEntry ) }, mDirPos{ dirPos }, mDirSize{ dirSize }
{
}

cppcoro::generator<std::span<int8_t const>> RootDirFile::read() const
{
  auto rootDir = mRawVolume->readSectors( mDirPos, mDirSize );
  co_yield std::span<int8_t const>{ (int8_t const *)rootDir.data(), rootDir.size() };
}

File::File( std::shared_ptr<RawVolume> rawVolume, std::shared_ptr<FAT> fat, std::shared_ptr<DirEntry> dirEntry ) : BaseFile{ std::move( rawVolume ), std::move( fat ), std::move( dirEntry ) }
{
}

cppcoro::generator<std::span<int8_t const>> File::read() const
{
  auto startCluster = mDirEntry->getCluster();
  auto clusterSize = mFAT->clusterSize();
  auto dataPos = mFAT->dataPos();

  std::string clusterBuf;
  clusterBuf.resize( clusterSize * RawVolume::RAW_SECTOR_SIZE );

  auto size = mDirEntry->getSizeInBytes();

  if ( size == 0 && !mDirEntry->isDirectory() )
    co_return;

  uint32_t count{ 1 };
  for ( uint16_t cluster : mFAT->fileClusters( startCluster ) )
  {
    mRawVolume->readSectors( dataPos + clusterSize * cluster, clusterSize, std::span<uint8_t>{ (uint8_t *)clusterBuf.data(), clusterBuf.size() } );

    co_yield std::span<int8_t const>{ (int8_t const *)clusterBuf.data(), ( size == 0 || count++ * clusterBuf.size() < size ) ? clusterBuf.size() : ( size % clusterBuf.size() ) };
  }
}
