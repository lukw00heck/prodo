#pragma once
#include "afxcmn.h"

class CDialogDll : public CDialog
{
	DECLARE_DYNAMIC(CDialogDll)

public:
	CDialogDll(CWnd* pParent = NULL);
	virtual ~CDialogDll();

	enum { IDD = IDD_DIALOG_DLL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();

	virtual void OnSysCommand(UINT nID, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_ListDll;
};
