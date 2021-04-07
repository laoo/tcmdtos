#pragma once

#include "generator.hpp"
#include <span>

class RawVolume;
class FAT;
class DirEntry;

class BaseFile
{
public:
  BaseFile( std::shared_ptr<RawVolume> rawVolume, std::shared_ptr<FAT> fat, std::shared_ptr<DirEntry> dirEntry );
  virtual ~BaseFile() = default;

  virtual std::shared_ptr<DirEntry> dirEntry() const;
  virtual cppcoro::generator<std::span<int8_t const>> read() const = 0;

  char * fullPath( char * it ) const;

  std::shared_ptr<RawVolume> rawVolume() const;
  std::shared_ptr<FAT> fat() const;

protected:
  std::shared_ptr<RawVolume> mRawVolume;
  std::shared_ptr<FAT> mFAT;
  std::shared_ptr<DirEntry> mDirEntry;
};

class RootDirFile : public BaseFile
{
public:
  RootDirFile( std::shared_ptr<RawVolume> rawVolume, std::shared_ptr<FAT> fat, std::shared_ptr<DirEntry> dirEntry, uint32_t dirPos, uint32_t dirSize );
  ~RootDirFile() override = default;

  cppcoro::generator<std::span<int8_t const>> read() const override;

protected:
  uint32_t mDirPos;
  uint32_t mDirSize;
};

class File : public BaseFile
{
public:
  File( std::shared_ptr<RawVolume> rawVolume, std::shared_ptr<FAT> fat, std::shared_ptr<DirEntry> dirEntry );
  ~File() override = default;

  cppcoro::generator<std::span<int8_t const>> read() const override;
};
