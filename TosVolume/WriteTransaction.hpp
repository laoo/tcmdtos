#pragma once

class RawVolume;

class WriteTransaction
{
public:
  WriteTransaction();
  WriteTransaction( uint32_t sector, uint32_t offset, std::vector<uint8_t> data );
  WriteTransaction( WriteTransaction const& other );

  void add( uint32_t sector, uint32_t offset, std::vector<uint8_t> data );
  WriteTransaction & operator+= ( WriteTransaction const& other );
  friend WriteTransaction operator+ ( WriteTransaction const & left, WriteTransaction const & right );

  void commit( RawVolume & volume ) const;

private:
  struct Node
  {
    uint32_t sector;
    uint32_t offset;
    std::vector<uint8_t> data;
  };

  std::vector<Node> mNodes;
};
