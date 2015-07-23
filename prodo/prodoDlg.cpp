#include "stdafx.h"
#include "prodo.h"
#include "prodoDlg.h"
#include "ProdoCmd.h"
#include "CommonUtil.h"

#include <WinIoCtl.h>
#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int nCurrItem = 0;

CprodoDlg::CprodoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CprodoDlg::IDD, pParent)
{
	m_hIcon_l = AfxGetApp()->LoadIcon(IDI_FRAME_ICON_LARGE);
	m_hIcon_s = AfxGetApp()->LoadIcon(IDI_FRAME_ICON_SMALL);
}

void CprodoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROMPT_TEXT, m_PromptText);
	DDX_Control(pDX, IDD_MAIN_TAB, m_TabMain);
}

BEGIN_MESSAGE_MAP(CprodoDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	
	//}}AFX_MSG_MAP
	ON_NOTIFY(TCN_SELCHANGE, IDD_MAIN_TAB, &CprodoDlg::OnTcnSelchangeMainTab)
END_MESSAGE_MAP()

BOOL CprodoDlg::InitDevice()
{  /*
	m_hDevice = ::CreateFile(_T("\\\\.\\ProdoBase"),
		GENERIC_WRITE | GENERIC_READ,  
		0,
		NULL,  
		OPEN_EXISTING,  
		0,
		NULL);  
	if( m_hDevice != INVALID_HANDLE_VALUE )  
	{
		// MessageBoxA(NULL, "Create Device ok ! \n" , "Info", MB_OK); 
		return TRUE;
	}
	else  
	{
		MessageBoxA(NULL, "Create Device faild", "Error", MB_OK); 
		return FALSE;
	}
	*/
	return TRUE;
}

BOOL CprodoDlg::GetProcess()
{
	ARK_DATA_COUNT ProcCount = {0};

	BOOL bResult = FALSE;

	DWORD dwRetSize = 0;

	WCHAR pwszCid[10] = {0};
	WCHAR pwszMsgText[20] = {0};

	LPWSTR lpszImageName = NULL;
	
	HANDLE hDevice;
	hDevice = ::CreateFile(_T("\\\\.\\ProdoBase"),
		GENERIC_WRITE | GENERIC_READ,  
		0,
		NULL,  
		OPEN_EXISTING,  
		0,
		NULL);  
	if( hDevice == INVALID_HANDLE_VALUE )  
	{
		MessageBoxA(NULL, "CreateFile Failed.", "ERROR", MB_OK);
		return FALSE;
	}

	nCurrItem = m_TabMain.GetCurSel();

	ProcCount.DataType = ArkDataProcList;

	bResult = ::DeviceIoControl(hDevice,
		IOCTL_GET_DATA_CNT,
		&ProcCount,
		sizeof(ARK_DATA_COUNT),
		&ProcCount,
		sizeof(ARK_DATA_COUNT),
		&dwRetSize, NULL );

	swprintf_s(pwszMsgText, 20, L"process count: %d",
		ProcCount.nDataCount);

	m_PromptText.SetWindowText(pwszMsgText);

	PARK_PROCESS pProcArray = new ARK_PROCESS[ ProcCount.nDataCount ];

	if( pProcArray )
	{
		::ZeroMemory( pProcArray, ( sizeof( ARK_PROCESS ) * ProcCount.nDataCount ) );
		bResult = ::DeviceIoControl( hDevice,
			IOCTL_GET_PROCESS,
			NULL,
			0,
			pProcArray,
			( sizeof( ARK_PROCESS ) * ProcCount.nDataCount ),
			&dwRetSize, NULL );
		if( !bResult )
		{
			return FALSE;
		}
	}

	for( UINT i = 0; i < ProcCount.nDataCount; i++ )
	{
		_itow_s(pProcArray[i].dwProcId, pwszCid, 10);

		lpszImageName = PathFindFileName(pProcArray[i].pwszImagePath);

		m_DialogProcess.m_ListProc.InsertItem(i, L"");

		m_DialogProcess.m_ListProc.SetItemText(i, 0, pwszCid);
		m_DialogProcess.m_ListProc.SetItemText(i, 1, lpszImageName);
		m_DialogProcess.m_ListProc.SetItemText(i, 2, pProcArray[i].pwszImagePath);
	}

	free(pProcArray);

	CloseHandle(hDevice);

	return TRUE;
}

BOOL CprodoDlg::GetDrv()
{
	ARK_DATA_COUNT DataCount = {0};

	BOOL bResult = FALSE;

	DWORD dwRetSize = 0;

	WCHAR pwszAddr[20] = {0};

	WCHAR pwszMsgText[20] = {0};

	WCHAR lpwszDrvName[MAX_PATH] = {0};

	HANDLE hDevice;
	hDevice = ::CreateFile(_T("\\\\.\\ProdoBase"),
		GENERIC_WRITE | GENERIC_READ,  
		0,
		NULL,
		OPEN_EXISTING,  
		0,
		NULL);  
	if( hDevice == INVALID_HANDLE_VALUE )  
	{
		MessageBoxA(NULL, "CreateFile Failed.", "ERROR", MB_OK);
		return FALSE;
	}
	nCurrItem = m_TabMain.GetCurSel();

	DataCount.DataType = ArkDataDriverList;

	bResult = ::DeviceIoControl(hDevice,
		IOCTL_GET_DATA_CNT,
		&DataCount,
		sizeof(ARK_DATA_COUNT),
		&DataCount,
		sizeof(ARK_DATA_COUNT),
		&dwRetSize, NULL );

	swprintf_s(pwszMsgText, 20, L"driver count: %d",
		DataCount.nDataCount);

	m_PromptText.SetWindowText(pwszMsgText);

	PARK_DRIVER pDrvArray = new ARK_DRIVER[ DataCount.nDataCount ];

	if( pDrvArray )
	{
		::ZeroMemory( pDrvArray, ( sizeof( ARK_DRIVER ) * DataCount.nDataCount ) );
		bResult = ::DeviceIoControl( hDevice,
			IOCTL_GET_DRIVERS,
			NULL,
			0,
			pDrvArray,
			( sizeof( ARK_DRIVER ) * DataCount.nDataCount ),
			&dwRetSize, NULL );
		if( !bResult )
		{
			return FALSE;
		}
	}
	for( UINT i = 0; i < DataCount.nDataCount; i++ )
	{
		m_DialogDrv.m_ListDrv.InsertItem(i, L"");

		swprintf_s(pwszAddr, 20, L"0x%X", pDrvArray[i].dwBaseAddr);

		m_DialogDrv.m_ListDrv.SetItemText(i, 0, pwszAddr);

		swprintf_s(pwszAddr, 20, L"0x%X", pDrvArray[i].dwEndAddr);
		m_DialogDrv.m_ListDrv.SetItemText(i, 1, pwszAddr);

		swprintf_s(pwszAddr, 20, L"0x%X", pDrvArray[i].dwEntryPoint);
		m_DialogDrv.m_ListDrv.SetItemText(i, 2, pwszAddr);

		m_DialogDrv.m_ListDrv.SetItemText(i, 3, pDrvArray[i].pszDriverName);
	}

	CloseHandle(hDevice);
	free(pDrvArray);

	return TRUE;
}

BOOL CprodoDlg::GetSsdtHook()
{
	ARK_DATA_COUNT DataCount = {0};

	BOOL bResult = FALSE;

	DWORD dwRetSize = 0;

	UINT nRetLen = 0;

	WCHAR pwszAddr[20] = {0};

	WCHAR pwszMsgText[30] = {0};

	WCHAR lpwszName[MAX_PATH] = {0};

	std::string strFunc("");

	CommonUtil Util;

	HANDLE hDevice;
	hDevice = ::CreateFile(_T("\\\\.\\ProdoBase"),
		GENERIC_WRITE | GENERIC_READ,  
		0,
		NULL,  
		OPEN_EXISTING,  
		0,
		NULL);  
	if( hDevice == INVALID_HANDLE_VALUE )  
	{
		MessageBoxA(NULL, "CreateFile Failed.", "ERROR", MB_OK);
		return FALSE;
	}

	nCurrItem = m_TabMain.GetCurSel();

	DataCount.DataType = ArkDataDriverList;

	bResult = ::DeviceIoControl(hDevice,
		IOCTL_GET_DATA_CNT,
		&DataCount,
		sizeof(ARK_DATA_COUNT),
		&DataCount,
		sizeof(ARK_DATA_COUNT),
		&dwRetSize, NULL );

	DataCount.DataType = ArkDataSsdtList;
	DataCount.nDataCount = 0;

	bResult = ::DeviceIoControl(hDevice,
		IOCTL_GET_DATA_CNT,
		&DataCount,
		sizeof(ARK_DATA_COUNT),
		&DataCount,
		sizeof(ARK_DATA_COUNT),
		&dwRetSize, NULL );

	swprintf_s(pwszMsgText, 30, L"SSDT Hook count: %d",
		DataCount.nDataCount);

	m_PromptText.SetWindowText(pwszMsgText);

	PARK_SSDTHOOK pHookArray = new ARK_SSDTHOOK[ DataCount.nDataCount ];

	if( pHookArray )
	{
		::ZeroMemory( pHookArray, ( sizeof( ARK_SSDTHOOK ) * DataCount.nDataCount ) );
		bResult = ::DeviceIoControl( hDevice,
			IOCTL_GET_SSDTHOOKS,
			NULL,
			0,
			pHookArray,
			( sizeof( ARK_SSDTHOOK ) * DataCount.nDataCount ),
			&dwRetSize, NULL );
		if( !bResult )
		{
			return FALSE;
		}
	}
	for( UINT i = 0; i < DataCount.nDataCount; i++ )
	{
		m_DialogSSDT.m_ListSSDT.InsertItem(i, L"");

		swprintf_s(pwszAddr, 20, L"0x%X", pHookArray[i].dwHookAddr);
		m_DialogSSDT.m_ListSSDT.SetItemText(i, 0, pwszAddr);

		strFunc.assign("");

		Util.getZwFuncNameBySsdtIndex(pHookArray[i].unSsdtIndex, strFunc);

		// swprintf_s(lpwszName, MAX_PATH, L"%s", strFunc.c_str());

		mbstowcs_s(&nRetLen, lpwszName, strFunc.c_str(), MAX_PATH);

		m_DialogSSDT.m_ListSSDT.SetItemText(i, 1, lpwszName);

		swprintf_s(pwszAddr, 20, L"0x%X", pHookArray[i].dwBaseAddr);
		m_DialogSSDT.m_ListSSDT.SetItemText(i, 2, pwszAddr);

		swprintf_s(pwszAddr, 20, L"0x%X", pHookArray[i].dwEndAddr);
		m_DialogSSDT.m_ListSSDT.SetItemText(i, 3, pwszAddr);

		m_DialogSSDT.m_ListSSDT.SetItemText(i, 4, pHookArray[i].pszDriverName);
	}

	CloseHandle(hDevice);
	free(pHookArray);

	return TRUE;
}

BOOL CprodoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon_l, TRUE);
	SetIcon(m_hIcon_s, FALSE);

	CRect rcTabClient;
	CRect rcTabItem;

	BOOL bResult = FALSE;
	BOOL bInit = InitDevice();

	if (!bInit)
	{
		return FALSE;
	}
	m_TabMain.InsertItem(0, _T("Process"));
	m_TabMain.InsertItem(1, _T("Driver Module"));
	m_TabMain.InsertItem(2, _T("SSDT Hook"));

	m_TabMain.GetClientRect(rcTabClient);
	m_TabMain.GetItemRect(0, rcTabItem);

	bResult = m_DialogProcess.Create(IDD_DIALOG_PROC, &m_TabMain);
	if (!bResult)
	{
		MessageBoxA(NULL, "m_DialogProcess.Create", "Error", MB_OK); 
		return FALSE;
	}
	bResult = m_DialogDrv.Create(IDD_DIALOG_DRV, &m_TabMain);
	bResult = m_DialogSSDT.Create(IDD_DIALOG_SSDT, &m_TabMain);

	rcTabClient.top = rcTabItem.bottom;

	rcTabClient.top += 2;
	rcTabClient.left += 3;
	rcTabClient.right -= 5;
	rcTabClient.bottom -= 5;

	m_DialogProcess.MoveWindow(&rcTabClient);
	m_DialogSSDT.MoveWindow(&rcTabClient);
	m_DialogDrv.MoveWindow(&rcTabClient);

	// m_TabMain.GetClientRect(rcTabClient);

	rcTabClient.top = 0;
	rcTabClient.left = 0;
	rcTabClient.right -= 3;
	rcTabClient.bottom -= 23;

	m_DialogProcess.m_ListProc.MoveWindow(&rcTabClient);
	m_DialogSSDT.m_ListSSDT.MoveWindow(&rcTabClient);
	m_DialogDrv.m_ListDrv.MoveWindow(&rcTabClient);

	m_DialogProcess.m_ListProc.SetExtendedStyle(
		m_DialogProcess.m_ListProc.GetExtendedStyle() | 
		LVS_EX_GRIDLINES | 
		WS_EX_STATICEDGE | 
		LVS_EX_FULLROWSELECT | 
		LVS_EX_GRIDLINES);
	m_DialogSSDT.m_ListSSDT.SetExtendedStyle(
		m_DialogSSDT.m_ListSSDT.GetExtendedStyle() | 
		LVS_EX_GRIDLINES | 
		WS_EX_STATICEDGE | 
		LVS_EX_FULLROWSELECT | 
		LVS_EX_GRIDLINES);
	m_DialogDrv.m_ListDrv.SetExtendedStyle(
		m_DialogDrv.m_ListDrv.GetExtendedStyle() | 
		LVS_EX_GRIDLINES | 
		WS_EX_STATICEDGE | 
		LVS_EX_FULLROWSELECT | 
		LVS_EX_GRIDLINES);

	int nWidht = GetSystemMetrics(SM_CXVSCROLL);

	m_DialogProcess.m_ListProc.GetClientRect(rcTabClient);

	m_DialogProcess.m_ListProc.InsertColumn(0, L"PID",  LVCFMT_LEFT, (rcTabClient.Width() - nWidht) / 11);
	m_DialogProcess.m_ListProc.InsertColumn(1, L"Name", LVCFMT_LEFT, (rcTabClient.Width() - nWidht) / 11 * 2);
	m_DialogProcess.m_ListProc.InsertColumn(2, L"Path", LVCFMT_LEFT, (rcTabClient.Width() - nWidht) / 11 * 8);

	m_DialogDrv.m_ListDrv.GetClientRect(rcTabClient);

	m_DialogDrv.m_ListDrv.InsertColumn(0, L"Base",  LVCFMT_LEFT, (rcTabClient.Width() - nWidht) / 17 * 3);
	m_DialogDrv.m_ListDrv.InsertColumn(1, L"End", LVCFMT_LEFT, (rcTabClient.Width() - nWidht) / 17 * 3);
	m_DialogDrv.m_ListDrv.InsertColumn(2, L"EntryPoint", LVCFMT_LEFT, (rcTabClient.Width() - nWidht) / 17 * 3);
	m_DialogDrv.m_ListDrv.InsertColumn(3, L"DriverName", LVCFMT_LEFT, (rcTabClient.Width() - nWidht) / 17 * 8);

	m_DialogSSDT.m_ListSSDT.GetClientRect(rcTabClient);

	m_DialogSSDT.m_ListSSDT.InsertColumn(0, L"Func Addr",  LVCFMT_LEFT, (rcTabClient.Width() - nWidht) / 17 * 3);
	m_DialogSSDT.m_ListSSDT.InsertColumn(1, L"Func Name",  LVCFMT_LEFT, (rcTabClient.Width() - nWidht) / 17 * 4);
	m_DialogSSDT.m_ListSSDT.InsertColumn(2, L"Base", LVCFMT_LEFT, (rcTabClient.Width() - nWidht) / 17 * 3);
	m_DialogSSDT.m_ListSSDT.InsertColumn(3, L"End", LVCFMT_LEFT, (rcTabClient.Width() - nWidht) / 17 * 3);
	m_DialogSSDT.m_ListSSDT.InsertColumn(4, L"DriverName", LVCFMT_LEFT, (rcTabClient.Width() - nWidht) / 17 * 4);

	m_DialogProcess.ShowWindow(SW_SHOW);

	GetProcess();

	return TRUE;
}

void CprodoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon_s);
	}
	else
	{
		CDialog::OnPaint();
	}
}

HCURSOR CprodoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon_l);
}

void CprodoDlg::OnTcnSelchangeMainTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	switch(nCurrItem)
	{
	case 0:
		{
			m_DialogProcess.ShowWindow(SW_HIDE);
		}
		break;
	case 1:
		{
			m_DialogDrv.ShowWindow(SW_HIDE);
		}
		break;
	case 2:
		{
			m_DialogSSDT.ShowWindow(SW_HIDE);
		}
		break;
	default:
		break;
	}

	switch( m_TabMain.GetCurSel() )
	{
	case 0:
		{
			m_DialogProcess.ShowWindow(SW_SHOW);
			m_DialogProcess.m_ListProc.DeleteAllItems();
			GetProcess();
		}
		break;
	case 1:
		{
			m_DialogDrv.ShowWindow(SW_SHOW);
			m_DialogDrv.m_ListDrv.DeleteAllItems();
			GetDrv();
		}
		break;
	case 2:
		{
			m_DialogSSDT.ShowWindow(SW_SHOW);
			m_DialogSSDT.m_ListSSDT.DeleteAllItems();
			GetSsdtHook();
		}
		break;
	default:
		break;
	}

	*pResult = 0;
}
