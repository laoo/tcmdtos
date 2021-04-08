#pragma once

#include "generator.hpp"
#include "WriteTransaction.hpp"

class RawVolume;

class FAT : public WTActor, public std::enable_shared_from_this<FAT>
{
public:

  struct Range
  {
    uint16_t cluster;
    uint16_t size;
  };

  FAT( uint32_t pos, uint32_t size, uint32_t clusterEnd, uint32_t clusterSize, uint32_t dataPos );
  ~FAT() override = default;

  void load( RawVolume & volume );

  uint32_t clusterSize() const;
  uint32_t dataPos() const;

  cppcoro::generator<uint16_t> fileClusters( uint16_t cluster ) const;

  Range findFreeClusterRange( uint32_t reqiredClusters ) const;
  std::vector<uint16_t> findFreeClusters( uint32_t reqiredClusters ) const;

  void freeClusters( WriteTransaction & trans, uint16_t startCluster );

  void beginTransaction() override;
  void commitTransaction( RawVolume & volume ) override;
  void endTransaction() override;

private:
  std::vector<uint16_t> mClusters;
  std::vector<uint16_t> mOriginalClusters;
  uint32_t mPos;
  uint32_t mSize;
  uint32_t mClusterEnd;
  uint32_t mClusterSize;
  uint32_t mDataPos;
};
