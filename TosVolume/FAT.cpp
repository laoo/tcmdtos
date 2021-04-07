#include "pch.hpp"
#include "FAT.hpp"
#include "RawVolume.hpp"
#include "Ex.hpp"
#include "Log.hpp"

FAT::FAT( uint32_t pos, uint32_t size, uint32_t clusterEnd, uint32_t clusterSize, uint32_t dataPos ) : mClusters{}, mPos{ pos }, mSize{ size }, mClusterEnd{ clusterEnd }, mClusterSize{ clusterSize }, mDataPos{ dataPos }
{
}

void FAT::load( RawVolume & volume )
{
  mClusters.resize( mSize * RawVolume::RAW_SECTOR_SIZE / sizeof( uint16_t ) );
  volume.readSectors( mPos, mSize, std::span<uint8_t>( reinterpret_cast<uint8_t *>( mClusters.data() ), mClusters.size() * sizeof( uint16_t ) ) );
}

uint32_t FAT::clusterSize() const
{
  return mClusterSize;
}

uint32_t FAT::dataPos() const
{
  return mDataPos;
}

cppcoro::generator<uint16_t> FAT::fileClusters( uint16_t cluster ) const
{
  uint16_t maxCluster = mClusters.size() > 4086 ? 0xfff0 : 0xff0;

  while ( cluster > 0 && cluster < maxCluster )
  {
    if ( cluster > mClusters.size() )
      throw Ex{} << "Cluster index " << cluster << " exceeds FAT size of " << mClusters.size();

    co_yield cluster;

    cluster = mClusters[cluster];
  }
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

//WriteTransaction FAT::freeClusters( uint16_t startCluster )
//{
//  uint16_t maxCluster = mClusters.size() > 4086 ? 0xfff0 : 0xff0;
//
//  Modifier mod;
//  std::vector<uint16_t> freed;
//
//  while ( startCluster > 0 && startCluster < maxCluster )
//  {
//    auto cluster = startCluster;
//
//    mod.add( cluster );
//
//    if ( cluster > mClusters.size() )
//      throw Ex{} << "Cluster index " << cluster << " exceeds FAT size of " << mClusters.size();
//
//    startCluster = 0;
//    std::swap( startCluster, mClusters[cluster] );
//  }
//
//  return mod.commit( *this );
//}
//
//void FAT::Modifier::add( uint16_t cluster )
//{
//  auto ub = std::upper_bound( mModified.begin(), mModified.end(), cluster );
//  mModified.insert( ub, cluster );
//}
//
//WriteTransaction FAT::Modifier::commit( FAT const& fat )
//{
//  WriteTransaction result;
//  for ( auto it = mModified.cbegin(); it != mModified.cend(); )
//  {
//    uint32_t firstSector = mModified.front() / ( RawVolume::RAW_SECTOR_SIZE / sizeof( uint16_t ) );
//    auto nextSectorIt = std::find_if_not( it, mModified.cend(), [=]( uint16_t c )
//    {
//      auto sector = c / ( RawVolume::RAW_SECTOR_SIZE / sizeof( uint16_t ) );
//      return firstSector == sector;
//    } );
//
//    uint32_t startOff = *it;
//    uint32_t endOff = *( nextSectorIt - 1 );
//    std::vector<uint8_t> data;
//    data.reserve( endOff - startOff + 1 );
//    std::copy( (uint8_t const *)&fat.mClusters[startOff], (uint8_t const *)&fat.mClusters[endOff + 1], std::back_inserter( data ) );
//    result.add( fat.mPos + firstSector, startOff % ( RawVolume::RAW_SECTOR_SIZE / sizeof( uint16_t ) ) * 2, std::move( data ) );
//
//    it = nextSectorIt;
//  }
//
//  return result;
//}
