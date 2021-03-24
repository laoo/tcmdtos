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

class Partition
{
public:
  Partition( PInfo const & partition, uint32_t offset, std::shared_ptr<RawVolume> rawVolume );
  virtual ~Partition() = default;

private:
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

  struct Dir
  {
    std::array<char, 8> fname;
    std::array<char, 3> fext;
    uint8_t attrib;
    std::array<uint8_t, 10> res;
    uint16_t ftime;
    uint16_t fdate;
    uint16_t scluster;
    uint32_t fsize;
  };
#pragma pack(pop)

protected:
  std::shared_ptr<RawVolume> mRawVolume;
  uint32_t mPosFat;
  uint32_t mPosDir;
  uint32_t mPosData;
};
