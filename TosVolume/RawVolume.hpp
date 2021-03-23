#pragma once

#include "generator.hpp"

class RawVolume
{
  static constexpr uint64_t LOGICAL_SECTOR_SIZE = 512;

public:
  static cppcoro::generator<std::shared_ptr<RawVolume>> enumeratePhysicalVolumes();
  static std::shared_ptr<RawVolume> openImageFile( std::filesystem::path const& path );

  std::span<uint8_t const, LOGICAL_SECTOR_SIZE> readSector( int64_t sector );

private:
  RawVolume( wchar_t volume );
  RawVolume( std::filesystem::path const & path );

private:
  HANDLE mHandle;
  std::vector<uint8_t> mSector;
  LARGE_INTEGER mSectorOffset;
  uint64_t mOffsetMask;
};
