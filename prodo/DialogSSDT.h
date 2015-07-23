#pragma once
#include "afxcmn.h"

#include <string>

class CDialogSSDT : public CDialog
{
	DECLARE_DYNAMIC(CDialogSSDT)
public:
	CDialogSSDT(CWnd* pParent = NULL);

	virtual ~CDialogSSDT();

	enum { IDD = IDD_DIALOG_SSDT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:

	BOOL FixHook(std::string zwFuncName);

	CListCtrl m_ListSSDT;
	afx_msg void OnNMRClickListSsdt(NMHDR *pNMHDR, LRESULT *pResult);
};
