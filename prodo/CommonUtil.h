#pragma once

#include <string>
#include <list>
#include <vector>

#define BYTES_TO_FIX 32
#define ARKITLIB_STR_LEN 260

typedef struct _ARKUTILEXPORTENTRY
{
	CHAR szFuncName[MAX_PATH];
	DWORD dwFuncAddress;
	BYTE cFuncData[BYTES_TO_FIX];
}  ARKUTILEXPORTENTRY, *PARKUTILEXPORTENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY 
{
	HANDLE Section; 
	PVOID MappedBase; 
	PVOID Base; 
	ULONG Size; 
	ULONG Flags; 
	USHORT LoadOrderIndex; 
	USHORT InitOrderIndex; 
	USHORT LoadCount; 
	USHORT PathLength; 
	CHAR ImageName[256]; 
} SYSTEM_MODULE_INFORMATION_ENTRY, *PSYSTEM_MODULE_INFORMATION_ENTRY; 

typedef struct _SYSTEM_MODULE_INFORMATION
{
	ULONG Count; 
	SYSTEM_MODULE_INFORMATION_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION; 

typedef LONG (__stdcall *fn_NtQuerySystemInformation)(DWORD SystemInformationClass, 
													  PVOID SystemInformation, 
													  ULONG SystemInformationLength, 
													  PULONG ReturnLength); 

class CommonUtil
{
public:
	CommonUtil();
	
	~CommonUtil();

	bool formatPath(std::wstring imagePath);

	bool getZwFuncNameBySsdtIndex( UINT unIndex, std::string& zwFuncName );

	bool getSsdtIndexByZwFuncName( std::string zwFuncName, UINT& unIndex );

	bool getNtFuncAddressBySsdtIndex( UINT unIndex, std::string& ntFuncName, DWORD& dwNtAddress );

	bool getNtFuncAddressByZwFuncName( std::string zwFuncName, std::string& ntFuncName, DWORD& dwNtAddress );

	bool exportWalker( std::string& dllFileName, std::list<ARKUTILEXPORTENTRY>& expFuncList, bool bReadFuncData );
};
