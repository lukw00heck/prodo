#include "Commons.h"

ARKNTAPI g_NtApiData = {0};

OS_SPEC_DATA g_globalData = {0};

NTOSKRNL_DATA g_NtOSKernel = {0};

PDRIVER_OBJECT g_pCurrDrvObj = NULL;

SSDT_MDL g_mdlSSDT = {0};

NTSTATUS InitGlobals()
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;

	__try
	{
		THRPARAMS stThrParams = {0};

		retStatus = ThreadWrapper( InitGlobalsThread, &stThrParams );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		retStatus = STATUS_UNSUCCESSFUL;
		DbgPrint( "Exception caught in InitGlobals()" );
	}
	return retStatus;
}

VOID InitGlobalsThread( PVOID pThrParam )
{
	__try
	{
		g_globalData.dwFlinkOffset = 0x088;
		g_globalData.dwPidOffset = 0x084;
		g_globalData.dwHandleTableOffset = 0xc4;
		g_globalData.dwHandleListOffset = 0x1c;
		g_globalData.dwEprocOffset = 0x08;
		g_globalData.dwThreadsProcess = 0x088;    // 0x220 byte offset
		g_globalData.dwCID = 0x7b;                // 0x1ec byte offset
		g_globalData.dwImageFilename = 0x174;
		g_globalData.dwCrossThreadFlags = 0x92;   // 0x248 byte offset
		g_globalData.dwSeAuditOffset = 0x1f4;
		g_globalData.dwProcessFlagsOffset = 0x248;
		g_globalData.dwCmKeyHiveOffset = 0x010;
		g_globalData.dwCmKeyCellOffset = 0x014;
		g_globalData.dwCmNodeNameOffset = 0x04c;
		g_globalData.dwCmNodeNameLenOffset = 0x048;
		g_globalData.dwCmKcbLastWriteTime = 0x038;
		g_globalData.dwModEntryOffset = 0;
		g_globalData.dwVadRootOffset = 0x11c;
		g_globalData.dwPebOffset = 0x7ffdf000;
		g_globalData.dwActiveThreads = 0x68;      // nt!_EPROCESS 0x1A0
		g_globalData.dwThreadListHead = 0x64;     // nt!_EPROCESS 0x190
		g_globalData.dwThreadListEntry = 0x8B;    // nt!_ETHREAD  0x22C
		InitSsdtMdl();

		((PTHRPARAMS) pThrParam)->bResult = TRUE;
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		DbgPrint( "Exception caught in InitGlobalsThread()" );
	}
	PsTerminateSystemThread( STATUS_SUCCESS );
}

NTSTATUS InitSsdtMdl()
{
	NTSTATUS retVal = STATUS_UNSUCCESSFUL;
	__try
	{
		g_mdlSSDT.pmdlSSDT = MmCreateMdl( NULL,
			KeServiceDescriptorTable.ServiceTableBase,
			KeServiceDescriptorTable.NumberOfServices * 4 );
		MmBuildMdlForNonPagedPool( g_mdlSSDT.pmdlSSDT );
		g_mdlSSDT.pmdlSSDT->MdlFlags = g_mdlSSDT.pmdlSSDT->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
		g_mdlSSDT.ppvMappedSSDT = MmMapLockedPages( g_mdlSSDT.pmdlSSDT, KernelMode );

		if( g_mdlSSDT.pmdlSSDT && g_mdlSSDT.ppvMappedSSDT )
		{
			retVal = STATUS_SUCCESS;
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		retVal = STATUS_UNSUCCESSFUL;
		DbgPrint( "Exception caught in InitSsdtMdl()" );
	}
	return retVal;
}

NTSTATUS ThreadWrapper( PVOID pfnStartRoutine, PTHRPARAMS pThrParam )
{
	NTSTATUS retStatus;
	HANDLE hThread = NULL;

	__try
	{
		if( MmIsAddressValid( pfnStartRoutine ) && MmIsAddressValid( pThrParam ) )
		{
			retStatus = PsCreateSystemThread(
				&hThread,
				0,
				NULL, NULL, NULL,
				pfnStartRoutine,
				pThrParam);

			if ( NT_SUCCESS(retStatus) )
			{
				retStatus = ZwWaitForSingleObject(hThread, FALSE, NULL);

				if ( NT_SUCCESS(retStatus) )
				{
					retStatus = ( FALSE != pThrParam->bResult ) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
				}
				else
				{
					DbgPrint( "ThreadWrapper: ZwWaitForSingleObject failed: 0x%x", retStatus );
				}
				ZwClose(hThread);
			}
			else
			{
				DbgPrint( "ThreadWrapper: PsCreateSystemThread failed: 0x%x", retStatus );
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		retStatus = STATUS_UNSUCCESSFUL;
		DbgPrint( "Exception caught in ThreadSpooler()" );
	}
	return retStatus;
}

BOOLEAN IsAddressInAnyDriver(DWORD dwAddress, PDRIVER_LIST_ENTRY pDrv)
{
	BOOLEAN bAddrInDriver = FALSE;
	__try
	{
		if( dwAddress && MmIsAddressValid(pDrv) )
		{
			UINT i = 0;

			PDRIVER_LIST_ENTRY pCurrent = NULL;

			while( STATUS_SUCCESS == GetListEntry(DriverList, i, &pCurrent) )
			{
				if( MmIsAddressValid( pCurrent ) )
				{
					if( ( pCurrent->dwBase <= dwAddress ) && ( pCurrent->dwEnd >= dwAddress ) )
					{
						pDrv->dwBase = pCurrent->dwBase;
						pDrv->dwEnd = pCurrent->dwEnd;
						RtlStringCchCopyW( pDrv->szDrvName, MAX_PATH, pCurrent->szDrvName );
						bAddrInDriver = TRUE;
						break;
					}
					++i;
				}
				else
				{
					break;
				}
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{        
		bAddrInDriver = FALSE;
		DbgPrint( "Exception caught in IsAddressInAnyDriver()" );
	}    
	return bAddrInDriver;
}

BOOLEAN IsDrvNameKernelA( CHAR *pszDrvName )
{
	BOOLEAN bIsKernel = FALSE;
	__try
	{
		if( MmIsAddressValid( pszDrvName ) )
		{
			if( ( NULL != strstr( pszDrvName, "ntoskrnl.exe" ) ) ||
				( NULL != strstr( pszDrvName, "ntkrnlpa.exe" ) ) ||
				( NULL != strstr( pszDrvName, "ntkrnlmp.exe" ) ) )
			{
				bIsKernel = TRUE;
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		bIsKernel = FALSE;
		DbgPrint( "Exception caught in IsDrvNameKernelA()" );
	}
	return bIsKernel;
}
BOOLEAN IsDrvNameKernelW( WCHAR *pwszDrvName )
{
	BOOLEAN bIsKernel = FALSE;
	__try
	{
		if( MmIsAddressValid( pwszDrvName ) )
		{
			if( ( NULL != wcsstr( pwszDrvName, L"ntoskrnl.exe" ) ) ||
				( NULL != wcsstr( pwszDrvName, L"ntkrnlpa.exe" ) ) ||
				( NULL != wcsstr( pwszDrvName, L"ntkrnlmp.exe" ) ) )
			{
				bIsKernel = TRUE;
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		bIsKernel = FALSE;
		DbgPrint( "Exception caught in IsDrvNameKernelW()" );
	}
	return bIsKernel;
}
NTSTATUS GetProcessImageName(PEPROCESS pEproc, PUNICODE_STRING pustrImageName)
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;

	PVOID pBuffer = NULL;

	ULONG nRetLength = 0;

	HANDLE hOpened;

	__try
	{
		retStatus = ObOpenObjectByPointer(pEproc,
			OBJ_KERNEL_HANDLE,
			NULL,
			GENERIC_READ,
			*PsProcessType,
			KernelMode,
			&hOpened);

		if (!NT_SUCCESS(retStatus))
		{
			DbgPrint( "ObOpenObjectByPointer() Fail - EPROCESS 0x%X \t\t 0x%X\n", pEproc, retStatus);
			return retStatus;
		}

		retStatus = ZwQueryInformationProcess(hOpened, ProcessImageFileName, NULL, 0, &nRetLength);
		
		if (STATUS_INFO_LENGTH_MISMATCH != retStatus)
		{
			DbgPrint( "ZwQueryInformationProcess() Fail - EPROCESS 0x%X \t\t 0x%X\n", pEproc, retStatus);
			return retStatus;
		}

		pBuffer = ExAllocatePoolWithTag(PagedPool, nRetLength, 'RTSU');
		if ( !MmIsAddressValid(pBuffer) )
		{
			DbgPrint( "ExAllocatePoolWithTag() Fail\n");
			return retStatus;
		}
		retStatus = ZwQueryInformationProcess(hOpened, ProcessImageFileName, pBuffer, nRetLength, &nRetLength);

		if ( !NT_SUCCESS(retStatus) )
		{
			DbgPrint( "ZwQueryInformationProcess() Fail - EPROCESS 0x%X \t\t 0x%X\n", pEproc, retStatus);
			return retStatus;
		}
		RtlCopyUnicodeString(pustrImageName, (PUNICODE_STRING)pBuffer);

		ZwClose(hOpened);

		ExFreePool(pBuffer);
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		retStatus = STATUS_UNSUCCESSFUL;
		DbgPrint( "Exception caught in GetProcessImageName() - EPROCESS %0x%X\n", pEproc );
	}
	return retStatus;
}
/*
NTSTATUS GetProcessImageName(PEPROCESS pEproc, PUNICODE_STRING pustrImageName)
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;
	PSE_AUDIT_PROCESS_CREATION_INFO pseAudit = NULL;
	__try
	{
		pseAudit = (PSE_AUDIT_PROCESS_CREATION_INFO)( (BYTE *)pEproc + g_globalData.dwSeAuditOffset );
		DbgPrint("GetProcessImageName: %ws\n", pseAudit->ImageFileName->Name.Buffer);
		if( MmIsAddressValid( pseAudit ) && pseAudit->ImageFileName->Name.Length )
		{
			RtlCopyUnicodeString(pustrImageName, &pseAudit->ImageFileName->Name);
			retStatus = STATUS_SUCCESS;
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		retStatus = STATUS_UNSUCCESSFUL;
		DbgPrint( "Exception caught in GetProcessImageName() - PEPROCESS 0x%X", pEproc );
	}
	return retStatus;
}
*/
NTSTATUS GetProcessImagePath(PEPROCESS pEproc, PUNICODE_STRING  pustrPath)
{
	HANDLE               hFile      = NULL;
	ULONG                nNeedSize	= 0;
	NTSTATUS             nStatus    = STATUS_SUCCESS;
	NTSTATUS             nDeviceStatus = STATUS_DEVICE_DOES_NOT_EXIST;

	KAPC_STATE           ApcState   = {0};			
	PVOID                lpBuffer   = NULL;
	OBJECT_ATTRIBUTES	 ObjectAttributes = {0};
	IO_STATUS_BLOCK      IoStatus   = {0}; 
	PFILE_OBJECT         FileObject = NULL;
	PFILE_NAME_INFORMATION FileName = NULL;   
	WCHAR                FileBuffer[MAX_PATH] = {0};
	DECLARE_UNICODE_STRING_SIZE(ProcessPath,MAX_PATH);
	DECLARE_UNICODE_STRING_SIZE(DosDeviceName,MAX_PATH);

	PAGED_CODE();

	__try
	{
		KeStackAttachProcess(pEproc, &ApcState);

		nStatus = ZwQueryInformationProcess(
			NtCurrentProcess(),
			ProcessImageFileName,
			NULL,
			0,
			&nNeedSize
			);

		if (STATUS_INFO_LENGTH_MISMATCH != nStatus)
		{
			KdPrint(("%s NtQueryInformationProcess error.\n", __FUNCTION__)); 
			nStatus = STATUS_MEMORY_NOT_ALLOCATED;
			__leave;
		}
		lpBuffer = ExAllocatePoolWithTag(PagedPool, nNeedSize, TAG_LIST);

		if (lpBuffer == NULL)
		{
			KdPrint(("%s ExAllocatePoolWithTag error.\n", __FUNCTION__));
			nStatus = STATUS_MEMORY_NOT_ALLOCATED;
			__leave; 
		}
		nStatus =  ZwQueryInformationProcess(
			NtCurrentProcess(),
			ProcessImageFileName, 
			lpBuffer, 
			nNeedSize,
			&nNeedSize
			);

		if (NT_ERROR(nStatus))
		{
			KdPrint(("%s NtQueryInformationProcess error 2.\n", __FUNCTION__));
			__leave;
		}
		RtlCopyUnicodeString(&ProcessPath, (PUNICODE_STRING)lpBuffer);

		InitializeObjectAttributes(
			&ObjectAttributes,
			&ProcessPath,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);

		nStatus = ZwCreateFile(
			&hFile,
			FILE_READ_ATTRIBUTES,
			&ObjectAttributes,
			&IoStatus,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OPEN,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
			NULL,
			0
			);  

		if (NT_ERROR(nStatus))
		{
			hFile = NULL;
			__leave;
		}

		nStatus = ObReferenceObjectByHandle(
			hFile, 
			FILE_READ_ATTRIBUTES,
			*IoFileObjectType, 
			KernelMode, 
			(PVOID*)&FileObject,
			NULL
			);

		if (NT_ERROR(nStatus))
		{
			FileObject = NULL;
			__leave;
		}

		FileName = (PFILE_NAME_INFORMATION)FileBuffer;

		nStatus = ZwQueryInformationFile(
			hFile,
			&IoStatus,
			FileName,
			sizeof(WCHAR)*MAX_PATH,
			FileNameInformation
			);

		if (NT_ERROR(nStatus))
		{
			__leave;
		}

		if (FileObject->DeviceObject == NULL)
		{
			nDeviceStatus = STATUS_DEVICE_DOES_NOT_EXIST;
			__leave;
		}

		nDeviceStatus = RtlVolumeDeviceToDosName(FileObject->DeviceObject, &DosDeviceName);
	}
	__finally
	{
		if (NULL != FileObject)
		{
			ObDereferenceObject(FileObject);
		}

		if (NULL != hFile)
		{
			ZwClose(hFile);
		}

		if (NULL != lpBuffer)
		{
			ExFreePool(lpBuffer);
		}

		KeUnstackDetachProcess(&ApcState);
	}

	if (NT_SUCCESS(nStatus))
	{
		RtlInitUnicodeString(&ProcessPath, FileName->FileName);

		if (NT_SUCCESS(nDeviceStatus))
		{
			RtlCopyUnicodeString(pustrPath, &DosDeviceName);

			RtlUnicodeStringCat(pustrPath, &ProcessPath);
		}
		else
		{
			RtlCopyUnicodeString(pustrPath, &ProcessPath);
		}
	}

	return nStatus;
}

NTSTATUS GetFullPathOfFileObject(PFILE_OBJECT FileObject, PUNICODE_STRING  pustrPath)
{
	NTSTATUS nStatus = STATUS_SUCCESS;
	NTSTATUS nDeviceStatus = STATUS_DEVICE_DOES_NOT_EXIST;

	DECLARE_UNICODE_STRING_SIZE(FullPath,MAX_PATH);
	DECLARE_UNICODE_STRING_SIZE(DosDeviceName,MAX_PATH);

	PAGED_CODE();
	__try
	{
		if (NULL != FileObject)
		{
			nDeviceStatus = RtlVolumeDeviceToDosName(FileObject->DeviceObject, &DosDeviceName);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("Execute RtlVolumeDeviceToDosName Error 0x%X.\n", nDeviceStatus);
	}
	if (NT_SUCCESS(nStatus))
	{
		RtlCopyUnicodeString(&FullPath, &FileObject->FileName);

		if (NT_SUCCESS(nDeviceStatus))
		{
			RtlCopyUnicodeString(pustrPath, &DosDeviceName);

			RtlUnicodeStringCat(pustrPath, &FullPath);
		}
		else
		{
			RtlCopyUnicodeString(pustrPath, &FullPath);
		}
	}
	return nStatus;
}

BOOLEAN IsProcessAlive( PEPROCESS pEproc )
{
	BOOLEAN bIsAlive = FALSE;
	__try
	{
		if( MmIsAddressValid( pEproc ) &&
			MmIsAddressValid( (UINT*)((BYTE*)pEproc + g_globalData.dwProcessFlagsOffset) ) )
		{
			if( !( (*(UINT*)( (BYTE*)pEproc + g_globalData.dwProcessFlagsOffset )) & PROCESS_STATE_EXITING ) &&
				!( (*(UINT*)( (BYTE*)pEproc + g_globalData.dwProcessFlagsOffset )) & PROCESS_STATE_DELETED ) )
			{
				bIsAlive = TRUE;
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		bIsAlive = FALSE;
		DbgPrint( "Exception caught in IsProcessAlive()" );
	}
	return bIsAlive;
}
BOOLEAN IsEthreadValid( PETHREAD pEthread )
{
	BOOLEAN bValid = FALSE;
	__try
	{
		if( MmIsAddressValid( pEthread ) )
		{
			if( MmIsAddressValid( ( (DWORD*)pEthread + g_globalData.dwCID ) ) && 
				MmIsAddressValid( ( (DWORD*)pEthread + g_globalData.dwCID + 0x01 ) ) && 
				MmIsAddressValid( ( (DWORD*)pEthread + g_globalData.dwCrossThreadFlags ) ) )
			{
				bValid = TRUE;
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		bValid = FALSE;
		DbgPrint( "Exception caught in IsEthreadValid()" );
	}
	return bValid;
}
BOOLEAN IsThreadAlive( PETHREAD pEthread )
{
	BOOLEAN bIsAlive = FALSE;
	__try
	{
		if( IsEthreadValid( pEthread ) )
		{
			if( !(*((DWORD*)pEthread + g_globalData.dwCrossThreadFlags) & THREAD_TERMINATED ) && // If thread is not terminated
				!(*((DWORD*)pEthread + g_globalData.dwCrossThreadFlags) & THREAD_DEAD ) ) // and not dead
			{
				bIsAlive = TRUE;
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		bIsAlive = FALSE;
		DbgPrint( "Exception caught in IsThreadAlive()" );
	}
	return bIsAlive;
}
BOOLEAN IsDummyModuleEntry( PLDR_MODULE pModuleToChk )
{
	BOOLEAN bDummy = FALSE;
	__try
	{
		if( MmIsAddressValid( pModuleToChk ) )
		{
			if( ( 0 == pModuleToChk->FullDllName.Length ) ||
				( 0 == pModuleToChk->SizeOfImage ) ||
				( 0 == pModuleToChk->BaseAddress ) )
			{
				bDummy = TRUE;
			}
		}
		else
		{
			bDummy = TRUE;
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		bDummy = TRUE;
		DbgPrint( "Exception caught in IsDummyModuleEntry()" );
	}
	return bDummy;
}
PDWORD GetPsLoadedModuleList()
{
	PDWORD pPsLoadedModuleList = NULL;
	__try
	{
		if( MmIsAddressValid( g_pCurrDrvObj ) )
		{
			pPsLoadedModuleList = (DWORD *)(g_pCurrDrvObj->DriverSection);
		}
		else
		{
			// If our driver object is invalid, then get it from KdVersionBlock
			PDWORD pKdVersionBlock = NULL;
			KeSetSystemAffinityThread( 1 );
			__asm
			{
				mov eax, fs:[0x1c]; // SelfPCR
				mov eax, fs:[0x34]; // Get address of KdVersionBlock
				mov pKdVersionBlock, eax;
				mov eax, [ eax + 0x18 ]; // Get address of PsLoadedModuleList
				mov pPsLoadedModuleList, eax;
			}
			KeRevertToUserAffinityThread();

			DbgPrint( "GetPsLoadedModuleList: KdVersionBlock 0x%x, PsLoadedModuleList 0x%x", pKdVersionBlock, pPsLoadedModuleList );
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		pPsLoadedModuleList = NULL;
		DbgPrint( "Exception caught in GetPsLoadedModuleList()" );
	}
	return pPsLoadedModuleList;
}
