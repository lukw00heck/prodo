#include "stdafx.h"
#include "prodo.h"
#include "DialogProcess.h"
#include "ProdoCmd.h"

#include <WinIoCtl.h>

IMPLEMENT_DYNAMIC(CDialogProcess, CDialog)

CDialogProcess::CDialogProcess(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogProcess::IDD, pParent)
{
}

CDialogProcess::~CDialogProcess()
{
}

BOOL CDialogProcess::OnInitDialog()
{
	CRect rc;

	CDialog::OnInitDialog();

	m_DialogDll.Create(IDD_DIALOG_DLL, this);

	m_DialogDll.m_ListDll.SetExtendedStyle(
		m_DialogDll.m_ListDll.GetExtendedStyle() | 
		LVS_EX_GRIDLINES | 
		WS_EX_STATICEDGE | 
		LVS_EX_FULLROWSELECT | 
		LVS_EX_GRIDLINES);

	m_DialogDll.m_ListDll.GetClientRect(rc);

	int nWidht = GetSystemMetrics(SM_CXVSCROLL);

	m_DialogDll.m_ListDll.InsertColumn(0, L"Base", LVCFMT_LEFT, (rc.Width() - nWidht) / 5);
	m_DialogDll.m_ListDll.InsertColumn(1, L"Path", LVCFMT_LEFT, (rc.Width() - nWidht) / 5 * 4);

	return TRUE;
}

void CDialogProcess::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PROC, m_ListProc);
}

BEGIN_MESSAGE_MAP(CDialogProcess, CDialog)
	ON_NOTIFY(HDN_ITEMCLICK, 0, &CDialogProcess::OnHdnItemclickListProc)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_PROC, &CDialogProcess::OnNMRClickListProc)
END_MESSAGE_MAP()

void CDialogProcess::OnHdnItemclickListProc(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = 0;
}
void CDialogProcess::OnNMRClickListProc(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int index = pNMLV->iItem;

	CString szCid;
	CPoint pt;

	DWORD dwCid;

	if( index == -1 )
		return;

	HMENU hMenu = ::CreatePopupMenu();
	AppendMenu( hMenu, MF_STRING, 10001, _T("进程模块") );
	AppendMenu( hMenu, MF_STRING, 10002, _T("结束进程") );
	
	GetCursorPos(&pt);

	UINT Cmd = (UINT)::TrackPopupMenu( hMenu, TPM_LEFTALIGN|TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL );

	switch( Cmd )
	{
	case 10001:
		{
			m_DialogDll.ShowWindow(SW_SHOW);

			m_DialogDll.m_ListDll.DeleteAllItems();

			szCid = m_ListProc.GetItemText(index, 0);
			dwCid = _tcstoul(szCid, 0, 10);
			GetDll(dwCid);
		}
		break;
	case 10002:
		{
			szCid = m_ListProc.GetItemText(index, 0);

			dwCid = _tcstoul(szCid, 0, 10);

			if( KillProcess(dwCid) )
			{
				m_ListProc.DeleteItem(index);
			}
			else
			{
				MessageBoxA(NULL, "Kill process failed.","ERROR", MB_OK);
			}
		}
		break;
	}
	*pResult = 0;
}

BOOL CDialogProcess::GetDll(DWORD dwCid)
{
	ARK_DATA_COUNT DataCount = {0};

	BOOL bResult = FALSE;

	DWORD dwRetSize = 0;

	WCHAR pwszAddr[20] = {0};

	WCHAR pwszMsgText[20] = {0};

	WCHAR lpwszPath[MAX_PATH] = {0};

	HANDLE hDevice;

	DataCount.DataType  = ArkDataDllList;
	DataCount.dwMixData = dwCid;

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
	bResult = ::DeviceIoControl(hDevice,
		IOCTL_GET_DATA_CNT,
		&DataCount,
		sizeof(ARK_DATA_COUNT),
		&DataCount,
		sizeof(ARK_DATA_COUNT),
		&dwRetSize, NULL );
	if( !bResult )  
	{
		::OutputDebugStringA("DeviceIoControl error\n");
		return FALSE;
	}
	PARK_DLL pDllArray = new ARK_DLL[ DataCount.nDataCount ];

	if( pDllArray )
	{
		::ZeroMemory( pDllArray, ( sizeof( ARK_DLL ) * DataCount.nDataCount ) );
		bResult = ::DeviceIoControl( hDevice,
			IOCTL_GET_DLLS,
			NULL,
			0,
			pDllArray,
			( sizeof( ARK_DLL ) * DataCount.nDataCount ),
			&dwRetSize, NULL );
		if( !bResult )
		{
			::OutputDebugStringA("DeviceIoControl error\n");
			return FALSE;
		}
	}
	for( UINT i = 0; i < DataCount.nDataCount; i++ )
	{
		m_DialogDll.m_ListDll.InsertItem(i, L"");

		swprintf_s(pwszAddr, 20, L"0x%X", pDllArray[i].baseAddr);

		m_DialogDll.m_ListDll.SetItemText(i, 0, pwszAddr);

		m_DialogDll.m_ListDll.SetItemText(i, 1, pDllArray[i].dllName);
	}
	free(pDllArray);

	CloseHandle(hDevice);

	return TRUE;
}

BOOL CDialogProcess::KillProcess(DWORD dwCid)
{
	BOOL bResult = FALSE;

	HANDLE hDevice;

	ARK_PROCESS ArkProcData = {0};

	DWORD dwRetSize;

	ArkProcData.dwProcId = dwCid;

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
	bResult = ::DeviceIoControl(hDevice,
		IOCTL_KILL_PROCESS,
		&ArkProcData,
		sizeof(ARK_PROCESS),
		&ArkProcData,
		sizeof(ARK_PROCESS),
		&dwRetSize, NULL );

	if( ArkProcData.dwProcId != 0 )
	{
		::OutputDebugStringA("DeviceIoControl error\n");
		return FALSE;
	}

	CloseHandle(hDevice);

	return TRUE;
}
