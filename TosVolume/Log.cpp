#include "pch.hpp"
#include "Log.hpp"

Log::Log() : mLogLevel{ LL_INFO }
{
}

void Log::setLogLevel( LogLevel ll )
{
  mLogLevel = ll;
}

void Log::log( LogLevel ll, std::string const & message )
{
  if ( ll >= mLogLevel )
  {
    OutputDebugStringA( message.c_str() );
  }
}

Log & Log::instance()
{
  static Log instance{};
  return instance;
}

Formatter::Formatter( Log::LogLevel ll ) : mLl{ ll }, mSS{}
{
}

Formatter::~Formatter()
{
  mSS << std::endl;
  Log::instance().log( mLl, mSS.str() );
}
