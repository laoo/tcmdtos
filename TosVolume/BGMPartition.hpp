#pragma once

#include "BasePartition.hpp"

class BGMPartition : public BasePartition
{
public:
  BGMPartition( PInfo const & partition );
  ~BGMPartition() override = default;

};
