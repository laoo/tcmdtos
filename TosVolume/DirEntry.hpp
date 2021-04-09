#pragma once

#include <array>

struct TOSDir;
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
  std::string nameWithExt() const;
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

private:
  std::shared_ptr<TOSDir> mTOS;
  std::shared_ptr<Dir> mDir;
};
