#pragma once

#include "Partition.hpp"

class TosVolume
{
public:
  TosVolume( std::filesystem::path const & path );
  TosVolume();

  std::span<std::shared_ptr<Partition> const> partitions() const;

private:

  static constexpr size_t pinfoOffset = 0x1c6;


private:
  void parseRootSector();
  void parseGEMPartition( PInfo const& partition, uint32_t offset = 0 );
  void parseBGMPartition( PInfo const& partition, uint32_t offset = 0 );
  void parseXGMPartition( PInfo const& partition, uint32_t offset = 0 );

private:
  std::shared_ptr<RawVolume> mRawVolume;
  std::vector<std::shared_ptr<Partition>> mPartitions;
};
