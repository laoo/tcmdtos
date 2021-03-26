#pragma once

#include "DirectoryEntry.hpp"
#include "generator.hpp"

class RawVolume;
class FAT;

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
struct TOSDir
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

class Partition : public std::enable_shared_from_this<Partition>
{
public:
  Partition( PInfo const & partition, uint32_t offset, std::shared_ptr<RawVolume> rawVolume );
  virtual ~Partition() = default;

  std::string getLabel();
  PInfo::Type type() const;

  std::shared_ptr<DirectoryEntry> rootDir();

private:
  friend class DirectoryEntry;

  cppcoro::generator<std::shared_ptr<DirectoryEntry>> listDir( std::shared_ptr<DirectoryEntry const> dir );
  cppcoro::generator<std::span<char const>> read( std::shared_ptr<DirectoryEntry const> dir ) const;
  
  void unlink( std::shared_ptr<DirectoryEntry> dir );

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
#pragma pack(pop)

private:
  std::shared_ptr<RawVolume> mRawVolume;
  std::shared_ptr<FAT> mFAT;
  uint32_t mBootPos;
  uint32_t mFATPos;
  uint32_t mDirPos;
  uint32_t mDataPos;
  uint32_t mDirSize;
  uint32_t mLogicalSectorSize;
  uint32_t mClusterSize;
  uint32_t mClusterEnd; //one past last cluster
  uint32_t mFATSize;
  PInfo::Type mType;
};
