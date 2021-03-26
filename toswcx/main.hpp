#pragma once

#include "wcxhead.h"

#ifdef TOSWCX_EXPORTS
#define DLL_EXPORT __declspec( dllexport )
#else
#define DLL_EXPORT
#endif

extern "C"
{

HANDLE DLL_EXPORT __stdcall OpenArchive( tOpenArchiveData * ArchiveData );
int DLL_EXPORT __stdcall ReadHeader( HANDLE hArcData, tHeaderData * HeaderData );
int DLL_EXPORT __stdcall ProcessFile( HANDLE hArcData, int Operation, char * DestPath, char * DestName );
int DLL_EXPORT __stdcall CloseArchive( HANDLE hArcData );
void DLL_EXPORT __stdcall SetChangeVolProc( HANDLE hArcData, tChangeVolProc pChangeVolProc1 );
void DLL_EXPORT __stdcall SetProcessDataProc( HANDLE hArcData, tProcessDataProc pProcessDataProc );
int DLL_EXPORT __stdcall GetPackerCaps();
int DLL_EXPORT __stdcall PackFiles( char * PackedFile, char * SubPath, char * SrcPath, char * AddList, int Flags );
int DLL_EXPORT __stdcall DeleteFiles( char * PackedFile, char * DeleteList );
BOOL DLL_EXPORT __stdcall CanYouHandleThisFile( char * FileName );

}
