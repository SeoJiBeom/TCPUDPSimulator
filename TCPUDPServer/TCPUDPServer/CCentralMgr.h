#pragma once
#include "CCommTcpMgr.h"
#include "CCommUdpMgr.h"

class CCommTcpMgr;
class CCommUdpMgr;
class CCentralMgr
{
	CCommTcpMgr* m_pCommTcpMgr;
	CCommUdpMgr* m_pCommUdpMgr;

public:
	CCentralMgr();
	~CCentralMgr(void);

	CCommTcpMgr* GetCommTcpMgrPtr() const { return m_pCommTcpMgr; }
	CCommUdpMgr* GetCommUdpMgrPtr() const { return m_pCommUdpMgr; }
};

