#include "stdafx.h"
#include "CommonUtil.h"
#include <winbase.h>
#include <Strsafe.h>
#include <Psapi.h>
#pragma comment( lib, "psapi.lib" )

CommonUtil::CommonUtil()
{

}
CommonUtil::~CommonUtil()
{
}

bool CommonUtil::getZwFuncNameBySsdtIndex( UINT unIndex, std::string& zwFuncName )
{
	bool retVal = false;
	try
	{
		std::list<ARKUTILEXPORTENTRY> exportList;
		std::string ntDllName( "ntdll.dll" );

		exportList.clear();
		zwFuncName.assign( "" );
 
		if( exportWalker( ntDllName, exportList, false ) )
		{
			std::string tmpStr;
			std::vector<std::string> ssdtNames;

			ssdtNames.clear();
			std::list<ARKUTILEXPORTENTRY>::iterator itExport = exportList.begin();
			for( ; itExport != exportList.end(); itExport++ )
			{
				tmpStr.assign( itExport->szFuncName );

				if( 0 == tmpStr.find( "Zw" ) )
				{
					if( ( std::string::npos == tmpStr.find( "ZwCreateKeyedEvent" ) ) &&
						( std::string::npos == tmpStr.find( "ZwOpenKeyedEvent" ) ) &&
						( std::string::npos == tmpStr.find( "ZwReleaseKeyedEvent" ) ) &&
						( std::string::npos == tmpStr.find( "ZwWaitForKeyedEvent" ) ) &&
						( std::string::npos == tmpStr.find( "ZwQueryPortInformationProcess" ) ) )
					{
						ssdtNames.push_back( tmpStr );
					}
				}
			}
			if( !ssdtNames.empty() )
			{
				ssdtNames.push_back( "ZwCreateKeyedEvent" );
				ssdtNames.push_back( "ZwOpenKeyedEvent" );
				ssdtNames.push_back( "ZwReleaseKeyedEvent" );
				ssdtNames.push_back( "ZwWaitForKeyedEvent" );
				ssdtNames.push_back( "ZwQueryPortInformationProcess" );
			}
			if( unIndex < ssdtNames.size() )
			{
				zwFuncName.assign( ssdtNames[unIndex] );
				retVal = true;
			}
		}
	}
	catch(...)
	{
		retVal = false;
		::OutputDebugStringA( "getZwFuncNameBySsdtIndex: Exception caught" );
	}
	return retVal;
}

bool CommonUtil::exportWalker( std::string& dllFileName, std::list<ARKUTILEXPORTENTRY>& expFuncList, bool bReadFuncData )
{
	bool retVal = false;
	try
	{
		expFuncList.clear();

		if( dllFileName.length() )
		{
			HMODULE hModBase = ::LoadLibraryExA( dllFileName.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES );
			if( NULL != hModBase )
			{
				PIMAGE_NT_HEADERS pNtHeader = NULL;
				PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModBase;
				if( pDosHeader && ( IMAGE_DOS_SIGNATURE == pDosHeader->e_magic ) )
				{
					// Get the pointer to IMAGE_NT_HEADERS
					pNtHeader = (PIMAGE_NT_HEADERS)( (PBYTE)hModBase + pDosHeader->e_lfanew );
				}
				if( pNtHeader && ( IMAGE_NT_SIGNATURE == pNtHeader->Signature ) )
				{
					// Get pointer to IMAGE_EXPORT_DIRECTORY
					PIMAGE_EXPORT_DIRECTORY pExpTable = NULL;
					pExpTable = (PIMAGE_EXPORT_DIRECTORY)( (PBYTE)hModBase + pNtHeader->OptionalHeader.DataDirectory[0].VirtualAddress );
					if( NULL != pExpTable )
					{
						// Get pointer to name and address tables of functions
						PDWORD pdwAddressOfFunctions = (PDWORD)( (PBYTE)hModBase + pExpTable->AddressOfFunctions );
						DWORD dwNumberOfFunctions = pExpTable->NumberOfFunctions;

						PDWORD pdwAddressOfNames = (PDWORD)( (PBYTE)hModBase + pExpTable->AddressOfNames );
						DWORD dwNumberOfNames = pExpTable->NumberOfNames;

						DWORD dwLimit = ( dwNumberOfFunctions > dwNumberOfNames ) ? dwNumberOfNames : dwNumberOfFunctions;

						// Loop through the names
						ARKUTILEXPORTENTRY exportEntry;
						for( UINT nFuncIndex = 0; nFuncIndex < dwLimit; nFuncIndex++ )
						{
							::ZeroMemory( &exportEntry, sizeof( ARKUTILEXPORTENTRY ) );

							// Get function name and address
							::StringCchPrintfA( exportEntry.szFuncName, ARKITLIB_STR_LEN, "%s",
								(char*)( (PBYTE)hModBase + pdwAddressOfNames[nFuncIndex] ) );
							exportEntry.dwFuncAddress = (DWORD)::GetProcAddress( hModBase, exportEntry.szFuncName );

							// If read-data flag is set, then read first few bytes
							// of function from this image
							if( bReadFuncData )
							{
								PBYTE pFuncPtr = (PBYTE)exportEntry.dwFuncAddress;
								for( UINT i = 0; ( i < BYTES_TO_FIX ) && ( pFuncPtr + i ) ; i++ )
								{
									exportEntry.cFuncData[i] = pFuncPtr[i];
								}
							}

							// Push it to list
							expFuncList.push_back( exportEntry );
						}
					}
				}
				::FreeLibrary( hModBase );
			}
		}

		if( !expFuncList.empty() )
		{
			retVal = true;
		}
	}
	catch(...)
	{
		retVal = false;
		::OutputDebugStringW( L"exportWalker: Exception caught" );
	}
	return retVal;
}

bool CommonUtil::formatPath(std::wstring imagePath)
{
	bool retVal = false;

	try
	{
		if ( imagePath.length() <= 0 || imagePath.length() >= MAX_PATH )
		{
			return retVal;
		}
		UINT newPathIndex = 0;
		UINT oldPathIndex = 0;
		WCHAR szTempPath[MAX_PATH];

		::ZeroMemory( szTempPath, MAX_PATH * sizeof(wchar_t) );
		if( std::wstring::npos != imagePath.find( L"\\Device\\" ) )
		{
			DWORD drvMaskIndex = 1;
			DWORD dwLogDrivesMask = 0;
			WCHAR szDrvLetter[3] = L"A:";
			WCHAR szDeviceName[MAX_PATH];

			dwLogDrivesMask = ::GetLogicalDrives();

			for( drvMaskIndex = 1; drvMaskIndex < 0x80000000; drvMaskIndex = ( drvMaskIndex * 0x2 ) )
			{
				if( dwLogDrivesMask & drvMaskIndex )
				{
					// then query its DOS device name ("HarddiskVolume1" etc.)
					::ZeroMemory( szDeviceName, MAX_PATH );
					if( ::QueryDosDevice( szDrvLetter, szDeviceName, MAX_PATH ) )
					{
						// Check if this DOS device name is present in our path
						if( std::wstring::npos != imagePath.find( szDeviceName ) )
						{
							// Copy drive letter with colon to szTempPath
							::StringCchPrintf( szTempPath, MAX_PATH, L"%s", szDrvLetter );
							oldPathIndex = ::lstrlen( szDeviceName );
							newPathIndex = ::lstrlen( szTempPath );
							for( ; oldPathIndex < imagePath.length(); newPathIndex++, oldPathIndex++ )
							{
								szTempPath[newPathIndex] = imagePath[oldPathIndex];
							}
							szTempPath[newPathIndex] = '\0';
							imagePath.assign( szTempPath );
							retVal = true;
							break;
						}
					}
				}
				// Try with next drive letter
				++( szDrvLetter[0] );
			}
		}
		else if( std::wstring::npos != imagePath.find( L"\\SystemRoot\\" ) )
		{
			WCHAR szExpandedStr[MAX_PATH];
			::ZeroMemory( szExpandedStr, MAX_PATH * sizeof(WCHAR) );
			if( ::GetEnvironmentVariable( L"SystemRoot", szExpandedStr, MAX_PATH ) > 0 )
			{
				::StringCchPrintf( szTempPath, MAX_PATH, L"%s\\", szExpandedStr );
				oldPathIndex = ::lstrlen( L"\\SystemRoot\\" );
				newPathIndex = ::lstrlen( szTempPath );
				for( ; oldPathIndex < imagePath.length(); newPathIndex++, oldPathIndex++ )
				{
					szTempPath[newPathIndex] = imagePath[oldPathIndex];
				}
				szTempPath[newPathIndex] = '\0';
				imagePath.assign( szTempPath );
				retVal = true;
			}
		}
	}
	catch (...)
	{
		retVal = false;
		::OutputDebugString( L"formatPath: Exception caught" );
	}
	return retVal;
}

bool CommonUtil::getNtFuncAddressBySsdtIndex( UINT unIndex, std::string& ntFuncName, DWORD& dwNtAddress )
{
	bool bResult = false;

	DWORD dwSystemNtKernelBase = 0;

	std::string ntKernelName( "" );

	DWORD dwNeededBuffSize = 0;
	DWORD dwKernelBase = 0;
	PCHAR lpKrnlName;

	char szFullPath[MAX_PATH] = {0};

	try
	{
		dwNtAddress = 0;
		ntFuncName.assign( "" );

		// Return if the index is out of range
		if( !getZwFuncNameBySsdtIndex( unIndex, ntFuncName ) )
		{
			return bResult;
		}

		// The above function returns ZwXxx, change it to NtXxx
		ntFuncName.replace( 0, 2, "Nt" );

		fn_NtQuerySystemInformation NtQuerySystemInformation = 
			(fn_NtQuerySystemInformation)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtQuerySystemInformation");

		// Get NT kernel name
		if(NtQuerySystemInformation(11, NULL, 0, &dwNeededBuffSize) && 0) //!=STATUS_INFO_LENGTH_MISMATCH) 
		{
			int errCode = GetLastError();
			printf("Error: %d\n", errCode);
			return FALSE; 
		}

		PSYSTEM_MODULE_INFORMATION pSysModInfo = (PSYSTEM_MODULE_INFORMATION)malloc(dwNeededBuffSize); 
		if(NtQuerySystemInformation(11, pSysModInfo, dwNeededBuffSize, NULL) && 0) 
		{
			int errCode = GetLastError();
			printf("Error: %d\n", errCode);
			return FALSE; 
		}

		dwSystemNtKernelBase = (DWORD)pSysModInfo->Module[0].Base; //ntoskrnl的当前加载基址

		lpKrnlName = pSysModInfo->Module[0].ImageName;

		lpKrnlName = strstr(lpKrnlName, "nt");
		GetSystemDirectoryA(szFullPath, MAX_PATH);
		strcat_s(szFullPath, MAX_PATH, "\\");
		strcat_s(szFullPath, MAX_PATH, lpKrnlName);

		ntKernelName.assign(szFullPath);

		if( 0 == ntKernelName.length() )
		{
			return bResult;
		}
		// Load NT kernel
		HMODULE hKernel = ::LoadLibraryExA( ntKernelName.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES );
		if( !hKernel || INVALID_HANDLE_VALUE == hKernel )
		{
			return bResult;
		}
		// Get kernel module info
		MODULEINFO kernelInfo;
		::ZeroMemory( &kernelInfo, sizeof( MODULEINFO ) );
		if( !::GetModuleInformation( ::GetCurrentProcess(), hKernel, &kernelInfo, sizeof( MODULEINFO ) ) )
		{
			return bResult;
		}

		// Calculate original kernel base and end addresses
		PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)( (DWORD)hKernel + ((PIMAGE_DOS_HEADER)hKernel)->e_lfanew );
		DWORD dwOrgKernelBase = pNtHeaders->OptionalHeader.ImageBase;
		DWORD dwOrgKernelEnd = dwOrgKernelBase + kernelInfo.SizeOfImage;

		// Get actual kernel base and end addresses
		DWORD dwActualKernelBase = (DWORD)hKernel;
		DWORD dwActualKernelEnd = dwActualKernelBase + kernelInfo.SizeOfImage;

		// Get NtXxx address of one kernel exported function, to use as a
		// reference to calculate address of any other ZwXxx/NtXxx function
		typedef struct _PilotFunction
		{
			char funcName[ARKITLIB_STR_LEN];
			UINT unSsdtIndex;
			DWORD dwNtAddress;
			DWORD dwSsdtAddress;
		} PilotFunction, *PPilotFunction;

		// NtCreateFile is our pilot function as it is exported by kernel
		PilotFunction pilotFunction;
		::ZeroMemory( &pilotFunction, sizeof( PilotFunction ) );
		::StringCchCopyA( pilotFunction.funcName, ARKITLIB_STR_LEN, "NtCreateFile" );

		// Get the index of ZwCreateFile in SSDT
		std::string zwPilotFuncName( "ZwCreateFile" );
		getSsdtIndexByZwFuncName( zwPilotFuncName, pilotFunction.unSsdtIndex );

		// Get the address of NtCreateFile and calculate its offset address in our kernel image
		PVOID pFunction = (PVOID)::GetProcAddress( hKernel, pilotFunction.funcName );
		pilotFunction.dwNtAddress = (DWORD)pFunction - (DWORD)hKernel + dwOrgKernelBase;

		// Now, find offset adress of ZwCreateFile in our kernel image
		DWORD dwCurDword = 0;
		DWORD dwPrevDword = 0;
		DWORD dwNextDword = 0;
		for( PBYTE i = (PBYTE)dwActualKernelBase + sizeof(DWORD); i < (PBYTE)dwActualKernelEnd - sizeof(DWORD); i++ )
		{
			dwCurDword = *(PDWORD)i;
			dwPrevDword = *(PDWORD)( i - sizeof(DWORD) );
			dwNextDword = *(PDWORD)( i + sizeof(DWORD) );

			if( ( dwCurDword == pilotFunction.dwNtAddress ) &&
				( ( dwPrevDword >= dwOrgKernelBase ) && ( dwPrevDword <= dwOrgKernelEnd ) ) &&
				( ( dwNextDword >= dwOrgKernelBase ) && ( dwNextDword <= dwOrgKernelEnd ) ) )
			{
				pilotFunction.dwSsdtAddress = (DWORD)i;
				break;
			}
		}

		// Get system's NT kernel image base
		// Now, calculate NtXxx address of specified unction in system's NT kernel image
		DWORD dwSsdtAddress = pilotFunction.dwSsdtAddress + ( unIndex - pilotFunction.unSsdtIndex )*sizeof(DWORD);
		dwNtAddress = *(PDWORD)dwSsdtAddress - dwOrgKernelBase + dwSystemNtKernelBase;

		::FreeLibrary( hKernel );

		if( dwNtAddress )
		{
			bResult = true;
		}
	}
	catch(...)
	{
		bResult = false;
		::OutputDebugStringA( "ARKitLibUtils::getNtFuncAddressBySsdtIndex: Exception caught" );
	}
	return bResult;
}

bool CommonUtil::getNtFuncAddressByZwFuncName( std::string zwFuncName, std::string& ntFuncName, DWORD& dwNtAddress )
{
	bool retVal = false;
	try
	{
		dwNtAddress = 0;
		UINT unIndex = 0;
		if( getSsdtIndexByZwFuncName( zwFuncName, unIndex ) )
		{
			retVal = getNtFuncAddressBySsdtIndex( unIndex, ntFuncName, dwNtAddress );
		}
	}
	catch(...)
	{
		retVal = false;
		::OutputDebugStringA( "ARKitLibUtils::getNtFuncAddressByZwFuncName: Exception caught" );
	}
	return retVal;
}
bool CommonUtil::getSsdtIndexByZwFuncName( std::string zwFuncName, UINT& unIndex )
{
	bool retVal = false;
	try
	{
		UINT i = 0;
		std::string tempStr;
		while( 1 )
		{
			tempStr.assign( "" );
			if( getZwFuncNameBySsdtIndex( i, tempStr ) )
			{
				if( tempStr == zwFuncName )
				{
					unIndex = i;
					retVal = true;
					break;
				}
			}
			else
			{
				break;
			}
			i++;
		}
	}
	catch(...)
	{
		retVal = false;
		::OutputDebugStringA( "ARKitLibUtils::getSsdtIndexByZwFuncName: Exception caught" );
	}
	return retVal;
}
