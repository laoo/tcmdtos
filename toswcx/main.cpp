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
  };

  State( char const * name ) : volume{ name }, parts{}, arcName{ name }
  {
    for ( size_t i = 0; i < volume.partitions().size(); ++i )
    {
      std::deque<std::shared_ptr<DirectoryEntry>> folderList;

      Part part{};

      std::stringstream ss;
      auto partition = volume.partitions()[i];
      ss << std::setw( 2 ) << std::setfill( '0' ) << i;
      auto label = partition->getLabel();
      if ( !label.empty() )
      {
        ss << "." << label;
      }
      switch ( partition->type() )
      {
      case PInfo::Type::GEM:
        ss << ".GEM";
        break;
      case PInfo::Type::BGM:
        ss << ".BGM";
        break;
      default:
        assert( false );
        break;
      }

      part.label = ss.str();

      for ( auto const & dir : partition->rootDir() )
      {
        if ( dir->isDirectory() )
        {
          folderList.push_back( dir );
        }
        else
        {
          part.fileList.push_back( dir );
        }
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

    if ( part.fileList.empty() )
    {
      state->parts.pop_back();
    }
    else
    {
      auto file = part.fileList.back();
      part.fileList.pop_back();

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

    std::ofstream fout{ destName, std::ios::binary };
    for ( auto span : state->currentFile->read() )
    {
      fout.write( span.data(), span.size() );
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
}
