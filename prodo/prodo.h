#pragma once

#ifndef __AFXWIN_H__
	#error
#endif

#include "resource.h"

class CprodoApp : public CWinApp
{
public:
	CprodoApp();

public:
	virtual BOOL InitInstance();

private:
	HANDLE m_hSem;

	DECLARE_MESSAGE_MAP()
};

extern CprodoApp theApp;
