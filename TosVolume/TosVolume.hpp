#pragma once

class RawVolume;
#include "BasePartition.hpp"

class TosVolume
{
public:
  TosVolume( std::filesystem::path const & path );
  TosVolume();

private:

  static constexpr size_t pinfoOffset = 0x1c6;

#pragma pack(push, 1)
  struct BPB
  {
    uint16_t bps;
    uint8_t spc;
    uint16_t res;
    uint8_t nfats;
    uint16_t ndirs;


  };
#pragma pack(pop)

private:
  void parseRootSector();
  void parseGEMPartition( PInfo const& partition );
  void parseBGMPartition( PInfo const& partition );
  void parseXGMPartition( PInfo const& partition );

private:
  std::shared_ptr<RawVolume> mRawVolume;
  std::vector<std::shared_ptr<BasePartition>> mPartitions;
};
