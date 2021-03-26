#include "pch.hpp"
#include "main.hpp"
#include "TosVolume/TosVolume.hpp"
#include "TosVolume/Ex.hpp"
#include "TosVolume/Log.hpp"
#include "TosVolume/Partition.hpp"


struct State
{
  struct Part
  {
    std::string label;
    std::vector<std::shared_ptr<DirectoryEntry>> fileList;
    std::shared_ptr<DirectoryEntry> rootDir;
  };

  State( char const * name ) : volume{ name }, parts{}, arcName{ name }
  {
    for ( size_t i = 0; i < volume.partitions().size(); ++i )
    {
      std::deque<std::shared_ptr<DirectoryEntry>> folderList;

      Part part{};

      auto partition = volume.partitions()[i];
      part.label = partition->getLabel();
      part.rootDir = partition->rootDir();

      for ( auto const & dir : part.rootDir->listDir() )
      {
        if ( dir->isDirectory() )
        {
          folderList.push_back( dir );
        }

        part.fileList.push_back( dir );
      }

      while ( !folderList.empty() )
      {
        for ( auto const & dir : folderList.front()->listDir() )
        {
          if ( dir->isDirectory() )
          {
            if ( dir->getName() != "." && dir->getName() != ".." )
            {
              folderList.push_back( dir );
              part.fileList.push_back( dir );
            }
          }
          else
          {
            part.fileList.push_back( dir );
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
  std::shared_ptr<DirectoryEntry> currentFile;
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

    std::shared_ptr<DirectoryEntry> file;

    if ( part.fileList.empty() )
    {
      if ( part.rootDir )
      {
        std::swap( part.rootDir, file );
      }
      else
      {
        state->parts.pop_back();
      }
    }
    else
    {
      file = part.fileList.back();
      part.fileList.pop_back();
    }

    if ( file )
    {
      auto it = headerData->FileName;
      for ( auto c : part.label )
      {
        *it++ = c;
      }
      *it++ = '\\';
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
    for ( auto span : state->currentFile->read() )
    {
      fout.write( span.data(), span.size() );
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
  return PK_CAPS_MULTIPLE | PK_CAPS_DELETE;
}

int __stdcall PackFiles( char * PackedFile, char * SubPath, char * SrcPath, char * AddList, int Flags )
{
  return E_NOT_SUPPORTED;
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
      if ( auto element = volume.find( deleteList ) )
      {
        if ( !element->isDirectory() )
        {
          element->unlink();
        }
        else
        {
          return E_NOT_SUPPORTED;
        }
      }
      else
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
