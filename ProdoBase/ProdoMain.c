#include "ProdoMain.h"

extern PDRIVER_OBJECT g_pCurrDrvObj;

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	UNICODE_STRING 		uDeviceName = {0};
	UNICODE_STRING 		uLinkName = {0};
	NTSTATUS 			ntStatus = STATUS_SUCCESS;
	PDEVICE_OBJECT		pDeviceObject = NULL;

	DbgPrint("Driver load begin.\n");

	pDriverObject->DriverUnload = DriverUnload;

	RtlInitUnicodeString(&uDeviceName, DEVICE_NAME);
	RtlInitUnicodeString(&uLinkName, LINK_NAME);

	ntStatus = IoCreateDevice(
		pDriverObject,
		0,
		&uDeviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&pDeviceObject);

	if ( !NT_SUCCESS(ntStatus) )
	{
		DbgPrint("IoCreateDevice failed: %x\n", ntStatus);
		return ntStatus;
	}
	pDeviceObject-> Flags |= DO_BUFFERED_IO;

	ntStatus = IoCreateSymbolicLink(&uLinkName, &uDeviceName);

	if( !NT_SUCCESS(ntStatus) )
	{
		DbgPrint("IoCreateSymbolicLink failed: %x\n",ntStatus);
		IoDeleteDevice(pDeviceObject);
		return ntStatus;
	}

	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE]  = DispatchClose;

	InitGlobals();

	g_pCurrDrvObj = pDriverObject;

	DbgPrint("Driver loaded.\n");

	return STATUS_SUCCESS;
}

VOID DriverUnload(struct _DRIVER_OBJECT *pDriverObject)
{
	UNICODE_STRING uLinkName = {0};   

	DbgPrint("Driver Unload.\n");

	RtlInitUnicodeString(&uLinkName, LINK_NAME);
	IoDeleteSymbolicLink(&uLinkName);
	IoDeleteDevice(pDriverObject->DeviceObject);

	return;
}

NTSTATUS DispatchIoctl (__in struct _DEVICE_OBJECT *DeviceObject, __inout struct _IRP *pIrp)
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;

	ULONG nInputLen = 0;
	ULONG nOutputLen = 0;

	ULONG nIoCtlCode = 0;

	ULONG nIndex = 0;

	PIO_STACK_LOCATION pStack = NULL;

	pStack = IoGetCurrentIrpStackLocation(pIrp);

	nInputLen = pStack->Parameters.DeviceIoControl.InputBufferLength;
	nOutputLen = pStack->Parameters.DeviceIoControl.OutputBufferLength;
	nIoCtlCode = pStack->Parameters.DeviceIoControl.IoControlCode;

	switch(nIoCtlCode)
	{
	case IOCTL_GET_DATA_CNT:
		{
			PARK_DATA_COUNT pArkDataCount = pIrp->AssociatedIrp.SystemBuffer;
			if( MmIsAddressValid( pArkDataCount ) )
			{
				switch( pArkDataCount->DataType )
				{
				case ArkDataProcList:
					{
						pArkDataCount->nDataCount = ProcessCount();
					}
					break;

				case ArkDataDllList:
					{

						pArkDataCount->nDataCount = ScanAndGetDllCount( pArkDataCount->dwMixData );
					}
					break;

				case ArkDataDriverList:
					{
						pArkDataCount->nDataCount = ScanAndGetDriverCount();
					}
					break;

				case ArkDataSsdtList:
					{
						pArkDataCount->nDataCount = SSDThooksCount();
					}
					break;

				default:
					{
						pArkDataCount->nDataCount = 0;
					}
					break;
				}
				if( pArkDataCount->nDataCount > 0 )
				{
					retStatus = STATUS_SUCCESS;
					pIrp->IoStatus.Information = sizeof( ARK_DATA_COUNT );
				}
			}
		}
		break;

	case IOCTL_GET_PROCESS:
		{
			PARK_PROCESS pArkProcData = pIrp->AssociatedIrp.SystemBuffer;
			if( MmIsAddressValid( pArkProcData ) && VALIDATE_LIST_BUFF_SIZE( nOutputLen, ARK_PROCESS, ProcList ) )
			{
				PPROC_LIST_ENTRY pProcListEntry = NULL;
				while( STATUS_SUCCESS == GetListEntry( ProcList, nIndex, &pProcListEntry ) )
				{
					if( MmIsAddressValid( pProcListEntry ) )
					{
						pArkProcData[nIndex].dwProcId = pProcListEntry->dwPID;

						RtlStringCchCopyW(pArkProcData[nIndex].pwszImageName,
							MAX_PATH,
							pProcListEntry->ustrImageName);
						RtlStringCchCopyW(pArkProcData[nIndex].pwszImagePath,
							MAX_PATH,
							pProcListEntry->ustrImagePath);

						if (pProcListEntry->dwPID == 4)
						{
							RtlStringCchCopyW(pArkProcData[nIndex].pwszImageName,
								MAX_PATH,
								L"System");
							RtlStringCchCopyW(pArkProcData[nIndex].pwszImagePath,
								MAX_PATH,
								L"System");
						}
						++nIndex;
					}
					else
					{
						break;
					}
				}
				if( nIndex > 0 )
				{
					retStatus = STATUS_SUCCESS;
					pIrp->IoStatus.Information = nOutputLen;
				}
			}
			DelList( ProcList );
		}
		break;

	case IOCTL_GET_DRIVERS:
		{
			PARK_DRIVER pArkDrvData = pIrp->AssociatedIrp.SystemBuffer;
			if( MmIsAddressValid( pArkDrvData ) && VALIDATE_LIST_BUFF_SIZE( nOutputLen, ARK_DRIVER, DriverList ) )
			{
				PDRIVER_LIST_ENTRY pDrvListEntry = NULL;
				while( STATUS_SUCCESS == GetListEntry( DriverList, nIndex, &pDrvListEntry ) )
				{
					if( MmIsAddressValid( pDrvListEntry ) )
					{
						pArkDrvData[nIndex].dwBaseAddr   = pDrvListEntry->dwBase;
						pArkDrvData[nIndex].dwEndAddr    = pDrvListEntry->dwEnd;
						pArkDrvData[nIndex].dwEntryPoint = pDrvListEntry->dwEntryPoint;
						RtlStringCchCopyW( pArkDrvData[nIndex].pszDriverName, MAX_PATH, pDrvListEntry->szDrvName );
						++nIndex;
					}
					else
					{
						break;
					}
				}
				if( nIndex > 0 )
				{
					retStatus = STATUS_SUCCESS;
					pIrp->IoStatus.Information = nOutputLen;
				}
			}
			DelList( DriverList );
		}
		break;

	case IOCTL_GET_SSDTHOOKS:
		{
			PARK_SSDTHOOK pArkSsdtData = pIrp->AssociatedIrp.SystemBuffer;
			if( MmIsAddressValid( pArkSsdtData ) && VALIDATE_LIST_BUFF_SIZE( nOutputLen, ARK_SSDTHOOK, SsdtList ) )
			{
				PSSDTHOOK_LIST_ENTRY pSsdtListEntry = NULL;
				while( STATUS_SUCCESS == GetListEntry( SsdtList, nIndex, &pSsdtListEntry ) )
				{
					if( MmIsAddressValid( pSsdtListEntry ) )
					{
						pArkSsdtData[nIndex].unSsdtIndex = pSsdtListEntry->unIndex;
						pArkSsdtData[nIndex].dwBaseAddr = pSsdtListEntry->dwBase;
						pArkSsdtData[nIndex].dwEndAddr = pSsdtListEntry->dwEnd;
						pArkSsdtData[nIndex].dwHookAddr = pSsdtListEntry->dwHookAddr;
						RtlStringCchCopyW( pArkSsdtData[nIndex].pszDriverName, MAX_PATH, pSsdtListEntry->szDrvName );
						++nIndex;
					}
					else
					{
						break;
					}
				}
				if( nIndex > 0 )
				{
					retStatus = STATUS_SUCCESS;
					pIrp->IoStatus.Information = nOutputLen;
				}
			}
			DelList( DriverList );
			DelList( SsdtList );
		}
		break;
	case IOCTL_GET_DLLS:
		{
			PARK_DLL pArkDllData = pIrp->AssociatedIrp.SystemBuffer;
			if( MmIsAddressValid( pArkDllData ) && VALIDATE_LIST_BUFF_SIZE( nOutputLen, ARK_DLL, DllList ) )
			{
				PDLL_LIST_ENTRY pDllListEntry = NULL;
				while( STATUS_SUCCESS == GetListEntry( DllList, nIndex, &pDllListEntry ) )
				{
					if( MmIsAddressValid( pDllListEntry ) )
					{
						pArkDllData[nIndex].baseAddr = pDllListEntry->dwBase;
						RtlStringCchCopyW( pArkDllData[nIndex].dllName, MAX_PATH, pDllListEntry->szDllName );
						++nIndex;
					}
					else
					{
						break;
					}
				}
				if( nIndex > 0 )
				{
					retStatus = STATUS_SUCCESS;
					pIrp->IoStatus.Information = nOutputLen;
				}
			}
			DelList( DllList );
		}
		break;
	case IOCTL_KILL_PROCESS:
		{
			PARK_PROCESS pArkProcData = pIrp->AssociatedIrp.SystemBuffer;
			BOOLEAN bResult;
			if( MmIsAddressValid( pArkProcData ) )
			{
				bResult = KillProcess(&pArkProcData->dwProcId);
				if (bResult)
				{
					pArkProcData->dwProcId = 0;
				}
			}
			retStatus = STATUS_SUCCESS;
			pIrp->IoStatus.Information = nOutputLen;
		}
		break;
	case IOCTL_FIX_SSDTHOOKS:
		{
			PARKFIXSSDT pFixSsdtHookData = (PARKFIXSSDT)pIrp->AssociatedIrp.SystemBuffer;
			if( MmIsAddressValid( pFixSsdtHookData ) )
			{
				if( FixSSDTHook( pFixSsdtHookData ) )
				{
					pFixSsdtHookData->dwOrigAddr = 0;
				}
				retStatus = STATUS_SUCCESS;
				pIrp->IoStatus.Information = nOutputLen;
			}
		}
		break;
	default:
		DbgPrint("Unknown IO control code\n");
	}
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DispatchCreate(__in struct _DEVICE_OBJECT *DeviceObject, __inout struct _IRP *pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	DbgPrint("DispatchCreate.\n");

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DispatchClose (__in struct _DEVICE_OBJECT *DeviceObject, __inout struct _IRP *pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	DbgPrint("DispatchClose.\n");

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}
