#pragma once

#include "BasePartition.hpp"

class GEMPartition : public BasePartition
{
public:
  GEMPartition( PInfo const & partition );
  ~GEMPartition() override = default;
};
