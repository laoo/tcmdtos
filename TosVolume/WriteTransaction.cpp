#include "pch.hpp"
#include "WriteTransaction.hpp"
#include "Log.hpp"
#include "Ex.hpp"

WriteTransaction::WriteTransaction()
{
}

WriteTransaction::~WriteTransaction()
{
  try
  {
    for ( auto & actor : mActors )
    {
      actor->endTransaction();
    }
  }
  catch ( Ex const& ex )
  {
    L_ERROR << "~WriteTransaction error: " << ex.what();
  }
}

void WriteTransaction::transaction( std::shared_ptr<WTActor> actor )
{
  auto it = std::find( mActors.begin(), mActors.end(), actor );

  if ( it == mActors.end() )
  {
    mActors.push_back( actor );
    actor->beginTransaction();
  }
}

void WriteTransaction::commit( RawVolume & volume )
{
  for ( auto & actor : mActors )
  {
    actor->commitTransaction( volume );
    actor->endTransaction();
  }

  mActors.clear();
}
