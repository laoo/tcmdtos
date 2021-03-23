#include "pch.hpp"
#include "TosVolume.hpp"
#include "RawVolume.hpp"
#include "Ex.hpp"

TosVolume::TosVolume( std::filesystem::path const & path ) : mRawVolume{ RawVolume::openImageFile( path ) }
{
  if ( !mRawVolume )
    throw Ex{} << "Error opening image file " << path.string();

  parseRootSector();
}

TosVolume::TosVolume()
{
}

void TosVolume::parseRootSector()
{
  static constexpr size_t pinfoOffset = 0x1c6;

  auto rootSector = mRawVolume->readSector( 0 );

  std::span<PInfo const, 4> infos{ reinterpret_cast<PInfo const *>( rootSector.data() + pinfoOffset ), 4 };

  for ( auto const & info : infos )
  {
    if ( info.exists() )
    {
      auto type = info.type();
      int x = 42;

    }
  }


  return;
}

bool TosVolume::PInfo::exists() const
{
  return ( statusAndName & 1 ) != 0;
}

TosVolume::PInfo::Type TosVolume::PInfo::type() const
{
  std::string_view name{ reinterpret_cast<char const *>( &statusAndName ) + 1, 3 };

  if ( name == "GEM" )
  {
    return Type::GEM;
  }
  else if ( name == "BGM" )
  {
    return Type::BGM;
  }
  else if ( name == "XGM" )
  {
    return Type::XGM;
  }
  else
  {
    return Type::UNKNOWN;
  }
}

uint32_t TosVolume::PInfo::partitionOffset() const
{
  return uint32_t();
}

uint32_t TosVolume::PInfo::partitionSize() const
{
  return uint32_t();
}
