#include "pch.hpp"
#include "WriteTransaction.hpp"
#include "RawVolume.hpp"

WriteTransaction::WriteTransaction() : mNodes{}
{
}

WriteTransaction::WriteTransaction( uint32_t sector, uint32_t offset, std::vector<uint8_t> data ) : mNodes{}
{
  mNodes.push_back( { sector, offset, std::move( data ) } );
}

WriteTransaction::WriteTransaction( WriteTransaction const & other ) : mNodes{}
{
  std::copy( other.mNodes.cbegin(), other.mNodes.cend(), std::back_inserter( mNodes ) );
}

void WriteTransaction::add( uint32_t sector, uint32_t offset, std::vector<uint8_t> data )
{
  mNodes.push_back( { sector, offset, std::move( data ) } );
}

WriteTransaction & WriteTransaction::operator+=( WriteTransaction const& other )
{
  std::copy( other.mNodes.cbegin(), other.mNodes.cend(), std::back_inserter( mNodes ) );
  return *this;
}

void WriteTransaction::commit( RawVolume & volume ) const
{
  for ( auto const & node : mNodes )
  {
    volume.writeFragment( node.sector, node.offset, { node.data.data(), node.data.size() } );
  }
}

WriteTransaction operator+( WriteTransaction const & left, WriteTransaction const & right )
{
  WriteTransaction result{ left };
  return result += right;
}
