#pragma once
#include "afxcmn.h"
#include "dialogdll.h"

class CDialogProcess : public CDialog
{
	DECLARE_DYNAMIC(CDialogProcess)

public:
	CDialogProcess(CWnd* pParent = NULL);
	virtual ~CDialogProcess();

	enum { IDD = IDD_DIALOG_PROC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	virtual BOOL OnInitDialog();

	BOOL GetDll(DWORD dwCid);
	BOOL KillProcess(DWORD dwCid);

	DECLARE_MESSAGE_MAP()
public:

	CListCtrl m_ListProc;
	afx_msg void OnHdnItemclickListProc(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickListProc(NMHDR *pNMHDR, LRESULT *pResult);
	CDialogDll m_DialogDll;
};
