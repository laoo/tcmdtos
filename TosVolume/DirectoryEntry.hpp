#pragma once

struct TOSDir;
class Partition;
#include "generator.hpp"

class DirectoryEntry : public std::enable_shared_from_this<DirectoryEntry>
{
public:
  DirectoryEntry( std::shared_ptr<Partition> partition ); //root
  DirectoryEntry( std::shared_ptr<Partition> partition, TOSDir const & dir, uint32_t sector, uint32_t offset, std::shared_ptr<DirectoryEntry const> parent = {} );
  DirectoryEntry() = delete;

  uint32_t getYear() const;
  uint32_t getMonth() const;
  uint32_t getDay() const;
  uint32_t getHour() const;
  uint32_t getMinute() const;
  uint32_t getSecond() const;
  uint16_t getCluster() const;
  uint32_t getSizeInBytes() const;
  std::string_view getName() const;
  std::string_view getExt() const;

  bool isReadOnly() const;
  bool isHidden() const;
  bool isSystem() const;
  bool isLabel() const;
  bool isDirectory() const;
  bool isNew() const;
  
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
    }
    it = nameWithExt( it );
    if ( isDirectory() )
      *it++ = '\\';
    return it;
  }

  std::shared_ptr<DirectoryEntry> findChild( std::string_view name ) const;
  cppcoro::generator<std::shared_ptr<DirectoryEntry>> listDir() const;
  cppcoro::generator<std::span<char const>> read() const;
  std::pair< uint32_t, uint32_t> getLocationInPartition() const;

  bool unlink();


  static constexpr uint8_t ATTR_READ_ONLY = 0x01;
  static constexpr uint8_t ATTR_HIDDEN    = 0x02;
  static constexpr uint8_t ATTR_SYSTEM    = 0x04;
  static constexpr uint8_t ATTR_LABEL     = 0x08;
  static constexpr uint8_t ATTR_DIRECTORY = 0x10;
  static constexpr uint8_t ATTR_NEW       = 0x20;

  static bool extractNameExt( std::string_view src, std::array<char, 8> & name, std::array<char, 3> & ext );

private:
  std::shared_ptr<Partition> mPartition;
  std::shared_ptr<DirectoryEntry const> mParent;
  uint32_t mYear;
  uint32_t mMonth;
  uint32_t mDay;
  uint32_t mHour;
  uint32_t mMinute;
  uint32_t mSecond;
  uint32_t mSize;
  uint16_t mCluster;
  uint32_t mSector;
  uint32_t mOffset;
  uint8_t mAttrib;
  std::array<char, 8> mName;
  std::array<char, 3> mExt;
};
