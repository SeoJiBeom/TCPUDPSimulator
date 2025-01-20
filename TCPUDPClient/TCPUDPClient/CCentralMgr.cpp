#include "pch.h"
#include "CCentralMgr.h"

CCentralMgr::CCentralMgr()
{
	m_pCommTcpMgr = nullptr;
	m_pCommUdpMgr = nullptr;

	m_pCommTcpMgr = new CCommTcpMgr();
	m_pCommUdpMgr = new CCommUdpMgr();
}

CCentralMgr::~CCentralMgr(void)
{
	delete m_pCommTcpMgr;
	delete m_pCommUdpMgr;
}