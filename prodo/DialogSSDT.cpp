#include "stdafx.h"
#include "prodo.h"
#include "DialogSSDT.h"
#include "CommonUtil.h"
#include "ProdoCmd.h"

#include <WinIoCtl.h>

IMPLEMENT_DYNAMIC(CDialogSSDT, CDialog)

CDialogSSDT::CDialogSSDT(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogSSDT::IDD, pParent)
{
}

CDialogSSDT::~CDialogSSDT()
{
}

BOOL CDialogSSDT::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;
}

void CDialogSSDT::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST_SSDT, m_ListSSDT);
}

BEGIN_MESSAGE_MAP(CDialogSSDT, CDialog)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_SSDT, &CDialogSSDT::OnNMRClickListSsdt)
END_MESSAGE_MAP()

void CDialogSSDT::OnNMRClickListSsdt(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int index = pNMLV->iItem;

	CPoint pt;
	CString szFunc;

	std::string zwFuncName;

	if( index == -1 )
		return;

	HMENU hMenu = ::CreatePopupMenu();
	AppendMenu( hMenu, MF_STRING, 10001, _T("Fix Hook") );

	GetCursorPos(&pt);

	UINT Cmd = (UINT)::TrackPopupMenu( hMenu, TPM_LEFTALIGN|TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL );

	switch( Cmd )
	{
	case 10001:
		{
			szFunc = m_ListSSDT.GetItemText(index, 1);

			UINT nRet;
			CHAR szItem[MAX_PATH] = {0};

			wcstombs_s(&nRet, szItem, szFunc, MAX_PATH);

			zwFuncName.assign(szItem);
			
			if (FixHook(zwFuncName))
			{
				m_ListSSDT.DeleteItem(index);
			}
			else
			{
				MessageBoxA(NULL, "Fix hook failed.","ERROR", MB_OK);
			}
		}
		break;
	}
	*pResult = 0;
}

BOOL CDialogSSDT::FixHook(std::string zwFuncName)
{
	BOOL bResult = FALSE;

	HANDLE hDevice;

	DWORD dwRetSize;

	UINT nFuncIndex;

	std::string ntFuncName;

	CommonUtil util;

	DWORD ntFuncAddr;

	util.getNtFuncAddressByZwFuncName(zwFuncName, ntFuncName, ntFuncAddr);

	util.getSsdtIndexByZwFuncName(zwFuncName, nFuncIndex);

	ARKFIXSSDT FixSsdtHookData;

	FixSsdtHookData.dwOrigAddr = ntFuncAddr;
	FixSsdtHookData.dwSsdtIndex = nFuncIndex;

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
		IOCTL_FIX_SSDTHOOKS,
		&FixSsdtHookData,
		sizeof(ARKFIXSSDT),
		&FixSsdtHookData,
		sizeof(ARKFIXSSDT),
		&dwRetSize, NULL );

	if( FixSsdtHookData.dwOrigAddr != 0 )
	{
		::OutputDebugStringA("DeviceIoControl error\n");
		return FALSE;
	}

	CloseHandle(hDevice);

	return TRUE;
}
