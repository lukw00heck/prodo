#include "stdafx.h"
#include "prodo.h"
#include "DialogDll.h"

IMPLEMENT_DYNAMIC(CDialogDll, CDialog)

CDialogDll::CDialogDll(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogDll::IDD, pParent)
{
}

CDialogDll::~CDialogDll()
{
}

void CDialogDll::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DLL, m_ListDll);
}

BOOL CDialogDll::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;
}

void CDialogDll::OnSysCommand(UINT nID, LPARAM lParam)
{
	if( (nID & 0xFFF0) == SC_CLOSE ) 
	{ 
		ShowWindow(SW_HIDE); 
		return; 
	} 
}

BEGIN_MESSAGE_MAP(CDialogDll, CDialog)
END_MESSAGE_MAP()
