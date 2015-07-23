#pragma once

#include "ProdoDef.h"

#define UNKNOWN_MODULE L"_unknown_"

#define TAG_LIST 'TSIL'

#define TAG_KAPC 'CPAK'

#define MAX_PATH 260

typedef enum _LIST_TYPE
{
	InvalidList = 0,
	ProcList,
	DriverList,
	SsdtList,
	DllList,
} LIST_TYPE;

typedef struct _PROC_LIST_ENTRY {
	LIST_ENTRY lEntry;
	DWORD dwPID;
	WCHAR ustrImageName[MAX_PATH];
	WCHAR ustrImagePath[MAX_PATH];
} PROC_LIST_ENTRY, *PPROC_LIST_ENTRY;

typedef struct _DLLLISTENTRY {
	LIST_ENTRY lEntry;
	DWORD dwBase;
	WCHAR szDllName[MAX_PATH];
} DLL_LIST_ENTRY, *PDLL_LIST_ENTRY;

typedef struct _DRIVER_LIST_ENTRY {
	LIST_ENTRY lEntry;
	DWORD dwBase;
	DWORD dwEnd;
	DWORD dwEntryPoint;
	WCHAR szDrvName[MAX_PATH];
} DRIVER_LIST_ENTRY, *PDRIVER_LIST_ENTRY;

typedef struct _SSDTHOOK_LIST_ENTRY {
	LIST_ENTRY lEntry;
	ULONG unIndex;
	DWORD dwHookAddr;
	DWORD dwBase;
	DWORD dwEnd;
	WCHAR szDrvName[MAX_PATH];
} SSDTHOOK_LIST_ENTRY, *PSSDTHOOK_LIST_ENTRY;

typedef struct _LISTS_ARRAY {
	PLIST_ENTRY pProcListHead;
	PLIST_ENTRY pDllListHead;
	PLIST_ENTRY pDrvListHead;
	PLIST_ENTRY pSsdtListHead;
} LISTS_ARRAY, *PLISTS_ARRAY;

typedef struct _MMVAD_LIST_ENTRY
{
	SINGLE_LIST_ENTRY lEntry;
	PMMVAD pVad;
} MMVAD_LIST_ENTRY, *PMMVAD_LIST_ENTRY;

NTSTATUS InitList(LIST_TYPE ListType);

NTSTATUS AddListEntry( LIST_TYPE eTypeOfList, PVOID pItemToAdd, BOOLEAN bFind );
NTSTATUS GetListEntry( LIST_TYPE eTypeOfList, UINT nPosition, PVOID *ppItem );
UINT GetListCount( LIST_TYPE eTypeOfList );
BOOLEAN IsMyListEmpty( LIST_TYPE eTypeOfList );
NTSTATUS DelAllLists();
NTSTATUS DelList( LIST_TYPE eTypeOfList );

NTSTATUS FindEntry( LIST_TYPE ListType, PVOID pItemToFind );
