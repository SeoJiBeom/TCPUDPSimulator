#include "pch.h"
#include "CCentralMgr.h"

CCentralMgr::CCentralMgr()
{
	m_pCommTcpMgr = nullptr;

	m_pCommTcpMgr = new CCommTcpMgr();
	m_pCommTcpMgr->SetNetInfo(true, _T("127.0.0.1"), 9000);
	m_pCommTcpMgr->Initialize();

	m_pCommUdpMgr = new CCommUdpMgr();
	m_pCommUdpMgr->SetNetInfo(true, _T("127.0.0.1"), 9001);
	m_pCommUdpMgr->Initialize();
}

CCentralMgr::~CCentralMgr(void)
{
	delete m_pCommTcpMgr;
	delete m_pCommUdpMgr;
}