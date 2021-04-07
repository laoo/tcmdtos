#include "pch.hpp"
#include "Dir.hpp"
#include "File.hpp"
#include "DirEntry.hpp"

namespace
{

bool match( std::array<char, 11> const & left, std::array<char, 11> const & right )
{
  for ( size_t i = 0; i < left.size(); ++i )
  {
    if ( left[i] == '?' || right[i] == '?' )
      continue;
    if ( left[i] != right[i] )
      return false;
  }

  return true;
}

void extractNameExt( std::string_view src, std::array<char, 11> & nameExt )
{
  std::fill( nameExt.begin(), nameExt.end(), ' ' );

  size_t pos = 0;
  for ( auto c : src )
  {
    if ( c == '.' )
    {
      nameExt[8] = nameExt[9] = nameExt[10] = ' ';
      pos = 8;
    }
    else if ( c == '*' )
    {
      while ( pos < nameExt.size() )
        nameExt[pos++] = '?';
    }
    else if ( pos < nameExt.size() )
    {
      nameExt[pos++] = c;
    }
  }
}

std::string_view extractNameExtRest( std::string_view src, std::array<char, 11> & nameExt )
{
  auto backSlash = std::find( src.cbegin(), src.cend(), '\\' );

  extractNameExt( std::string_view{ src.data(), (size_t)std::distance( src.cbegin(), backSlash ) }, nameExt );

  if ( backSlash == src.cend() )
    return {};
  else
    return std::string_view{ &*backSlash + 1, (size_t)std::distance( backSlash + 1, src.cend() ) };
}

}


Dir::Dir( std::shared_ptr<BaseFile> baseFile ) : mBaseFile{ std::move( baseFile ) }
{
}

char * Dir::fullPath( char * it ) const
{
  assert( mBaseFile );
  return mBaseFile->fullPath( it );
}

cppcoro::generator<std::shared_ptr<DirEntry>> Dir::list()
{
  auto baseFile = mBaseFile;
  for ( std::span<int8_t const> block : mBaseFile->read() )
  {
    assert( block.size() % sizeof( TOSDir ) == 0 );
    size_t dirsInCluster = block.size() / sizeof( TOSDir );
    auto tosBlock = std::make_shared<TOSDir[]>( dirsInCluster );
    auto tosDirs = tosBlock.get();
    std::copy( block.begin(), block.end(), std::bit_cast<int8_t*>( tosDirs ) );

    for ( uint32_t i = 0; i < dirsInCluster; ++i )
    {
      TOSDir & tosDir = tosDirs[i];

      if ( tosDir.fnameExt[0] == 0 )
        co_return;

      static constexpr std::array<char, 11> dot1 = { '.',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
      static constexpr std::array<char, 11> dot2 = { '.','.',' ',' ',' ',' ',' ',' ',' ',' ',' ' };

      if ( match( tosDir.fnameExt, dot1 ) || match( tosDir.fnameExt, dot2 ) )
        continue;

      if ( tosDir.fnameExt[0] != (char)0xe5 )
      {
        co_yield std::make_shared<DirEntry>( std::shared_ptr<TOSDir>( tosBlock, &tosDir ), shared_from_this() );
      }
    }
  }
}

cppcoro::generator<std::shared_ptr<DirEntry>> Dir::find( std::string_view pattern )
{
  std::array<char, 11> nameExt{};

  std::string_view rest = extractNameExtRest( pattern, nameExt );

  for ( auto dirEntry : list() )
  {
    if ( !rest.empty() && !dirEntry->isDirectory() )
      continue;

    if ( !match( nameExt, dirEntry->getNameExtArray() ) )
      continue;

    if ( rest.empty() )
    {
      co_yield dirEntry;
    }
    else
    {
      auto dir = dirEntry->openDir();
      assert( dir );
      for ( auto r : dir->find( rest ) )
      {
        co_yield r;
      }
    }
  }
}

std::shared_ptr<BaseFile> Dir::baseFile() const
{
  return mBaseFile;
}