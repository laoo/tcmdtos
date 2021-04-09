#pragma once

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
  std::array<char, 11> const & getNameExtArray() const;

  bool isReadOnly() const;
  bool isHidden() const;
  bool isSystem() const;
  bool isLabel() const;
  bool isDirectory() const;
  bool isNew() const;

  void setNameExt( std::string_view src );

  void setYear( uint32_t value );
  void setMonth( uint32_t value );
  void setDay( uint32_t value );
  void setHour( uint32_t value );
  void setMinute( uint32_t value );
  void setSecond( uint32_t value );
  void setCluster( uint16_t value );
  void setSizeInBytes( uint32_t value );
  void setReadOnly( bool value );
  void setHidden( bool value );
  void setSystem( bool value );
  void setLabel( bool value );
  void setDirectory( bool value );
  void setNew( bool value );

  static constexpr uint8_t ATTR_READ_ONLY = 0b00000001;
  static constexpr uint8_t ATTR_HIDDEN =    0b00000010;
  static constexpr uint8_t ATTR_SYSTEM =    0b00000100;
  static constexpr uint8_t ATTR_LABEL =     0b00001000;
  static constexpr uint8_t ATTR_DIRECTORY = 0b00010000;
  static constexpr uint8_t ATTR_NEW =       0b00100000;

  static constexpr uint16_t MASK_YEAR =   0b1111111000000000;
  static constexpr uint16_t MASK_MONTH =  0b0000000111100000;
  static constexpr uint16_t MASK_DAY =    0b0000000000011111;
  static constexpr uint16_t MASK_HOUR =   0b1111100000000000;
  static constexpr uint16_t MASK_MINUTE = 0b0000011111100000;
  static constexpr uint16_t MASK_SECOND = 0b0000000000011111;

};
#pragma pack(pop)

