#pragma once

#include <array>

#pragma pack(push, 1)
struct TOSDir
{
  TOSDir() : fnameExt{}, attrib{}, res{}, ftime{}, fdate{}, scluster{}, fsize{}
  {
    std::fill( fnameExt.begin(), fnameExt.end(), ' ' );
  }

  std::array<char, 11> fnameExt;
  uint8_t attrib;
  std::array<uint8_t, 10> res;
  uint16_t ftime;
  uint16_t fdate;
  uint16_t scluster;
  uint32_t fsize;
};
#pragma pack(pop)

class Dir;
class BaseFile;
class WriteTransaction;

class DirEntry : public std::enable_shared_from_this<DirEntry>
{
public:
  DirEntry( std::shared_ptr<TOSDir> tos, std::shared_ptr<Dir> dir );

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
  std::array<char, 11> const& getNameExtArray() const;

  bool isReadOnly() const;
  bool isHidden() const;
  bool isSystem() const;
  bool isLabel() const;
  bool isDirectory() const;
  bool isNew() const;

  char * nameWithExt( char * it ) const;
  char * fullPath( char * it ) const;

  std::shared_ptr<Dir> openDir();
  std::shared_ptr<BaseFile> openFile();

  void unlink( WriteTransaction & transaction );

  static constexpr uint8_t ATTR_READ_ONLY = 0x01;
  static constexpr uint8_t ATTR_HIDDEN = 0x02;
  static constexpr uint8_t ATTR_SYSTEM = 0x04;
  static constexpr uint8_t ATTR_LABEL = 0x08;
  static constexpr uint8_t ATTR_DIRECTORY = 0x10;
  static constexpr uint8_t ATTR_NEW = 0x20;

private:
  std::shared_ptr<TOSDir> mTOS;
  std::shared_ptr<Dir> mDir;
};
