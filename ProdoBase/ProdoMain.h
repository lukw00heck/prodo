#include "Commons.h"
#include "Lists.h"
#include "ProdoCmd.h"

NTSTATUS DispatchIoctl (__in struct _DEVICE_OBJECT *DeviceObject, __inout struct _IRP *pIrp);

NTSTATUS DispatchCreate (__in struct _DEVICE_OBJECT *DeviceObject, __inout struct _IRP *pIrp);
NTSTATUS DispatchClose (__in struct _DEVICE_OBJECT *DeviceObject, __inout struct _IRP *pIrp);

VOID DriverUnload(struct _DRIVER_OBJECT *pDriverObject);

UINT SSDThooksCount();
VOID SSDTscanThread(PVOID pThrParam);

UINT ProcessCount();
VOID ProcessScanThread(PVOID pThrParam);

VOID RefreshProcessByPspCidTable();
NTSTATUS ScanProcByPspCidTable();
NTSTATUS ScanProcByHandleTable();

BOOLEAN KillProcess( PDWORD pdwPid );
VOID KillProcessThread( PVOID pThrParam );
NTSTATUS KillProcessByInsertApc(DWORD dwPid);

UINT ScanAndGetDriverCount();
VOID ScanAndGetDriverCountThread( PVOID pThrParam );
NTSTATUS GetDriversByModuleEntryScan();

UINT ScanAndGetDllCount( DWORD dwPid );
VOID ScanAndGetDllCountThread( PVOID pThrParam );
NTSTATUS GetDllByVadTree( DWORD dwPid );
VOID TraverseVadTreeInOrder( PMMVAD pVadNode );

VOID FixSSDTHookThread( PVOID pThrParam );
BOOLEAN FixSSDTHook( PARKFIXSSDT pFixSsdtHookData );
