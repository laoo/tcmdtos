#pragma once

class RawVolume;

class PInfo
{
  uint32_t statusAndName;
  uint32_t offset;
  uint32_t size;

public:

  enum struct Type
  {
    UNKNOWN,
    GEM,
    BGM,
    XGM
  };

  bool exists() const;
  Type type() const;
  uint32_t partitionOffset() const;
  uint32_t partitionSize() const;
};

#pragma pack(push, 1)
struct BPB
{
  uint16_t bps;
  uint8_t spc;
  uint16_t res;
  uint8_t nfats;
  uint16_t ndirs;
  uint16_t nsects;
  uint8_t media;
  uint16_t spf;
  uint16_t spt;
  uint16_t nheads;
  uint16_t nhid;
};
#pragma pack(pop)

class Partition
{
public:
  Partition( PInfo const & partition, uint32_t offset, std::shared_ptr<RawVolume> rawVolume );
  virtual ~Partition() = default;

protected:
  std::shared_ptr<RawVolume> mRawVolume;
  BPB mBPB;
};
