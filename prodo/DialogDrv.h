#pragma once
#include "afxcmn.h"

class CDialogDrv : public CDialog
{
	DECLARE_DYNAMIC(CDialogDrv)

public:
	CDialogDrv(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDialogDrv();

	enum { IDD = IDD_DIALOG_DRV };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_ListDrv;
};
