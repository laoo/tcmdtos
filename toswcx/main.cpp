#include "pch.hpp"
#include "main.hpp"
#include "TosVolume/TosVolume.hpp"
#include "TosVolume/Ex.hpp"
#include "TosVolume/Log.hpp"
#include "TosVolume/Partition.hpp"
#include "TosVolume/Dir.hpp"
#include "TosVolume/File.hpp"
#include "TosVolume/DirEntry.hpp"


struct State
{
  struct Part
  {
    std::vector<std::shared_ptr<DirEntry>> fileList;
  };

  State( char const * name ) : volume{ name }, parts{}, arcName{ name }
  {
    for ( size_t i = 0; i < volume.partitions().size(); ++i )
    {
      std::deque<std::shared_ptr<DirEntry>> folderList;

      Part part{};

      auto partition = volume.partitions()[i];
      auto rootDir = partition->rootDir();

      part.fileList.push_back( rootDir->baseFile()->dirEntry() );

      for ( auto dir : rootDir->list() )
      {
        if ( dir->isDirectory() )
        {
          folderList.push_back( dir );
        }

        part.fileList.push_back( dir );
      }

      while ( !folderList.empty() )
      {
        auto dir = folderList.front()->openDir();
        for ( auto const & dirEntry : dir->list() )
        {
          if ( dirEntry->isDirectory() )
          {
            folderList.push_back( dirEntry );
            part.fileList.push_back( dirEntry );
          }
          else
          {
            part.fileList.push_back( dirEntry );
          }
        }
        folderList.pop_front();
      }

      parts.push_back( std::move( part ) );
    }
  }


  TosVolume volume;
  std::vector<Part> parts;
  std::string arcName;
  std::shared_ptr<DirEntry> currentFile;
};

tProcessDataProc processDataProc;


HANDLE __stdcall OpenArchive( tOpenArchiveData * archiveData )
{
  try
  {
    auto state = new State{ archiveData->ArcName };
    return HANDLE( state );
  }
  catch ( [[maybe_unused]] Ex const & ex )
  {
    archiveData->OpenResult = E_BAD_ARCHIVE;
    return 0;
  }
}

int __stdcall ReadHeader( HANDLE hArcData, tHeaderData * headerData )
{
  auto state = (State *)hArcData;

  for ( ;; )
  {
    if ( state->parts.empty() )
      return E_END_ARCHIVE;

    auto & part = state->parts.back();

    std::shared_ptr<DirEntry> file;

    if ( part.fileList.empty() )
    {
      state->parts.pop_back();
    }
    else
    {
      file = part.fileList.back();
      part.fileList.pop_back();
    }

    if ( file )
    {
      auto it = headerData->FileName;
      file->fullPath( it );
      if ( file->isReadOnly() )
        headerData->FileAttr |= 0x01;
      if ( file->isHidden() )
        headerData->FileAttr |= 0x02;
      if ( file->isSystem() )
        headerData->FileAttr |= 0x04;
      if ( file->isLabel() )
        headerData->FileAttr |= 0x08;
      if ( file->isDirectory() )
        headerData->FileAttr |= 0x10;
      headerData->PackSize = file->getSizeInBytes();
      headerData->UnpSize = file->getSizeInBytes();
      headerData->FileTime = ( ( file->getYear() - 1980 ) << 25 ) | ( file->getMonth() << 21 ) | ( file->getDay() << 16 ) | ( file->getHour() << 11 ) | ( file->getMinute() << 5 ) | ( file->getSecond() / 2 );

      it = headerData->ArcName;
      for ( auto c : state->arcName )
      {
        *it++ = c;
      }

      state->currentFile = file;
      return 0;
    }
  }
}

int __stdcall ProcessFile( HANDLE hArcData, int operation, char * destPath, char * destName )
{
  if ( operation == PK_SKIP )
  {
    return 0;
  }
  else
  {
    auto state = (State *)hArcData;

    std::filesystem::path path;

    if ( destPath )
    {
      path = destPath;
      path /= destName;
    }
    else
    {
      path = destName;
    }

    std::ofstream fout{ path, std::ios::binary };
    auto file = state->currentFile->openFile();
    for ( auto span : file->read() )
    {
      fout.write( std::bit_cast<const char*>( span.data() ), span.size() );
      if ( processDataProc( nullptr, (int)span.size() ) == 0 )
        return E_EABORTED;
    }

    return 0;
  }
}

int __stdcall CloseArchive( HANDLE hArcData )
{
  auto state = (State *)hArcData;
  delete state;

  return 0;
}

void __stdcall SetChangeVolProc( HANDLE hArcData, tChangeVolProc pChangeVolProc1 )
{
}

void __stdcall SetProcessDataProc( HANDLE hArcData, tProcessDataProc pProcessDataProc )
{
  processDataProc = pProcessDataProc;
}

int __stdcall GetPackerCaps()
{
  return PK_CAPS_NEW | PK_CAPS_MULTIPLE | PK_CAPS_DELETE | PK_CAPS_MODIFY;
}

int __stdcall PackFiles( char * packedFile, char * subPath, char * srcPath, char * addList, int flags )
{
  try
  {
    TosVolume volume{ packedFile };

    if ( !addList )
      return E_NO_FILES;

    while ( *addList )
    {
      std::filesystem::path src{ srcPath };
      src /= addList;
      std::filesystem::path dst{ subPath };
      dst /= addList;

      if ( !std::filesystem::exists( src ) )
        return E_EOPEN;


      if ( std::filesystem::is_directory( src ) )
      {
        auto dststr = src.string();

        if ( !volume.mkdir( std::string_view{ dststr } ) )
        {
          return E_ECREATE;
        }
      }
      else
      {
        auto srcstr = src.string();
        auto parent = dst.parent_path();
        auto leaf = dst.filename();
        auto dstpar = parent.string();
        auto dstlea = leaf.string();

        if ( !volume.add( std::string_view{ srcstr }, std::string_view{ dstpar }, std::string_view{ dstlea } ) )
        {
          return E_ECREATE;
        }
      }

      auto size = strlen( addList );
      addList += size + 1;
    }

    return 0;
  }
  catch ( [[maybe_unused]] Ex const & ex )
  {
    return E_BAD_ARCHIVE;
  }
}

int __stdcall DeleteFiles( char * packedFile, char * deleteList )
{
  try
  {
    TosVolume volume{ packedFile };

    if ( !deleteList )
      return E_NO_FILES;

    while ( *deleteList )
    {
      if ( !volume.unlink( deleteList ) )
      {
        return E_NO_FILES;
      }
      
      auto size = strlen( deleteList );
      deleteList += size + 1;
    }

    return 0;
  }
  catch ( [[maybe_unused]] Ex const & ex )
  {
    return E_BAD_ARCHIVE;
  }
}

BOOL __stdcall CanYouHandleThisFile( char * FileName )
{
  try
  {
    TosVolume volume{ FileName };
    return 1;
  }
  catch ( [[maybe_unused]] Ex const & ex )
  {
    return 0;
  }
}
