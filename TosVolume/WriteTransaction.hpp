#pragma once

class RawVolume;

class WTActor
{
public:
  virtual ~WTActor() = default;

  virtual void beginTransaction() = 0;
  virtual void commitTransaction( RawVolume & volume ) = 0;
  virtual void endTransaction() = 0;
};

class WriteTransaction
{
public:
  WriteTransaction();
  ~WriteTransaction();

  void transaction( std::shared_ptr<WTActor> actor );
  void commit( RawVolume & volume );

private:
  std::vector<std::shared_ptr<WTActor>> mActors;
};

