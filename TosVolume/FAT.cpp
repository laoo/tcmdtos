#include "pch.hpp"
#include "FAT.hpp"
#include "RawVolume.hpp"
#include "Ex.hpp"
#include "Log.hpp"

FAT::FAT( uint32_t pos, uint32_t size ) : mClusters{}, mPos{ pos }, mSize{ size }
{
}

void FAT::load( RawVolume & volume )
{
  mClusters.resize( mSize * RawVolume::RAW_SECTOR_SIZE / sizeof( uint16_t ) );
  volume.readSectors( mPos, mSize, std::span<uint8_t>( reinterpret_cast<uint8_t *>( mClusters.data() ), mClusters.size() * sizeof( uint16_t ) ) );
}

cppcoro::generator<uint16_t> FAT::fileClusters( uint16_t cluster ) const
{
  if ( mClusters.size() > 4086 )
  {
    while ( cluster > 0 && cluster < 0xfff0 )
    {
      if ( cluster > mClusters.size() )
        throw Ex{} << "Cluster index " << cluster << " exceeds FAT size of " << mClusters.size();

      co_yield cluster;

      cluster = mClusters[cluster];
    }
  }
  else
  {
    while ( cluster > 0 && cluster < 0xff0 )
    {
      if ( cluster > mClusters.size() )
        throw Ex{} << "Cluster index " << cluster << " exceeds FAT size of " << mClusters.size();

      co_yield cluster;

      cluster = mClusters[cluster];
    }
  }
}

cppcoro::generator<FAT::Range> FAT::fileClusterRanges( uint16_t cluster ) const
{
  Range r{};

  for ( uint16_t c : fileClusters( cluster ) )
  {
    if ( r.size > 0 )
    {
      [[likely]]
      if ( r.cluster + r.size == c )
      {
        [[likely]]
        r.size += 1;
      }
      else
      {
        L_DEBUG << "Fragmented file at cluster " << c;
        co_yield r;
        r.cluster = c;
        r.size = 1;
      }
    }
    else
    {
      r.cluster = c;
      r.size = 1;
    }
  }

  if ( r.size > 0 )
    co_yield r;
}
