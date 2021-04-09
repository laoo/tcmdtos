#include "pch.hpp"
#include "Utils.hpp"

void extractNameExt( std::string_view src, std::array<char, 11> & nameExt )
{
  std::fill( nameExt.begin(), nameExt.end(), ' ' );

  size_t pos = 0;
  for ( auto c : src )
  {
    c = toupper( c );

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
