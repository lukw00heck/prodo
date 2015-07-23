#include "ProdoMain.h"

extern NTOSKRNL_DATA g_NtOSKernel;

UINT ScanAndGetDriverCount()
{
	UINT cntDriver = 0;
	__try
	{
		THRPARAMS stThrParams = {0};
		if( STATUS_SUCCESS == ThreadWrapper( ScanAndGetDriverCountThread, &stThrParams ) )
		{
			cntDriver = GetListCount( DriverList );
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		cntDriver = 0;
		DbgPrint( "Exception caught in ScanAndGetDriverCount()" );
	}
	return cntDriver;
}

VOID ScanAndGetDriverCountThread( PVOID pThrParam )
{
	__try
	{
		if( MmIsAddressValid( pThrParam ) )
		{
			if( STATUS_SUCCESS == InitList( DriverList ) )
			{
				GetDriversByModuleEntryScan();

				((PTHRPARAMS)pThrParam)->bResult = !IsMyListEmpty( DriverList );
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		DbgPrint( "Exception caught in ScanAndGetDriverCountThread()" );
	}
	PsTerminateSystemThread( STATUS_SUCCESS );
}

NTSTATUS GetDriversByModuleEntryScan()
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;
	__try
	{
		DRIVER_LIST_ENTRY drvEntry;

		PLDR_MODULE pDrvModule = NULL;
		PLIST_ENTRY pModEntryCurrent = (PLIST_ENTRY)GetPsLoadedModuleList();
		PLIST_ENTRY pModEntryFirst = pModEntryCurrent;
		do
		{
			if( MmIsAddressValid( pModEntryCurrent ) )
			{
				pDrvModule = (PLDR_MODULE)pModEntryCurrent;
				if( !IsDummyModuleEntry( pDrvModule ) )
				{
					if( IsDrvNameKernelW( pDrvModule->FullDllName.Buffer ) )
					{
						g_NtOSKernel.dwBase = (DWORD)pDrvModule->BaseAddress;
						g_NtOSKernel.dwEnd = (DWORD)pDrvModule->BaseAddress + pDrvModule->SizeOfImage;
						g_NtOSKernel.dwEntryPoint = (DWORD)pDrvModule->EntryPoint;
					}
					RtlZeroMemory( &drvEntry, sizeof( DRIVER_LIST_ENTRY ) );
					drvEntry.dwBase = (DWORD)(pDrvModule->BaseAddress);
					drvEntry.dwEnd = (DWORD)(pDrvModule->BaseAddress) + pDrvModule->SizeOfImage;
					drvEntry.dwEntryPoint = (DWORD)(pDrvModule->EntryPoint);
					RtlStringCchCopyW( drvEntry.szDrvName, MAX_PATH, pDrvModule->BaseDllName.Buffer );

					retStatus = AddListEntry( DriverList, &drvEntry, TRUE );
				}
			}
			else
			{
				break;
			}
			pModEntryCurrent = pModEntryCurrent->Blink;
		}
		while( pModEntryCurrent != pModEntryFirst );

	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		retStatus = STATUS_UNSUCCESSFUL;
		DbgPrint( "Exception caught in GetDriversByModuleEntryScan()" );
	}

	return retStatus;
}

