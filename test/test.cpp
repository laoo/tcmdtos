#include "pch.hpp"
#include "TosVolume/TosVolume.hpp"
#include "TosVolume/Ex.hpp"
#include "TosVolume/Log.hpp"
#include "TosVolume/Partition.hpp"

int main()
{
  TosVolume volume{ "j:\\ST\\orgUltraSatan.img" };

  for ( auto const & part : volume.partitions() )
  {
    std::cout << part->getLabel();
  }

  std::cout << "Hello World!\n";
}
