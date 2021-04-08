#pragma once

#include <span>
#include "generator.hpp"
#include "WriteTransaction.hpp"

class RawVolume;
class FAT;
class DirEntry;

template<typename T>
struct SharedSpan
{
  std::shared_ptr<T[]> data;
  uint32_t size;

  explicit operator bool() const
  {
    return size != 0;
  }
};

class BaseFile : public WTActor
{
public:
  BaseFile( std::shared_ptr<RawVolume> rawVolume, std::shared_ptr<FAT> fat, std::shared_ptr<DirEntry> dirEntry );
  virtual ~BaseFile() = default;

  virtual std::shared_ptr<DirEntry> dirEntry() const;
  virtual cppcoro::generator<std::span<int8_t const>> read() const = 0;
  virtual cppcoro::generator<SharedSpan<int8_t>> readCached() = 0;

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
  cppcoro::generator<SharedSpan<int8_t>> readCached() override;

  void beginTransaction() override;
  void commitTransaction( RawVolume & volume ) override;
  void endTransaction() override;

protected:
  SharedSpan<int8_t> mCache;
  SharedSpan<int8_t> mOriginalCache;
  uint32_t mDirPos;
  uint32_t mDirSize;
};

class File : public BaseFile
{
public:
  File( std::shared_ptr<RawVolume> rawVolume, std::shared_ptr<FAT> fat, std::shared_ptr<DirEntry> dirEntry );
  ~File() override = default;

  cppcoro::generator<std::span<int8_t const>> read() const override;
  cppcoro::generator<SharedSpan<int8_t>> readCached() override;

  void beginTransaction() override;
  void commitTransaction( RawVolume & volume ) override;
  void endTransaction() override;

private:
  std::vector<SharedSpan<int8_t>> mCache;
  std::vector<SharedSpan<int8_t>> mOriginalCache;

};
