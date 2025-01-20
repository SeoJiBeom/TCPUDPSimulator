#pragma once
#include <afxstr.h>
#define __USER_DEFINE_ADDRESS_H__

struct stCallbackClient
{
	CString strClientID;
	CString strClientIP;
	int nPort;
	CString strConnectTime;
};