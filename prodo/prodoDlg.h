#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "DialogProcess.h"
#include "DialogSSDT.h"
#include "dialogdrv.h"

class CprodoDlg : public CDialog
{
private:
	BOOL InitDevice();
public:
	CprodoDlg(CWnd* pParent = NULL);

	enum { IDD = IDD_PRODO_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon_l;
	HICON m_hIcon_s;

	virtual BOOL OnInitDialog();

	afx_msg void OnPaint();

	afx_msg void OnTcnSelchangeMainTab(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg HCURSOR OnQueryDragIcon();

	BOOL GetProcess();

	BOOL GetDrv();

	BOOL GetSsdtHook();

	DECLARE_MESSAGE_MAP()
public:
	CTabCtrl       m_TabMain;
	CStatic        m_PromptText;

	CDialogDrv     m_DialogDrv;
	CDialogSSDT    m_DialogSSDT;
	CDialogProcess m_DialogProcess;
};
