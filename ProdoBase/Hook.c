#include "ProdoMain.h"

extern NTOSKRNL_DATA g_NtOSKernel;

extern SSDT_MDL g_mdlSSDT;

UINT SSDThooksCount()
{
	UINT cntHooks = 0;

	__try
	{
		THRPARAMS stThrParams = {0};
		if( STATUS_SUCCESS == ThreadWrapper(SSDTscanThread, &stThrParams) )
		{
			cntHooks = GetListCount(SsdtList);
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		cntHooks = 0;
		DbgPrint( "Exception caught in ScanAndGetSSDTHooksCount()" );
	}
	return cntHooks;
}

VOID SSDTscanThread(PVOID pThrParam)
{
	__try
	{
		if ( MmIsAddressValid( pThrParam ) && ( STATUS_SUCCESS == InitList(SsdtList) ) )
		{
			if ( g_NtOSKernel.dwBase && g_NtOSKernel.dwEnd )
			{
				UINT i = 0;
				DRIVER_LIST_ENTRY drvEntry;
				SSDTHOOK_LIST_ENTRY ssdtEntry;

				for (i = 0; i < KeServiceDescriptorTable.NumberOfServices; i++)
				{
					RtlZeroMemory( &drvEntry, sizeof(DRIVER_LIST_ENTRY) );
					RtlZeroMemory( &ssdtEntry, sizeof(SSDTHOOK_LIST_ENTRY) );

					if( IsAddressInAnyDriver(KeServiceDescriptorTable.ServiceTableBase[i], &drvEntry) )
					{
						if( !IsDrvNameKernelW( drvEntry.szDrvName ) )
						{
							ssdtEntry.unIndex = i;
							ssdtEntry.dwHookAddr = KeServiceDescriptorTable.ServiceTableBase[i];
							ssdtEntry.dwBase = drvEntry.dwBase;
							ssdtEntry.dwEnd = drvEntry.dwEnd;
							RtlStringCchCopyW( ssdtEntry.szDrvName, MAX_PATH, drvEntry.szDrvName );

							AddListEntry( SsdtList, &ssdtEntry, TRUE );
						}
					}
					else
					{
						ssdtEntry.unIndex = i;
						ssdtEntry.dwHookAddr = KeServiceDescriptorTable.ServiceTableBase[i];
						ssdtEntry.dwBase = 0;
						ssdtEntry.dwEnd = 0;
						RtlStringCchCopyW( ssdtEntry.szDrvName, MAX_PATH, UNKNOWN_MODULE );

						AddListEntry( SsdtList, &ssdtEntry, TRUE );
					}
				}
			}
			((PTHRPARAMS)pThrParam)->bResult = !IsMyListEmpty(SsdtList);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint( "Exception caught in ScanAndGetSSDTHooksCountThread()" );
	}
	PsTerminateSystemThread(STATUS_SUCCESS);
}

BOOLEAN FixSSDTHook( PARKFIXSSDT pFixSsdtHookData )
{
	BOOLEAN bResult = FALSE;
	__try
	{
		if( MmIsAddressValid( pFixSsdtHookData ) )
		{
			THRPARAMS stThrParams = {0};
			stThrParams.pParam = pFixSsdtHookData;
			stThrParams.dwParamLen = sizeof( PARKFIXSSDT );
			if( STATUS_SUCCESS == ThreadWrapper( FixSSDTHookThread, &stThrParams ) )
			{
				bResult = stThrParams.bResult;
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		bResult = FALSE;
		DbgPrint( "Exception caught in FixSSDTHook()" );
	}
	return bResult;
}

VOID FixSSDTHookThread( PVOID pThrParam )
{
	NTSTATUS retStatus;
	PTHRPARAMS pParams;
	PARKFIXSSDT pFixSsdtHookData;
	__try
	{
		if( MmIsAddressValid( pThrParam ) )
		{
			retStatus = STATUS_UNSUCCESSFUL;
			pParams = (PTHRPARAMS)pThrParam;
			pFixSsdtHookData = (PARKFIXSSDT)(pParams->pParam);

			__asm
			{
				push    eax
				mov     eax, CR0
				and     eax, 0FFFEFFFFh
				mov     CR0, eax
				pop     eax
			}
			if( g_mdlSSDT.pmdlSSDT && g_mdlSSDT.ppvMappedSSDT )
			{
				InterlockedExchange( (PLONG)&(g_mdlSSDT.ppvMappedSSDT)[pFixSsdtHookData->dwSsdtIndex],
					(LONG)(pFixSsdtHookData->dwOrigAddr) );
				pParams->bResult = TRUE;
			}
			__asm
			{
				push    eax
				mov     eax, CR0
				or      eax, NOT 0FFFEFFFFh
				mov     CR0, eax
				pop     eax
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		DbgPrint( "Exception caught in FixSSDTHookThread()" );
	}
	PsTerminateSystemThread( STATUS_SUCCESS );
}
