#pragma once

struct TOSDir;
class Partition;
#include "generator.hpp"

class DirectoryEntry : public std::enable_shared_from_this<DirectoryEntry>
{
public:
  friend class Partition;
  DirectoryEntry( std::shared_ptr<Partition const> partition, TOSDir const & dir, std::shared_ptr<DirectoryEntry const> parent = {} );
  DirectoryEntry() = delete;

  uint32_t getYear() const;
  uint32_t getMonth() const;
  uint32_t getDay() const;
  uint32_t getHour() const;
  uint32_t getMinute() const;
  uint32_t getSecond() const;
  uint16_t getCluster() const;
  uint32_t getSizeInBytes() const;

  bool isReadOnly() const;
  bool isHidden() const;
  bool isSystem() const;
  bool isLabel() const;
  bool isDirectory() const;
  bool isNew() const;
  std::string_view getName() const;
  std::string_view getExt() const;
  
  template<typename IT>
  IT nameWithExt( IT it ) const
  {
    for ( auto c : getName() )
    {
      *it++ = c;
    }
    auto ext = getExt();
    if ( !ext.empty() )
    {
      *it++ = '.';
      for ( auto c : ext )
      {
        *it++ = c;
      }
    }
    return it;
  }

  template<typename IT>
  IT fullPath( IT it ) const
  {
    if ( mParent )
    {
      it = mParent->fullPath( it );
      *it++ = '\\';
    }
    return nameWithExt( it );
  }

  cppcoro::generator<std::shared_ptr<DirectoryEntry>> listDir() const;
  cppcoro::generator<std::span<char const>> read() const;

  static constexpr uint8_t ATTR_READ_ONLY = 0x01;
  static constexpr uint8_t ATTR_HIDDEN    = 0x02;
  static constexpr uint8_t ATTR_SYSTEM    = 0x04;
  static constexpr uint8_t ATTR_LABEL     = 0x08;
  static constexpr uint8_t ATTR_DIRECTORY = 0x10;
  static constexpr uint8_t ATTR_NEW       = 0x20;

private:
  std::shared_ptr<Partition const> mPartition;
  std::shared_ptr<DirectoryEntry const> mParent;
  uint32_t mYear;
  uint32_t mMonth;
  uint32_t mDay;
  uint32_t mHour;
  uint32_t mMinute;
  uint32_t mSecond;
  uint32_t mSize;
  uint16_t mCluster;
  uint8_t mAttrib;
  std::array<char, 8> mName;
  std::array<char, 3> mExt;
};
