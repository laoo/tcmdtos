#pragma once

#include "generator.hpp"

class RawVolume
{
public:
  static constexpr uint64_t RAW_SECTOR_SIZE = 512;

  static cppcoro::generator<std::shared_ptr<RawVolume>> enumeratePhysicalVolumes();
  static std::shared_ptr<RawVolume> openImageFile( std::filesystem::path const & path );

  std::string readSectors( uint32_t sector, uint32_t count );
  void readSectors( uint32_t sector, uint32_t count, std::span<uint8_t> destination );

  ~RawVolume();

private:
  RawVolume( wchar_t volume );
  RawVolume( std::filesystem::path const & path );

private:
  HANDLE mHandle;
};
