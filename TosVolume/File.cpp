#include "pch.hpp"
#include "File.hpp"
#include "RawVolume.hpp"
#include "DirEntry.hpp"
#include "FAT.hpp"
#include "Ex.hpp"

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
  : BaseFile{ std::move( rawVolume ), std::move( fat ), std::move( dirEntry ) }, mCache{}, mOriginalCache{}, mDirPos{ dirPos }, mDirSize{ dirSize }
{
}

cppcoro::generator<std::span<int8_t const>> RootDirFile::read() const
{
  auto rootDir = mRawVolume->readSectors( mDirPos, mDirSize );
  co_yield std::span<int8_t const>{ (int8_t const *)rootDir.data(), rootDir.size() };
}

cppcoro::generator<SharedSpan<int8_t>> RootDirFile::readCached()
{
  if ( mCache )
  {
    co_yield mCache;
  }
  else
  {
    size_t clusterBufSize = mDirSize * RawVolume::RAW_SECTOR_SIZE;
    mCache = { std::make_shared<int8_t[]>( clusterBufSize ), (uint32_t)clusterBufSize };
    mOriginalCache = { std::make_shared<int8_t[]>( clusterBufSize ), 0 };

    mRawVolume->readSectors( mDirPos, mDirSize, std::span<uint8_t>{ (uint8_t *)mCache.data.get(), clusterBufSize } );
    co_yield mCache;
  }
}

void RootDirFile::beginTransaction()
{
  if ( !mOriginalCache )
  {
    std::copy_n( mCache.data.get(), mCache.size, mOriginalCache.data.get() );
    mOriginalCache.size = mCache.size;
  }
  else
  {
    throw Ex{} << "RootDirFile begin transaction error";
  }
}

void RootDirFile::commitTransaction( RawVolume & volume )
{
  if ( mOriginalCache )
  {
    for ( uint32_t i = 0; i < mDirSize; ++i )
    {
      uint32_t off = i * RawVolume::RAW_SECTOR_SIZE;
      int8_t const * orgBeg = mOriginalCache.data.get() + off;
      int8_t const * orgEnd = orgBeg + RawVolume::RAW_SECTOR_SIZE;
      int8_t const * traBeg = mCache.data.get() + off;

      if ( std::mismatch( orgBeg, orgEnd, traBeg ).first < orgEnd )
      {
        volume.writeSectors( mDirPos + i, 1, std::span<uint8_t const>{ (uint8_t const *)traBeg, RawVolume::RAW_SECTOR_SIZE } );
      }
    }
  }
  else
  {
    throw Ex{} << "RootDirFile commit transaction error";
  }
}

void RootDirFile::endTransaction()
{
  if ( mOriginalCache )
  {
    mOriginalCache.size = 0;
  }
  else
  {
    throw Ex{} << "RootDirFile end transaction error";
  }
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

cppcoro::generator<SharedSpan<int8_t>> File::readCached()
{
  if ( !mCache.empty() )
  {
    for ( auto & ss : mCache )
    {
      co_yield ss;
    }
  }
  else
  {
    auto startCluster = mDirEntry->getCluster();
    auto clusterSize = mFAT->clusterSize();
    auto dataPos = mFAT->dataPos();

    size_t clusterBufSize = clusterSize * RawVolume::RAW_SECTOR_SIZE;

    auto size = mDirEntry->getSizeInBytes();

    if ( size == 0 && !mDirEntry->isDirectory() )
      co_return;

    uint32_t count{ 1 };
    for ( uint16_t cluster : mFAT->fileClusters( startCluster ) )
    {
      mCache.push_back( { std::make_shared<int8_t[]>( clusterBufSize ), (uint32_t)( ( size == 0 || count++ * clusterBufSize < size ) ? clusterBufSize : ( size % clusterBufSize ) ) } );
      mOriginalCache.push_back( { std::make_shared<int8_t[]>( clusterBufSize ), 0 } );

      mRawVolume->readSectors( dataPos + clusterSize * cluster, clusterSize, std::span<uint8_t>{ (uint8_t *)mCache.back().data.get(), clusterBufSize } );
      co_yield mCache.back();
    }
  }
}

void File::beginTransaction()
{
  if ( !mOriginalCache.empty() && !mOriginalCache.front() )
  {
    for ( size_t i = 0; i < mCache.size(); ++i )
    {
      std::copy_n( mCache[i].data.get(), mCache[i].size, mOriginalCache[i].data.get() );
      mOriginalCache[i].size = mCache[i].size;
    }
  }
  else
  {
    throw Ex{} << "File begin transaction error";
  }
}

void File::commitTransaction( RawVolume & volume )
{
  if ( !mOriginalCache.empty() && mOriginalCache.front() )
  {
    auto startCluster = mDirEntry->getCluster();
    auto clusterSize = mFAT->clusterSize();
    auto dataPos = mFAT->dataPos();

    size_t clusterBufSize = clusterSize * RawVolume::RAW_SECTOR_SIZE;
    uint32_t i{ 0 };
    for ( uint16_t cluster : mFAT->fileClusters( startCluster ) )
    {
      int8_t const * orgBeg = mOriginalCache[i].data.get();
      int8_t const * orgEnd = mOriginalCache[i].data.get() + mOriginalCache[i].size;
      int8_t const * traBeg = mCache[i].data.get();

      if ( std::mismatch( orgBeg, orgEnd, traBeg ).first < orgEnd )
      {
        mRawVolume->writeSectors( dataPos + clusterSize * cluster, clusterSize, std::span<uint8_t const>{ (uint8_t const *)traBeg, clusterBufSize } );
      }
    }
  }
  else
  {
    throw Ex{} << "RootDirFile commit transaction error";
  }
}

void File::endTransaction()
{
  if ( !mOriginalCache.empty() && mOriginalCache.front() )
  {
    for ( auto & ss : mOriginalCache )
    {
      ss.size = 0;
    }
  }
  else
  {
    throw Ex{} << "File end transaction error";
  }
}
