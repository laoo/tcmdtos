#pragma once

#include "generator.hpp"

class RawVolume;

class FAT
{
public:

  struct Range
  {
    uint16_t cluster;
    uint16_t size;
  };

  FAT( uint32_t pos, uint32_t size, uint32_t clusterEnd, uint32_t clusterSize, uint32_t dataPos );
  void load( RawVolume & volume );

  uint32_t clusterSize() const;
  uint32_t dataPos() const;

  cppcoro::generator<uint16_t> fileClusters( uint16_t cluster ) const;

  Range findFreeClusterRange( uint32_t reqiredClusters ) const;
  std::vector<uint16_t> findFreeClusters( uint32_t reqiredClusters ) const;

  //WriteTransaction freeClusters( uint16_t startCluster );

private:
  //class Modifier
  //{
  //public:
  //  void add( uint16_t cluster );
  //  WriteTransaction commit( FAT const& fat );

  //  std::vector<uint16_t> mModified;
  //};

private:
  std::vector<uint16_t> mClusters;
  uint32_t mPos;
  uint32_t mSize;
  uint32_t mClusterEnd;
  uint32_t mClusterSize;
  uint32_t mDataPos;
};
