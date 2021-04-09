#pragma once

#include "Partition.hpp"
class DirEntry;

class TosVolume
{
public:
  TosVolume( std::filesystem::path const & path );
  TosVolume();

  std::span<std::shared_ptr<Partition> const> partitions() const;

  cppcoro::generator<std::shared_ptr<DirEntry>> find( std::string_view fullPath ) const;

  //bool mkdir( std::string_view fullPath );
  bool add( std::string_view dstFolder, std::string_view srcFolder, std::string_view path );
  bool unlink( std::string_view path ) const;

private:
  std::pair<std::shared_ptr<Partition>, std::string_view> findPartition( std::string_view path ) const;

private:

  static constexpr size_t pinfoOffset = 0x1c6;


private:
  void parseRootSector();
  void parseGEMPartition( PInfo const& partition, uint32_t offset = 0 );
  void parseBGMPartition( PInfo const& partition, uint32_t offset = 0 );
  void parseXGMPartition( PInfo const& partition, uint32_t offset = 0 );


private:
  std::shared_ptr<RawVolume> mRawVolume;
  std::vector<std::shared_ptr<Partition>> mPartitions;
};
