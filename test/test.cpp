#include "pch.hpp"
#include "TosVolume/TosVolume.hpp"
#include "TosVolume/Ex.hpp"
#include "TosVolume/Log.hpp"
#include "TosVolume/Partition.hpp"

int main()
{
  TosVolume volume{ "j:\\ST\\orgUltraSatan.img" };

  auto partitions = volume.partitions();

  for ( size_t i = 0; i < partitions.size(); ++i )
  {
    std::stringstream ss;
    auto partition = partitions[i];
    ss << std::setw(2) << std::setfill('0') << i;
    auto label = partition->getLabel();
    if ( !label.empty() )
    {
      ss << "." << label;
    }
    switch ( partition->type() )
    {
    case PInfo::Type::GEM:
      ss << ".GEM";
      break;
    case PInfo::Type::BGM:
      ss << ".BGM";
      break;
    default:
      assert( false );
      break;
    }

    L_NOTICE << ss.str();
  }
}
