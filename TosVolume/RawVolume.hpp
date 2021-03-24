#pragma once

#include "generator.hpp"

class RawVolume
{
public:
  static constexpr uint64_t RAW_SECTOR_SIZE = 512;

  static cppcoro::generator<std::shared_ptr<RawVolume>> enumeratePhysicalVolumes();
  static std::shared_ptr<RawVolume> openImageFile( std::filesystem::path const& path );

  std::span<uint8_t const, RAW_SECTOR_SIZE> readSector( uint32_t sector );

private:
  RawVolume( wchar_t volume );
  RawVolume( std::filesystem::path const & path );

private:
  HANDLE mHandle;
  std::vector<uint8_t> mSector;
  LARGE_INTEGER mSectorOffset;
  uint64_t mOffsetMask;
};
