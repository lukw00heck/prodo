#include "ProdoMain.h"

extern OS_SPEC_DATA g_globalData;

UINT ProcessCount()
{
	UINT cntProcess = 0;

	__try
	{
		THRPARAMS ThrParams = {0};

		if ( STATUS_SUCCESS == ThreadWrapper(ProcessScanThread, &ThrParams) )
		{
			cntProcess = GetListCount(ProcList);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		cntProcess = 0;
		DbgPrint( "Exception caught in ScanAndGetProcessCount()" );
	}
	return cntProcess;
}

VOID ProcessScanThread(PVOID pThrParam)
{
	__try
	{
		if (MmIsAddressValid(pThrParam))
		{
			InitList(ProcList);

			ScanProcByPspCidTable();

			((PTHRPARAMS)pThrParam)->bResult = !IsMyListEmpty(ProcList);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint( "Exception caught in ProcessScanThread()" );
	}
	PsTerminateSystemThread( STATUS_SUCCESS );
}

/** Hard code search PspCidTable **/
NTSTATUS GetPspCidTable(PULONG pCidTable)
{
	PUCHAR puPtr;
	for (
		puPtr = (PUCHAR)PsLookupProcessByProcessId;
		puPtr < (PUCHAR)PsLookupProcessByProcessId + PAGE_SIZE;
		puPtr++)
	{
		if( (*(PUSHORT)puPtr == 0x35ff) && (*(puPtr + 6) == 0xe8) )
		{
			*pCidTable = *(PULONG)(puPtr + 2);
			return STATUS_SUCCESS;
		}
	}
	return STATUS_UNSUCCESSFUL;
}
ULONG BrowerTableL3(ULONG TableAddr)
{
    ULONG Object = 0;
    ULONG ItemCount = 511;
    DWORD dwProcessId = 0;
    ULONG flags;

	UCHAR *pszImageName = NULL;

    do{
        TableAddr += 8;
        Object = *(PULONG)TableAddr;
        Object = Object & 0xfffffff8;

        if(Object == 0)
        {
            continue;
        }
        if(!MmIsAddressValid((PVOID)Object))
        {
            continue;
        }
		dwProcessId = (DWORD)PsGetProcessId((PEPROCESS)Object);
		if(dwProcessId < 65536 && dwProcessId != 0)
		{
			pszImageName = PsGetProcessImageFileName((PEPROCESS)Object);
			DbgPrint("Process Cid: %d\t Name %s\n", dwProcessId, pszImageName);
		}
    }while (--ItemCount > 0);

    return 0;
}

ULONG BrowerTableL2(ULONG TableAddr)
{
    do{
        BrowerTableL3(*(PULONG)TableAddr);
        TableAddr += 4;
    }while((*(PULONG)TableAddr) != 0);

    return 0;
}

ULONG BrowerTableL1(ULONG TableAddr)
{
    do{
        BrowerTableL2(*(PULONG)TableAddr);
        TableAddr += 4;
    }while((*(PULONG)TableAddr) != 0);

    return 0;
}

/** Method 2 analyse/resolve handle table directly **/
VOID RefreshProcessByPspCidTable()
{
	ULONG PspCidTable = 0;
	ULONG HandleTable = 0;
	ULONG TableCode = 0;
	ULONG flag = 0;

	GetPspCidTable(&PspCidTable);
	HandleTable = *(PULONG)PspCidTable;

	TableCode = *(PULONG)HandleTable;
	flag = TableCode & 3;
	TableCode &= 0xfffffffc;

	switch (flag)
	{
	case 0:
		BrowerTableL3(TableCode);
		break;
	case 1:
		BrowerTableL2(TableCode);
		break;
	case 2:
		BrowerTableL1(TableCode);
		break;
	}
}
BOOLEAN IsProcess(PVOID Object)
{
	POBJECT_TYPE ObjectType = NULL;

	if (Object == NULL)
	{
		return FALSE;
	}
	if (!MmIsAddressValid(Object))
	{
		return FALSE;
	}

	ObjectType = ( (POBJECT_HEADER) ( OBJECT_TO_OBJECT_HEADER(Object) ) )->Type;

	if (!MmIsAddressValid(ObjectType) || ObjectType == NULL)
	{
		return FALSE;
	}

	if (ObjectType != *PsProcessType)
	{
		return FALSE;
	}

	return TRUE;
}
BOOLEAN EnumHandleCallback(PHANDLE_TABLE_ENTRY HandleTableEntry,
						   HANDLE Handle,
						   PVOID EnumParameter)
{
	PVOID Object = HandleTableEntry->Object;

	PEPROCESS pEproc = NULL;

	HANDLE hProcID = NULL;

	UCHAR *pszImageName = NULL;

	PROC_LIST_ENTRY ProcessListEntry = {0};

	DECLARE_UNICODE_STRING_SIZE(ustrImageName, MAX_PATH);
	DECLARE_UNICODE_STRING_SIZE(ustrImagePath, MAX_PATH);

	if (Object == NULL)
	{
		return TRUE;
	}

	if (IsProcess(Object))
	{
		pEproc = (PEPROCESS) Object;
		hProcID = PsGetProcessId(pEproc);
		pszImageName = PsGetProcessImageFileName((PEPROCESS)Object);

		// DbgPrint("Process Cid: %d\t Name %s\n", hProcID, pszImageName);
		if (IsProcessAlive(pEproc))
		{
			ProcessListEntry.dwPID = (DWORD)hProcID;

			GetProcessImageName(pEproc, &ustrImageName);
			GetProcessImagePath(pEproc, &ustrImagePath);
			/*
			RtlStringCchLengthA(pszImageName, MAX_PATH, &len);
			
			RtlMultiByteToUnicodeN(ProcessListEntry.ustrImageName,
				MAX_PATH,
				NULL,
				pszImageName,
				len);
				*/
			RtlStringCchCopyUnicodeString(ProcessListEntry.ustrImageName,
				MAX_PATH,
				&ustrImageName);
			RtlStringCchCopyUnicodeString(ProcessListEntry.ustrImagePath,
				MAX_PATH,
				&ustrImagePath);

			AddListEntry( ProcList, &ProcessListEntry, TRUE );
		}
	}
	return FALSE;
}

/** Method 1 analyse/resolve handle table by ExEnumHandleTable **/
NTSTATUS ScanProcByPspCidTable()
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;

	ULONG nPspCidTable = 0;

	ULONG HandleTable = 0;

	HANDLE Handle;

	HANDLE hParam;

	__try
	{
		NTSTATUS retStatus = GetPspCidTable(&nPspCidTable);

		HandleTable = *(PULONG)nPspCidTable;
		if ( !NT_SUCCESS(retStatus) )
		{
			DbgPrint( "Unable find PspCidTable.\n" );
			return retStatus;
		}
		ExEnumHandleTable((PHANDLE_TABLE)HandleTable, EnumHandleCallback, NULL, &Handle);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		retStatus = STATUS_UNSUCCESSFUL;
		DbgPrint( "Exception caught in ScanProcByPspCidTable()\n" );
	}
	return retStatus;
}

NTSTATUS ScanProcByHandleTable()
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;
	__try
	{
		ULONG nReturnLength = 0;

		PEPROCESS pEproc;
		PROC_LIST_ENTRY ProcessListEntry;

		HANDLE hProcID = NULL;

		PSYSTEM_PROCESS_INFORMATION pProcInfo = NULL;

		retStatus = ZwQuerySystemInformation(SystemProcessesAndThreadsInformation, NULL, 0, &nReturnLength);
		
		/**
		if (STATUS_BUFFER_OVERFLOW != retStatus &&
			STATUS_BUFFER_TOO_SMALL != retStatus &&
			STATUS_INFO_LENGTH_MISMATCH != retStatus)
		{
			return retStatus;
		}
		**/
		pProcInfo = (PSYSTEM_PROCESS_INFORMATION)ExAllocatePoolWithTag(PagedPool, nReturnLength, TAG_LIST);
		RtlZeroMemory(pProcInfo, nReturnLength);

		retStatus = ZwQuerySystemInformation(SystemProcessesAndThreadsInformation, 
			pProcInfo, nReturnLength, &nReturnLength);

		if ( !NT_SUCCESS(retStatus) )
		{
			DbgPrint( "ZwQuerySystemInformation Error: 0x%X\n", retStatus );
			ExFreePool(pProcInfo);
			pProcInfo = NULL;
			return retStatus;
		}

		for (;;)
		{
			if (pProcInfo->NextEntryOffset == 0)
			{
				break;
			}
			hProcID = pProcInfo->UniqueProcessId;

			retStatus = PsLookupProcessByProcessId( hProcID, &pEproc );
			if ( NT_SUCCESS(retStatus) )
			{
				if (IsProcessAlive(pEproc))
				{
					ProcessListEntry.dwPID = (DWORD)hProcID;
					
					// GetProcessImageName(hProcID, &ProcessListEntry.ustrImageName);

					retStatus = AddListEntry( ProcList, &ProcessListEntry, TRUE );
				}
				ObDereferenceObject(pEproc);
			}
			else
			{
				DbgPrint( "GetProcByHandleTableScan: PsLookupProcessByProcessId failed: 0x%x, pid: %ld\n",
					retStatus, pProcInfo->UniqueProcessId );
			}
			pProcInfo = (PSYSTEM_PROCESS_INFORMATION)(((PUCHAR)pProcInfo) + pProcInfo->NextEntryOffset);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		retStatus = STATUS_UNSUCCESSFUL;
		DbgPrint( "Exception caught in GetProcByHandleTable()" );
	}
	return retStatus;
}

NTSTATUS ScanProcByPID()
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;

	UINT uPID;
	DWORD dwPIDindex = 0;
	DWORD dwBPID = 0;
	PEPROCESS pEproc = NULL;
	PROC_LIST_ENTRY ProcessListEntry;

	for ( dwPIDindex = 0; dwPIDindex < NT_PROCESS_LIMIT; dwPIDindex+=4 )
	{
		pEproc = NULL;
		if( STATUS_SUCCESS == PsLookupProcessByProcessId((HANDLE)dwPIDindex, &pEproc) )
		{
			if( IsProcessAlive( pEproc ) )
			{
				uPID = (UINT) PsGetProcessId(pEproc);
				if( dwBPID != (DWORD) uPID )
				{
					RtlZeroMemory( &ProcessListEntry, sizeof( PROC_LIST_ENTRY ) );

					ProcessListEntry.dwPID = dwBPID;
					// GetProcessImageName( (HANDLE) dwPIDindex, &ProcessListEntry.ustrImageName );
					retStatus = AddListEntry( ProcList, &ProcessListEntry, TRUE );
					dwBPID = dwPIDindex;
				}
			}
			ObDereferenceObject( pEproc );
		}
	}

	return retStatus;
}

UINT ScanAndGetDllCount( DWORD dwPid )
{
	UINT cntDlls = 0;

	DbgPrint( "Execute ScanAndGetDllCount.\n" );
	__try
	{
		THRPARAMS stThrParams = {0};
		stThrParams.pParam = &dwPid;
		stThrParams.dwParamLen = sizeof( DWORD );
		if( STATUS_SUCCESS == ThreadWrapper( ScanAndGetDllCountThread, &stThrParams ) )
		{
			cntDlls = GetListCount( DllList );
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		cntDlls = 0;
		DbgPrint( "Exception caught in ScanAndGetDllCount()" );
	}
	return cntDlls;
}

VOID ScanAndGetDllCountThread( PVOID pThrParam )
{
	DbgPrint( "Execute ScanAndGetDllCountThread.\n" );
	__try
	{
		if( MmIsAddressValid( pThrParam ) && ( STATUS_SUCCESS == InitList( DllList ) ) )
		{
			DWORD dwPid = *(PDWORD)((PTHRPARAMS)pThrParam)->pParam;

			GetDllByVadTree( dwPid );

			((PTHRPARAMS)pThrParam)->bResult = !IsMyListEmpty( DllList );
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		DbgPrint( "Exception caught in ScanAndGetDllCountThread()" );
	}
	PsTerminateSystemThread( STATUS_SUCCESS );
}

NTSTATUS GetDllByVadTree( DWORD dwPid )
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;

	DbgPrint( "Execute GetDllByVadTree.\n" );
	__try
	{
		PEPROCESS pEproc = NULL;

		retStatus = PsLookupProcessByProcessId( (HANDLE)dwPid, &pEproc );
		if( STATUS_SUCCESS == retStatus )
		{
			if( IsProcessAlive( pEproc ) )
			{
				if( MmIsAddressValid( (PBYTE)pEproc + g_globalData.dwVadRootOffset ) )
				{
					PMMVAD pVadRoot = NULL;

					pVadRoot = (PMMVAD) *(PULONG)( (PBYTE)pEproc + g_globalData.dwVadRootOffset );
					TraverseVadTreeInOrder( pVadRoot );
				}
			}
			ObDereferenceObject( pEproc );
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		retStatus = STATUS_UNSUCCESSFUL;
		DbgPrint( "Exception caught in GetDllByVadTree()" );
	}
	return retStatus;
}

VOID
	PushMmvadEntry(PSINGLE_LIST_ENTRY ListHead, PMMVAD_LIST_ENTRY Entry)
{
	PushEntryList(ListHead, &(Entry->lEntry));
}
PMMVAD_LIST_ENTRY
	PopMmvadEntry(PSINGLE_LIST_ENTRY ListHead)
{
	PSINGLE_LIST_ENTRY SingleListEntry;
	SingleListEntry = PopEntryList(ListHead);
	return CONTAINING_RECORD(SingleListEntry, MMVAD_LIST_ENTRY, lEntry);
}
VOID ParseMmvad( PMMVAD pVadNode )
{
	DECLARE_UNICODE_STRING_SIZE(ustrFullPath, MAX_PATH);

	WCHAR buf[MAX_PATH] = {0};

	if( MmIsAddressValid( pVadNode ) )
	{
		if( MmIsAddressValid( pVadNode->ControlArea ) &&
			MmIsAddressValid( pVadNode->ControlArea->FilePointer ) &&
			pVadNode->ControlArea->FilePointer->FileName.Length )
		{
			ULONG StartingVpn = 0;
			ULONG EndingVpn = 0;
			DLL_LIST_ENTRY dllEntry = {0};

			StartingVpn = pVadNode->StartingVpn << 12;
			EndingVpn = ( ( pVadNode->EndingVpn + 1 ) << 12 ) - 1;

			RtlZeroMemory( &dllEntry, sizeof( DLL_LIST_ENTRY ) );
			dllEntry.dwBase = StartingVpn;

			RtlStringCchCopyUnicodeString(buf, MAX_PATH, &pVadNode->ControlArea->FilePointer->FileName);

			if ( !wcsstr(buf, L".dll") )
			{
				return;
			}
			GetFullPathOfFileObject(pVadNode->ControlArea->FilePointer, &ustrFullPath);

			RtlStringCchCopyUnicodeString(dllEntry.szDllName, MAX_PATH, &ustrFullPath);

			AddListEntry( DllList, &dllEntry, TRUE );
		}
	}
}

/** non-recursion **/
VOID TraverseVadTreeInOrder( PMMVAD pVadNode )
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;

	PMMVAD_LIST_ENTRY pMmvadListEntry;
	PMMVAD pTempVad = pVadNode;

	SINGLE_LIST_ENTRY ListHead = {0};

	DbgPrint( "Execute TraverseVadTreeInOrder.\n" );
	__try
	{
		while ( MmIsAddressValid(pTempVad) || ListHead.Next )
		{
			while( MmIsAddressValid(pTempVad) )
			{
				ParseMmvad(pTempVad);

				pMmvadListEntry = ExAllocatePoolWithTag(PagedPool, sizeof(MMVAD_LIST_ENTRY), TAG_LIST);

				pMmvadListEntry->pVad = pTempVad;
				
				PushMmvadEntry(&ListHead, pMmvadListEntry);

				pTempVad = pTempVad->LeftChild;
			}
			if (ListHead.Next)
			{
				pMmvadListEntry = PopMmvadEntry(&ListHead);

				pTempVad = pMmvadListEntry->pVad->RightChild;

				ExFreePool(pMmvadListEntry);

				pMmvadListEntry = NULL;
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		DbgPrint( "Exception caught in TraverseVadTreeInOrder()" );
	}
}

BOOLEAN KillProcess( PDWORD pdwPid )
{
	BOOLEAN bResult = FALSE;

	DbgPrint("Execute KillProcess.\n");

	__try
	{
		if( MmIsAddressValid( pdwPid ) )
		{
			THRPARAMS stThrParams = {0};
			stThrParams.pParam = pdwPid;
			stThrParams.dwParamLen = sizeof( PDWORD );
			if( STATUS_SUCCESS == ThreadWrapper( KillProcessThread, &stThrParams ) )
			{
				bResult = stThrParams.bResult;
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		bResult = FALSE;
		DbgPrint( "Exception caught in KillProcess()" );
	}
	return bResult;
}

VOID KillProcessThread( PVOID pThrParam )
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;

	PEPROCESS pEproc;

	DbgPrint("Execute KillProcessThread.\n");
	__try
	{
		if( MmIsAddressValid( pThrParam ) )
		{
			PTHRPARAMS pParams = (PTHRPARAMS)pThrParam;
			DWORD dwPid = *(PDWORD)(pParams->pParam);

			retStatus = KillProcessByInsertApc( dwPid );

			if( NT_SUCCESS(retStatus) )
			{
				DbgPrint("Execute KillProcessByInsertApc Success.\n");
				pParams->bResult = TRUE;
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		DbgPrint( "Exception caught in KillProcessThread()" );
	}
	PsTerminateSystemThread( STATUS_SUCCESS );
}

VOID TerminateThreadKernelRoutine(IN PKAPC Apc,
							  IN OUT PKNORMAL_ROUTINE *NormalRoutine,
							  IN OUT PVOID *NormalContext,
							  IN OUT PVOID *SystemArgument1,
							  IN OUT PVOID *SystemArgument2)
{
	PETHREAD pCurrThr = NULL;
	PULONG pThreadFlags = NULL;
	ExFreePool(Apc);

	pCurrThr = PsGetCurrentThread();
	pThreadFlags = (PULONG) pCurrThr + g_globalData.dwCrossThreadFlags;

	if ( MmIsAddressValid(pThreadFlags) )
	{
		*pThreadFlags = (*pThreadFlags) | PS_CROSS_THREAD_FLAGS_SYSTEM;
		PsTerminateSystemThread(STATUS_SUCCESS);
	}
}
NTSTATUS KillProcessByInsertApc(DWORD dwPid)
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;

	PKAPC pTermThrApc = NULL;

	PEPROCESS pEproc;

	PETHREAD pEthr;

	ULONG cntThr;

	ULONG nIndex;

	PLIST_ENTRY plEntry;

	DbgPrint("Execute KillProcessByInsertApc.\n");
	__try
	{
		retStatus = PsLookupProcessByProcessId( (HANDLE)dwPid, &pEproc );

		if( NT_ERROR(retStatus) )
		{
			DbgPrint("KillProcessThread PsLookupProcessByProcessId failed\n");
			return STATUS_UNSUCCESSFUL;
		}
		cntThr = *((PULONG) pEproc + g_globalData.dwActiveThreads);

		plEntry = (PLIST_ENTRY)( (PULONG) pEproc + g_globalData.dwThreadListHead );

		for ( nIndex = 0; nIndex < cntThr; nIndex++ )
		{
			plEntry = plEntry->Flink;

			pEthr = (PETHREAD)( (PLONG) plEntry - g_globalData.dwThreadListEntry );

			// DbgPrint("_ETHREAD: 0x%X\n", pEthr);

			pTermThrApc = (PKAPC) ExAllocatePoolWithTag(PagedPool, sizeof(KAPC), TAG_KAPC);

			if( !MmIsAddressValid(pTermThrApc) )
			{
				DbgPrint("ExAllocatePoolWithTag failed.\n");
				return STATUS_UNSUCCESSFUL;
			}
			KeInitializeApc(
				pTermThrApc,
				pEthr,
				OriginalApcEnvironment,
				TerminateThreadKernelRoutine,
				NULL,
				NULL,
				KernelMode,
				NULL);

			retStatus = KeInsertQueueApc(pTermThrApc, pTermThrApc, NULL, 2);

			if( NT_ERROR(retStatus) )
			{
				DbgPrint("KeInsertQueueApc failed.\n");
				ExFreePool(pTermThrApc);
				return retStatus;
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("Exception caught in KillProcessByInsertApc.\n");
		retStatus = STATUS_UNSUCCESSFUL;
	}
	return retStatus;
}
