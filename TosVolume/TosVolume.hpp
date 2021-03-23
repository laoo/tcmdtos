#pragma once

class RawVolume;

class TosVolume
{
public:
  TosVolume( std::filesystem::path const & path );
  TosVolume();

private:

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

private:
  void parseRootSector();

private:
  std::shared_ptr<RawVolume> mRawVolume;
};
