#include "stdafx.h"
#include "prodo.h"
#include "DialogDrv.h"

IMPLEMENT_DYNAMIC(CDialogDrv, CDialog)

CDialogDrv::CDialogDrv(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogDrv::IDD, pParent)
{
	
}

CDialogDrv::~CDialogDrv()
{
}

void CDialogDrv::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DRV, m_ListDrv);
}

BOOL CDialogDrv::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;
}

BEGIN_MESSAGE_MAP(CDialogDrv, CDialog)
END_MESSAGE_MAP()
