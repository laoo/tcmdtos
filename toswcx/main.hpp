#pragma once

#include "wcxhead.h"

extern "C"
{

HANDLE __declspec( dllexport ) __stdcall OpenArchive( tOpenArchiveData * ArchiveData );
int __declspec( dllexport ) __stdcall ReadHeader( HANDLE hArcData, tHeaderData * HeaderData );
int __declspec( dllexport ) __stdcall ProcessFile( HANDLE hArcData, int Operation, char * DestPath, char * DestName );
int __declspec( dllexport ) __stdcall CloseArchive( HANDLE hArcData );
void __declspec( dllexport ) __stdcall SetChangeVolProc( HANDLE hArcData, tChangeVolProc pChangeVolProc1 );
void __declspec( dllexport ) __stdcall SetProcessDataProc( HANDLE hArcData, tProcessDataProc pProcessDataProc );
int __declspec( dllexport ) __stdcall GetPackerCaps();

}

