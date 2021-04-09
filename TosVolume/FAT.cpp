#include "pch.hpp"
#include "FAT.hpp"
#include "RawVolume.hpp"
#include "Ex.hpp"
#include "Log.hpp"
#include "WriteTransaction.hpp"

FAT::FAT( uint32_t pos, uint32_t size, uint32_t clusterEnd, uint32_t clusterSize, uint32_t dataPos ) : mClusters{}, mOriginalClusters{}, mPos{ pos }, mSize{ size }, mClusterEnd{ clusterEnd }, mClusterSize{ clusterSize }, mDataPos{ dataPos }
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
  uint16_t maxCluster = maxClusterValue();

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

uint16_t FAT::findFirstFreeCluster() const
{
  for ( uint16_t i = 2; i < mClusterEnd; ++i )
  {
    if ( mClusters[i] == 0 )
    {
      return i;
    }
  }

  return 0;
}

void FAT::freeClusters( WriteTransaction & trans, uint16_t startCluster )
{
  trans.transaction( shared_from_this() );

  uint16_t maxCluster = maxClusterValue();

  while ( startCluster > 0 && startCluster < maxCluster )
  {
    auto cluster = startCluster;
  
    if ( cluster > mClusters.size() )
      throw Ex{} << "Cluster index " << cluster << " exceeds FAT size of " << mClusters.size();
  
    startCluster = 0;
    std::swap( startCluster, mClusters[cluster] );
  }
}

bool FAT::appendCluster( WriteTransaction & trans, uint16_t startCluster )
{
  trans.transaction( shared_from_this() );

  if ( auto lastCluster = findLastCluster( startCluster ) )
  {
    if ( auto freeCluster = findFirstFreeCluster() )
    {
      mClusters[freeCluster] = lastClusterValue();
      mClusters[lastCluster] = freeCluster;
      return true;
    }
  }

  return false;
}

void FAT::beginTransaction()
{
  if ( mOriginalClusters.empty() )
  {
    mOriginalClusters.reserve( mClusters.size() );
    std::copy( mClusters.begin(), mClusters.end(), std::back_inserter( mOriginalClusters ) );
  }
  else
  {
    throw Ex{} << "FAT begin transaction error";
  }
}

void FAT::commitTransaction( RawVolume & volume )
{
  if ( mOriginalClusters.empty() )
  {
    throw Ex{} << "FAT commit transaction error";
  }
  else
  {
    for ( uint32_t i = 0; i < mSize; ++i )
    {
      static constexpr uint32_t clusterIndicesPerSector = RawVolume::RAW_SECTOR_SIZE / sizeof( uint16_t );
      uint32_t off = i * clusterIndicesPerSector;
      uint16_t const* orgBeg = mOriginalClusters.data() + off;
      uint16_t const* orgEnd = orgBeg + clusterIndicesPerSector;
      uint16_t const* traBeg = mClusters.data() + off;

      if ( std::mismatch( orgBeg, orgEnd, traBeg ).first < orgEnd )
      {
        volume.writeSectors( mPos + i, 1, std::span<uint8_t const>{ (uint8_t const*)traBeg, RawVolume::RAW_SECTOR_SIZE } );
      }
    }
  }
}

void FAT::endTransaction()
{
  if ( mOriginalClusters.empty() )
  {
    throw Ex{} << "FAT end transaction error";
  }
  else
  {
    mOriginalClusters.clear();
  }
}

uint16_t FAT::findLastCluster( uint16_t cluster ) const
{
  uint16_t maxCluster = maxClusterValue();
  uint16_t lastCluster = lastClusterValue();

  while ( cluster > 0 && cluster < maxCluster )
  {
    if ( cluster > mClusters.size() )
      throw Ex{} << "Cluster index " << cluster << " exceeds FAT size of " << mClusters.size();

    cluster = mClusters[cluster];
  }

  if ( cluster != lastCluster )
    return 0;

  return cluster;
}

uint16_t FAT::maxClusterValue() const
{
  return mClusters.size() > 4086 ? 0xfff0 : 0xff0;
}

uint16_t FAT::lastClusterValue() const
{
  return mClusters.size() > 4086 ? 0xffff : 0xfff;
}

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
