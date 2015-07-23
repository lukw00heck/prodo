#include "Lists.h"

LISTS_ARRAY g_lstArray = {0};

NTSTATUS InitList(LIST_TYPE ListType)
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;

	__try
	{
		PLIST_ENTRY pHead = NULL;

		DelList(ListType);

		pHead = ExAllocatePoolWithTag(PagedPool, sizeof( LIST_ENTRY ), TAG_LIST);

		switch( ListType )
		{
		case ProcList:
			{
				if( MmIsAddressValid( g_lstArray.pProcListHead ) )
				{
					ExFreePool( g_lstArray.pProcListHead );
				}
				g_lstArray.pProcListHead = pHead;
			}
			break;

		case DllList:
			{
				if( MmIsAddressValid( g_lstArray.pDllListHead ) )
				{
					ExFreePool( g_lstArray.pDllListHead );
				}
				g_lstArray.pDllListHead = pHead;
			}
			break;

		case DriverList:
			{
				if( MmIsAddressValid( g_lstArray.pDrvListHead ) )
				{
					ExFreePool( g_lstArray.pDrvListHead );
				}
				g_lstArray.pDrvListHead = pHead;
			}
			break;

		case SsdtList:
			{
				if( MmIsAddressValid( g_lstArray.pSsdtListHead ) )
				{
					ExFreePool( g_lstArray.pSsdtListHead );
				}
				g_lstArray.pSsdtListHead = pHead;
			}
			break;

		default:
			{
				pHead = NULL;
				retStatus = STATUS_UNSUCCESSFUL;
			}
			break;
		}

		if( pHead )
		{
			RtlZeroMemory(pHead, sizeof( LIST_ENTRY ));
			InitializeListHead( pHead );
			retStatus = STATUS_SUCCESS;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint( "Exception caught in InitList()" );
		retStatus = STATUS_UNSUCCESSFUL;
	}
	return retStatus;
}

NTSTATUS DelList( LIST_TYPE ListType )
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;

	__try
	{
		PLIST_ENTRY pHead  = NULL;
		PLIST_ENTRY pEntry = NULL;

		PVOID pCurrent = NULL;

		switch (ListType)
		{
		case ProcList:    pHead = g_lstArray.pProcListHead;
			break;
		case DllList:     pHead = g_lstArray.pDllListHead;
			break;
		case DriverList:  pHead = g_lstArray.pDrvListHead;
			break;
		case SsdtList:    pHead = g_lstArray.pSsdtListHead;
			break;
		default:
			break;
		}
		while( MmIsAddressValid(pHead) && !IsListEmpty(pHead) )
		{
			pEntry = RemoveHeadList( pHead );

			if( pEntry )
			{
				pCurrent = NULL;
				switch( ListType )
				{
				case ProcList:
					pCurrent = CONTAINING_RECORD( pEntry, PROC_LIST_ENTRY, lEntry );
					break;
				case DllList:
					pCurrent = CONTAINING_RECORD( pEntry, DLL_LIST_ENTRY, lEntry );
					break;
				case DriverList:
					pCurrent = CONTAINING_RECORD( pEntry, DRIVER_LIST_ENTRY, lEntry );
					break;
				case SsdtList:
					pCurrent = CONTAINING_RECORD( pEntry, SSDTHOOK_LIST_ENTRY, lEntry );
					break;
				}
				if( MmIsAddressValid( pCurrent ) )
				{
					ExFreePool( pCurrent );
				}
			}
			if( pEntry == pHead )
			{
				retStatus = STATUS_SUCCESS;
				break;
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("DelList error");
	}
	return retStatus;
}

BOOLEAN IsMyListEmpty( LIST_TYPE eTypeOfList )
{
	BOOLEAN bIsEmpty = TRUE;
	__try
	{
		PLIST_ENTRY pHead  = NULL;

		switch (eTypeOfList)
		{
		case ProcList: pHead = g_lstArray.pProcListHead;
			break;
		case DllList:     pHead = g_lstArray.pDllListHead;
			break;
		case DriverList:  pHead = g_lstArray.pDrvListHead;
			break;
		case SsdtList:    pHead = g_lstArray.pSsdtListHead;
			break;
		default:
			break;
		}

		if( pHead )
		{
			bIsEmpty = IsListEmpty( pHead );
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("IsMyListEmpty error");
	}
	return bIsEmpty;
}

NTSTATUS GetListEntry( LIST_TYPE eTypeOfList, UINT nPosition, PVOID* ppItem )
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;

	__try
	{
		PLIST_ENTRY pHead = NULL;

		*ppItem = NULL;

		switch (eTypeOfList)
		{
		case ProcList:    pHead = g_lstArray.pProcListHead;
			break;
		case DllList:     pHead = g_lstArray.pDllListHead;
			break;
		case DriverList:  pHead = g_lstArray.pDrvListHead;
			break;
		case SsdtList:    pHead = g_lstArray.pSsdtListHead;
			break;
		default:
			break;
		}
		if (pHead)
		{
			UINT i = 0;
			PLIST_ENTRY pEntry = pHead->Flink;
			do 
			{
				if( MmIsAddressValid(pEntry) )
				{
					if (i == nPosition)
					{
						switch( eTypeOfList )
						{
						case ProcList:
							*ppItem = CONTAINING_RECORD( pEntry, PROC_LIST_ENTRY, lEntry );
							break;
						case DllList:
							*ppItem = CONTAINING_RECORD( pEntry, DLL_LIST_ENTRY, lEntry );
							break;
						case DriverList:
							*ppItem = CONTAINING_RECORD( pEntry, DRIVER_LIST_ENTRY, lEntry );
							break;
						case SsdtList:
							*ppItem = CONTAINING_RECORD( pEntry, SSDTHOOK_LIST_ENTRY, lEntry );
							break;
						}
					}
					i++;
					pEntry = pEntry->Flink;
				}
				else
				{
					break;
				}
			} while (pEntry != pHead);
		}

		if( *ppItem )
		{
			retStatus = STATUS_SUCCESS;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("GetListEntry error");
		retStatus = STATUS_UNSUCCESSFUL;
	}
	return retStatus;
}

NTSTATUS AddListEntry( LIST_TYPE eTypeOfList, PVOID pItemToAdd, BOOLEAN bFind )
{
	NTSTATUS retStatus = STATUS_UNSUCCESSFUL;
	__try
	{
		if( bFind && ( STATUS_SUCCESS == FindEntry( eTypeOfList, pItemToAdd ) ) )
		{
			retStatus = STATUS_DUPLICATE_OBJECTID;
			return retStatus;
		}

		switch( eTypeOfList )
		{
		case ProcList:
			{
				if( MmIsAddressValid( g_lstArray.pProcListHead ) )
				{
					PPROC_LIST_ENTRY pNewEntry = NULL;
					pNewEntry = ExAllocatePoolWithTag( PagedPool,
						sizeof( PROC_LIST_ENTRY ),
						TAG_LIST );
					if( pNewEntry )
					{
						RtlZeroMemory( pNewEntry, sizeof( PROC_LIST_ENTRY ) );
						pNewEntry->dwPID = ((PPROC_LIST_ENTRY)pItemToAdd)->dwPID;
						RtlStringCchCopyW( pNewEntry->ustrImageName, MAX_PATH, ((PPROC_LIST_ENTRY)pItemToAdd)->ustrImageName );
						RtlStringCchCopyW( pNewEntry->ustrImagePath, MAX_PATH, ((PPROC_LIST_ENTRY)pItemToAdd)->ustrImagePath );
						InsertHeadList( g_lstArray.pProcListHead, &( pNewEntry->lEntry ) );
						retStatus = STATUS_SUCCESS;
					}
				}
			}
			break;

		case DllList:
			{
				if( MmIsAddressValid( g_lstArray.pDllListHead ) )
				{
					PDLL_LIST_ENTRY pNewEntry = ExAllocatePoolWithTag( NonPagedPool,
						sizeof( DLL_LIST_ENTRY ),
						TAG_LIST );
					if( pNewEntry )
					{
						RtlZeroMemory( pNewEntry, sizeof( DLL_LIST_ENTRY ) );
						pNewEntry->dwBase = ((PDLL_LIST_ENTRY)pItemToAdd)->dwBase;
						RtlStringCchCopyW( pNewEntry->szDllName, MAX_PATH, ((PDLL_LIST_ENTRY)pItemToAdd)->szDllName );
						InsertHeadList( g_lstArray.pDllListHead, &( pNewEntry->lEntry ) );
						retStatus = STATUS_SUCCESS;
					}
				}
			}
			break;

		case DriverList:
			{
				if( MmIsAddressValid( g_lstArray.pDrvListHead ) )
				{
					PDRIVER_LIST_ENTRY pNewEntry = NULL;
					pNewEntry = ExAllocatePoolWithTag( NonPagedPool,
						sizeof( DRIVER_LIST_ENTRY ),
						TAG_LIST );
					if( pNewEntry )
					{
						RtlZeroMemory( pNewEntry, sizeof( DRIVER_LIST_ENTRY ) );
						pNewEntry->dwBase = ((PDRIVER_LIST_ENTRY)pItemToAdd)->dwBase;
						pNewEntry->dwEnd = ((PDRIVER_LIST_ENTRY)pItemToAdd)->dwEnd;
						pNewEntry->dwEntryPoint = ((PDRIVER_LIST_ENTRY)pItemToAdd)->dwEntryPoint;
						RtlStringCchCopyW( pNewEntry->szDrvName, MAX_PATH, ((PDRIVER_LIST_ENTRY)pItemToAdd)->szDrvName );
						InsertHeadList( g_lstArray.pDrvListHead, &( pNewEntry->lEntry ) );
						retStatus = STATUS_SUCCESS;
					}
				}
			}
			break;

		case SsdtList:
			{
				if( MmIsAddressValid( g_lstArray.pSsdtListHead ) )
				{
					PSSDTHOOK_LIST_ENTRY pNewEntry = NULL;
					pNewEntry = ExAllocatePoolWithTag( NonPagedPool,
						sizeof( SSDTHOOK_LIST_ENTRY ),
						TAG_LIST );
					if( pNewEntry )
					{
						RtlZeroMemory( pNewEntry, sizeof( SSDTHOOK_LIST_ENTRY ) );
						pNewEntry->unIndex = ((PSSDTHOOK_LIST_ENTRY)pItemToAdd)->unIndex;
						pNewEntry->dwHookAddr = ((PSSDTHOOK_LIST_ENTRY)pItemToAdd)->dwHookAddr;
						pNewEntry->dwBase = ((PSSDTHOOK_LIST_ENTRY)pItemToAdd)->dwBase;
						pNewEntry->dwEnd = ((PSSDTHOOK_LIST_ENTRY)pItemToAdd)->dwEnd;
						RtlStringCchCopyW( pNewEntry->szDrvName, MAX_PATH, ((PSSDTHOOK_LIST_ENTRY)pItemToAdd)->szDrvName );
						InsertHeadList( g_lstArray.pSsdtListHead, &( pNewEntry->lEntry ) );
						retStatus = STATUS_SUCCESS;
					}
				}
			}
			break;

		default:
			{
				retStatus = STATUS_UNSUCCESSFUL;
			}
			break;
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		DbgPrint( "Exception caught in AddListEntry()" );
		retStatus = STATUS_UNSUCCESSFUL;
	}
	return retStatus;
}

UINT GetListCount( LIST_TYPE eTypeOfList )
{
	UINT nCnt = 0;
	__try
	{
		PVOID pItem = NULL;
		while( STATUS_SUCCESS == GetListEntry( eTypeOfList, nCnt, &pItem ) )
		{
			++nCnt;
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		DbgPrint( "Exception caught in GetListCount()" );
		nCnt = 0;
	}
	return nCnt;
}

NTSTATUS FindEntry( LIST_TYPE ListType, PVOID pItemToFind )
{
	NTSTATUS retVal = STATUS_UNSUCCESSFUL;
	__try
	{
		UINT i = 0;
		PVOID pCurrent = NULL;

		if( pItemToFind )
		{
			// Loop through the list to find required entry
			while( STATUS_SUCCESS == GetListEntry( ListType, i, &pCurrent ) )
			{
				++i;
				if( pCurrent )
				{
					switch( ListType )
					{
					case ProcList:
						{
							if( ((PPROC_LIST_ENTRY)pCurrent)->dwPID == ((PPROC_LIST_ENTRY)pItemToFind)->dwPID )
							{
								retVal = STATUS_SUCCESS;
							}
						}
						break;

					case DllList:
						{
							if( ((PDLL_LIST_ENTRY)pCurrent)->dwBase == ((PDLL_LIST_ENTRY)pItemToFind)->dwBase )
							{
								retVal = STATUS_SUCCESS;
							}
						}
						break;

					case DriverList:
						{
							if( ((PDRIVER_LIST_ENTRY)pCurrent)->dwBase == ((PDRIVER_LIST_ENTRY)pItemToFind)->dwBase )
							{
								retVal = STATUS_SUCCESS;
							}
						}
						break;

					case SsdtList:
						{
							if( ((PSSDTHOOK_LIST_ENTRY)pCurrent)->dwHookAddr == ((PSSDTHOOK_LIST_ENTRY)pItemToFind)->dwHookAddr )
							{
								retVal = STATUS_SUCCESS;
							}
						}
						break;

					default:
						{
							retVal = STATUS_UNSUCCESSFUL;
						}
						break;
					}
				}
				else
				{
					break;
				}

				// Found entry
				if( STATUS_SUCCESS == retVal )
				{
					break;
				}
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		DbgPrint( "Exception caught in FindEntry()" );
		retVal = STATUS_UNSUCCESSFUL;
	}
	return retVal;
}
