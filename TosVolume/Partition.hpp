#pragma once

#include "generator.hpp"

class Dir;
class RawVolume;
class FAT;
class BaseFile;
struct TOSDir;

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


class Partition : public std::enable_shared_from_this<Partition>
{
public:
  Partition( int number, PInfo const & partition, uint32_t offset, std::shared_ptr<RawVolume> rawVolume );
  virtual ~Partition() = default;

  PInfo::Type type() const;
  int number() const;

  std::shared_ptr<Dir> rootDir();
  bool add( std::string_view dstFolder, std::vector<TOSDir> const & path, std::function<std::span<uint8_t const>()> const & dataSource );

private:
  friend class DirectoryEntry;

  std::shared_ptr<BaseFile> rootDirFile() const;
  
  //void removeDirectoryEntry( std::shared_ptr<DirectoryEntry> dir, WriteTransaction & trans );

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
  int mNumber;
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
