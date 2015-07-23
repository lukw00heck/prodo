#pragma once

#define		DEVICE_NAME		L"\\Device\\ProdoBase"
#define		LINK_NAME		L"\\DosDevices\\ProdoBase"

#define IOCRL_BASE 0x800
#define MY_CTL_CODE(i) CTL_CODE(FILE_DEVICE_UNKNOWN, \
	IOCRL_BASE + i, \
	METHOD_BUFFERED, \
	FILE_ANY_ACCESS)

#define IOCTL_GET_DATA_CNT      MY_CTL_CODE(0)
#define IOCTL_GET_PROCESS       MY_CTL_CODE(1)
#define IOCTL_GET_DRIVERS       MY_CTL_CODE(2)
#define IOCTL_GET_SSDTHOOKS     MY_CTL_CODE(3)
#define IOCTL_GET_DLLS          MY_CTL_CODE(4)
#define IOCTL_KILL_PROCESS      MY_CTL_CODE(5)
#define IOCTL_FIX_SSDTHOOKS     MY_CTL_CODE(6)
#define IOCTL_INIT              MY_CTL_CODE(7)

typedef enum ArkDataType
{
	ArkDataInvalid = 0,
	ArkDataProcList,
	ArkDataDllList,
	ArkDataDriverList,
	ArkDataSsdtList
} ARK_DATA_TYPE;

typedef struct _ARK_DATA_COUNT {
	UINT  nDataCount;
	DWORD dwMixData;
	ARK_DATA_TYPE DataType;
} ARK_DATA_COUNT, *PARK_DATA_COUNT;

typedef struct _ARK_PROCESS {
	DWORD dwProcId;
	WCHAR pwszImageName[MAX_PATH];
	WCHAR pwszImagePath[MAX_PATH];
} ARK_PROCESS, *PARK_PROCESS;

typedef struct _ARK_DRIVER {
	DWORD dwBaseAddr;
	DWORD dwEndAddr;
	DWORD dwEntryPoint;
	WCHAR pszDriverName[MAX_PATH];
} ARK_DRIVER, *PARK_DRIVER;

typedef struct _ARK_SSDTHOOK {
	UINT unSsdtIndex;
	DWORD dwBaseAddr;
	DWORD dwEndAddr;
	DWORD dwHookAddr;
	WCHAR pszSsdtFuncName[MAX_PATH];
	WCHAR pszDriverName[MAX_PATH];
} ARK_SSDTHOOK, *PARK_SSDTHOOK;

typedef struct _ARK_DLL {
	DWORD baseAddr;
	WCHAR dllName[MAX_PATH];
} ARK_DLL, *PARK_DLL;

typedef struct _ARKFIXSSDT {
	DWORD dwSsdtIndex;
	DWORD dwOrigAddr;
} ARKFIXSSDT, *PARKFIXSSDT;
