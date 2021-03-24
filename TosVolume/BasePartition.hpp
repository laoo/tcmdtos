#pragma once

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

class BasePartition
{
public:
  virtual ~BasePartition() = default;
};
