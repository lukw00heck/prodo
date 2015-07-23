#include "stdafx.h"
#include "prodo.h"
#include "prodoDlg.h"
#include <shlwapi.h>
#include <winsvc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define SYS_FOLDER L"D://driver/"
#define SYS_FILE   L"D://driver/ProdoBase.sys"
#define DRV_NAME   L"ProdoBase"
#define SRV_NAME   L"\\\\.\\ProdoBase"

BEGIN_MESSAGE_MAP(CprodoApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CprodoApp::CprodoApp()
{
}

CprodoApp theApp;

BOOL InitDevice()
{
	SC_HANDLE	hSCManager;
	SC_HANDLE   hService;

	SERVICE_STATUS ServiceStatus;

	BOOL		bret = FALSE;

	hSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

	if (!hSCManager)
	{
		TRACE("[-] OpenSCManager Failed.\n");
		return FALSE;
	}
	hService = OpenService( hSCManager, DRV_NAME, SERVICE_ALL_ACCESS );

	if (!hService)
	{
		TRACE("[+] OpenService Fail. Try to CreateService.\n");

		hService = CreateService( hSCManager,
			DRV_NAME,              // name of service
			DRV_NAME,              // name to display
			SERVICE_ALL_ACCESS,    // desired access
			SERVICE_KERNEL_DRIVER, // service type
			SERVICE_DEMAND_START,  // start type
			SERVICE_ERROR_NORMAL,  // error control type
			SYS_FILE,              // service's binary
			NULL,                  // no load ordering group
			NULL,                  // no tag identifier
			NULL,                  // no dependencies
			NULL,                  // LocalSystem account
			NULL                   // no password
			);

		if (!hService)
		{
			TRACE("[-] CreateService Fail 0x%X.\n", GetLastError());
			return FALSE;
		}

		// hService = OpenService( hSCManager, DRV_NAME, SERVICE_ALL_ACCESS );
	}
	bret = QueryServiceStatus(hService, &ServiceStatus);
	if (!bret)
	{
		TRACE("[-] QueryServiceStatus Fail.\n");
		CloseHandle(hService);
		CloseHandle(hSCManager);
		return FALSE;
	}
	if (SERVICE_RUNNING != ServiceStatus.dwCurrentState)
	{
		bret = StartService(hService, 0, NULL);
		if (!bret)
		{
			TRACE("[-] StartService Fail.\n");
			CloseHandle(hService);
			CloseHandle(hSCManager);
			return FALSE;
		}
	}

	return bret;
}

BOOL CprodoApp::InitInstance()
{
	BOOL bRet;

	INITCOMMONCONTROLSEX InitCtrls;

	InitCtrls.dwSize = sizeof(InitCtrls);

	InitCtrls.dwICC = ICC_WIN95_CLASSES;

	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	m_hSem = CreateSemaphore(NULL, 1, 1, AfxGetApp()->m_pszAppName);

	if (GetLastError() == ERROR_ALREADY_EXISTS) 
	{
		CloseHandle(m_hSem);
		m_hSem = NULL;

		HWND hWnd = ::FindWindow(NULL, _T("Prodo"));
		if (hWnd)
		{
			::SetForegroundWindow(hWnd); 
			::ShowWindow(hWnd, SW_SHOW);
		}
		return FALSE; 
	}

	if (!PathFileExists(SYS_FOLDER))
	{
		TRACE("[-] Sys Folder not exists\n");
		bRet = CreateDirectory(SYS_FOLDER, NULL);
		if ( !bRet )
		{
			TRACE("[-] CreateDirectory Failed.\n");
			return FALSE;
		}
	}
	if (!PathFileExists(SYS_FILE))
	{
		TRACE("[-] Sys File not exists\n");
		HANDLE hFile = NULL;

		HMODULE hModule = GetModuleHandle(NULL);
		if (!hModule)
		{
			TRACE("[-] GetModuleHandle Failed.\n");
			return FALSE;
		}
		HRSRC hrSrc = FindResource(hModule, MAKEINTRESOURCE(IDR_SYS4), _T("SYS"));
		if (!hrSrc)
		{
			TRACE("[-] FindResource Failed.\n");
			return FALSE;
		}
		DWORD dwSize = 0;
		dwSize = SizeofResource(hModule, hrSrc);
		HGLOBAL hGlobal = LoadResource(hModule, hrSrc);
		if (!hGlobal)
		{
			TRACE("[-] LoadResource Failed.\n");
			return FALSE;
		}
		LPVOID pBuffer = LockResource(hGlobal);
		if (!pBuffer)
		{
			TRACE("[-] LockResource Failed.\n");
			return FALSE;
		}
		hFile = CreateFile(SYS_FILE, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!hFile)
		{
			TRACE("[-] CreateFile Failed.\n");
			return FALSE;
		}
		DWORD dwWriten = 0;
		bRet = WriteFile(hFile, pBuffer, dwSize, &dwWriten, NULL);
		if ( !bRet )
		{
			TRACE("[-] WriteFile Failed.\n");
			return FALSE;
		}
		if (hFile != NULL)
		{
			CloseHandle(hFile);
		}
	}

	::InitDevice();

	CprodoDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{

	}
	else if (nResponse == IDCANCEL)
	{

	}
	return FALSE;
}
