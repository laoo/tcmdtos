#include "pch.hpp"
#include "FAT.hpp"
#include "RawVolume.hpp"
#include "Ex.hpp"
#include "Log.hpp"

FAT::FAT( uint32_t pos, uint32_t size, uint32_t clusterEnd ) : mClusters{}, mPos{ pos }, mSize{ size }, mClusterEnd{ clusterEnd }
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

FAT::Range FAT::findFreeClusterRange( uint32_t requiredClusters ) const
{
  if ( requiredClusters == 0 )
    return {};

  auto it = mClusters.cbegin() + 2;
  auto end = mClusters.cbegin() + mClusterEnd - requiredClusters;
    
  while ( it < end )
  {
    it = std::find( it, end, 0 );
    if ( it == end )
      return {};
    auto not0 = std::find_if( it, it + requiredClusters, []( uint16_t c )
    {
      return c != 0;
    } );
    if ( not0 == it + requiredClusters )
      return { (uint16_t)std::distance( mClusters.cbegin(), it ), (uint16_t)requiredClusters };
    else
      it = not0 + 1;
  }

  return {};
}

std::vector<uint16_t> FAT::findFreeClusters( uint32_t requiredClusters ) const
{
  if ( requiredClusters == 0 )
    return {};

  std::vector<uint16_t> result;

  for ( uint16_t i = 2; i < mClusterEnd; ++i )
  {
    if ( mClusters[i] == 0 )
    {
      result.push_back( i );
      if ( result.size() == requiredClusters )
        return result;
    }
  }

  return {};
}
