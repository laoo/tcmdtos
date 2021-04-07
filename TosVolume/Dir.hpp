#pragma once

#include "generator.hpp"

class BaseFile;
class DirEntry;

class Dir : public std::enable_shared_from_this<Dir>
{
public:
  explicit Dir( std::shared_ptr<BaseFile> baseFile );
  ~Dir() = default;

  char* fullPath( char * it ) const;

  cppcoro::generator<std::shared_ptr<DirEntry>> list();
  cppcoro::generator<std::shared_ptr<DirEntry>> find( std::string_view pattern );

  std::shared_ptr<BaseFile> baseFile() const;


private:
  std::shared_ptr<BaseFile> mBaseFile;
};
