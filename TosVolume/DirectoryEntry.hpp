#pragma once

struct TOSDir;

class DirectoryEntry
{
public:
  friend class Partition;
  DirectoryEntry( TOSDir const& dir );
  DirectoryEntry() = delete;

  uint32_t getYear() const;
  uint32_t getMonth() const;
  uint32_t getDay() const;
  uint32_t getHour() const;
  uint32_t getMinute() const;
  uint32_t getSecond() const;

  bool isReadOnly() const;
  bool isHidden() const;
  bool isSystem() const;
  bool isLabel() const;
  bool isDirectory() const;
  bool isNew() const;
  std::string_view getName() const;
  std::string_view getExt() const;

  static constexpr uint8_t ATTR_READ_ONLY = 0x01;
  static constexpr uint8_t ATTR_HIDDEN    = 0x02;
  static constexpr uint8_t ATTR_SYSTEM    = 0x04;
  static constexpr uint8_t ATTR_LABEL     = 0x08;
  static constexpr uint8_t ATTR_DIRECTORY = 0x10;
  static constexpr uint8_t ATTR_NEW       = 0x20;

private:
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
