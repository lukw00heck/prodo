#pragma once

#include "Lists.h"

#define VALIDATE_LIST_BUFF_SIZE( totalBuffSize, dataType, listType ) ( ( totalBuffSize / sizeof( dataType ) ) <= GetListCount( listType ) )

typedef struct _OS_SPEC_DATA {
	DWORD dwFlinkOffset;
	DWORD dwPidOffset;
	DWORD dwHandleTableOffset;
	DWORD dwHandleListOffset;
	DWORD dwEprocOffset;
	DWORD dwThreadsProcess;
	DWORD dwCID;
	DWORD dwImageFilename;
	DWORD dwCrossThreadFlags;
	DWORD dwSeAuditOffset;
	DWORD dwProcessFlagsOffset;
	DWORD dwCmKeyHiveOffset;
	DWORD dwCmKeyCellOffset;
	DWORD dwCmNodeNameOffset;
	DWORD dwCmNodeNameLenOffset;
	DWORD dwCmKcbLastWriteTime;
	DWORD dwModEntryOffset;
	DWORD dwVadRootOffset;
	DWORD dwPebOffset;
	DWORD dwActiveThreads;
	DWORD dwThreadListHead;
	DWORD dwThreadListEntry;
} OS_SPEC_DATA, *POS_SPEC_DATA;

typedef struct _ARKNTAPI {
	DWORD dwNtOpenProcess;
	DWORD dwNtTerminateProcess;
	DWORD dwNtOpenDirectoryObject;
	DWORD dwNtQueryDirectoryObject;
	DWORD dwNtOpenThread;
	DWORD dwNtTerminateThread;
} ARKNTAPI, *PARKNTAPI;

typedef struct _THRPARAMS {
	PVOID pParam;
	DWORD dwParamLen;
	BOOLEAN bResult;
} THRPARAMS, *PTHRPARAMS;

typedef struct _NTOSKRNLDATA {
	DWORD dwBase;
	DWORD dwEnd;
	DWORD dwEntryPoint;
} NTOSKRNL_DATA, *PNTOSKRNLDATA;

typedef struct _SSDT_MDL {
	PMDL pmdlSSDT;
	PVOID *ppvMappedSSDT;
} SSDT_MDL, *PSSDT_MDL;

NTSTATUS ThreadWrapper( PVOID pfnStartRoutine, PTHRPARAMS pThrParam );

NTSTATUS InitGlobals();
VOID InitGlobalsThread( PVOID pThrParam );
NTSTATUS InitNtApiData( PARKNTAPI pNtApiData );
VOID InitNtApiDataThread( PVOID pThrParam );
NTSTATUS InitSsdtMdl();
NTSTATUS DeInitSsdtMdl();

BOOLEAN IsProcessAlive( PEPROCESS pEproc );
BOOLEAN IsThreadAlive( PETHREAD pEthread );
BOOLEAN IsEthreadValid( PETHREAD pEthread );
ULONG GetPid(PEPROCESS pEproc);

NTSTATUS GetProcessImageName(PEPROCESS pEproc, PUNICODE_STRING pustrImageName);
NTSTATUS GetProcessImagePath(PEPROCESS pEproc, PUNICODE_STRING  pustrPath);

NTSTATUS GetFullPathOfFileObject(PFILE_OBJECT FileObject, PUNICODE_STRING  pustrPath);

DWORD GetTid( PETHREAD pEthread );
DWORD GetPidThr( PETHREAD pEthread );
PEPROCESS GetEprocByPid( DWORD dwPid );
BOOLEAN IsDrvNameKernelA( CHAR *pszDrvName );
BOOLEAN IsDrvNameKernelW( WCHAR *pwszDrvName );

BOOLEAN IsAddressInAnyDriver( DWORD dwAddress, PDRIVER_LIST_ENTRY pDrv );

VOID DisableReadOnly();
VOID EnableReadOnly();
BOOLEAN IsJumpOutsideKernel( DWORD dwJumpToAddr );
DWORD GetJumpToAddr( BYTE *pbSrcAddr, int nOpCode );
BOOLEAN IsValidDeviceDriverObject( PDEVICE_OBJECT pDevObj );
NTSTATUS NtZwTerminateProcess( DWORD dwPid );
NTSTATUS NtZwTerminateProcessByThreads( DWORD dwPid );

PDWORD GetPsLoadedModuleList();
BOOLEAN IsDummyModuleEntry( PLDR_MODULE pModuleToChk );
