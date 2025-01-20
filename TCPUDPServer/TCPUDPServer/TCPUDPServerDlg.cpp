
// TCPUDPServerDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "TCPUDPServer.h"
#include "TCPUDPServerDlg.h"
#include "CCentralMgr.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTCPUDPServerDlg 대화 상자



CTCPUDPServerDlg::CTCPUDPServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TCPUDPSERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTCPUDPServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CLIENT, m_listClient);
	DDX_Control(pDX, IDC_LIST_LOG, m_listLog);
}

BEGIN_MESSAGE_MAP(CTCPUDPServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
ON_WM_DESTROY()
END_MESSAGE_MAP()


// CTCPUDPServerDlg 메시지 처리기

BOOL CTCPUDPServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	m_listClient.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 120);
	m_listClient.InsertColumn(1, _T("IP"), LVCFMT_LEFT, 120);
	m_listClient.InsertColumn(2, _T("Port"), LVCFMT_LEFT, 60);
	m_listClient.InsertColumn(3, _T("접속 시간"), LVCFMT_LEFT, 150);
	m_listClient.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	CreateHorizontalScroll();

	m_pCentralMgr = new CCentralMgr();

	((CCommTcpMgr*)m_pCentralMgr->GetCommTcpMgrPtr())->SetCallbackFunction_ConnectClient(this, callback_ConnectClient);
	((CCommTcpMgr*)m_pCentralMgr->GetCommTcpMgrPtr())->SetCallbackFunction_DisconnectClient(this, callback_DisconnectClient);
	((CCommTcpMgr*)m_pCentralMgr->GetCommTcpMgrPtr())->SetCallbackFunction_SendLogTCP(this, callback_SendLog);
	((CCommUdpMgr*)m_pCentralMgr->GetCommUdpMgrPtr())->SetCallbackFunction_SendLogUDP(this, callback_SendLog);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CTCPUDPServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CTCPUDPServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CTCPUDPServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CTCPUDPServerDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		//다이얼로그 종료 방지
		switch (pMsg->wParam)
		{
		case VK_ESCAPE:
		case VK_RETURN:
		case VK_SPACE:
		case VK_LEFT:
		case VK_UP:
		case VK_RIGHT:
		case VK_DOWN:
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CTCPUDPServerDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	ShowWindow(SW_HIDE);
	EnableWindow(FALSE);

	delete m_pCentralMgr;
}

void CTCPUDPServerDlg::CreateHorizontalScroll()
{
	CString str; CSize sz; int dx = 0;
	CDC* pDC = m_listLog.GetDC();

	for (int i = 0; i < m_listLog.GetCount(); i++)
	{
		m_listLog.GetText(i, str);
		sz = pDC->GetTextExtent(str);

		if (sz.cx > dx)
			dx = sz.cx;
	}
	m_listLog.ReleaseDC(pDC);

	if (m_listLog.GetHorizontalExtent() < dx)
	{
		m_listLog.SetHorizontalExtent(dx);
		ASSERT(m_listLog.GetHorizontalExtent() == dx);
	}
}

//Callback Function
void CTCPUDPServerDlg::callback_ConnectClient(LPVOID p1, LPVOID p2)
{
	CTCPUDPServerDlg* pThis = (CTCPUDPServerDlg*)p1;
	if (pThis)
	{
		pThis->callback_ConnectClient(p2);
	}
}

void CTCPUDPServerDlg::callback_ConnectClient(LPVOID p2)
{
	stCallbackClient stClient = *(stCallbackClient*)p2;

	int index = m_listClient.GetItemCount();

	m_listClient.InsertItem(index, stClient.strClientID);			// 클라이언트 ID 추가

	m_listClient.SetItemText(index, 1, stClient.strClientIP);		// IP 주소 추가

	CString strPort;
	strPort.Format(_T("%d"), stClient.nPort);
	m_listClient.SetItemText(index, 2, strPort);					// 포트 번호 추가

	m_listClient.SetItemText(index, 3, stClient.strConnectTime);	// 연결 시간 추가
}

void CTCPUDPServerDlg::callback_DisconnectClient(LPVOID p1, LPVOID p2)
{
	CTCPUDPServerDlg* pThis = (CTCPUDPServerDlg*)p1;
	if (pThis)
	{
		pThis->callback_DisconnectClient(p2);
	}
}

void CTCPUDPServerDlg::callback_DisconnectClient(LPVOID p2)
{
	stCallbackClient strClient = *(stCallbackClient*)p2;

	CString strPort;
	strPort.Format(_T("%d"), strClient.nPort);

	// 리스트뷰의 항목 개수 가져오기
	int itemCount = m_listClient.GetItemCount();

	for (int i = itemCount - 1; i >= 0; --i) {
		// 리스트에서 소켓 정보를 검색
		CString clientIP = m_listClient.GetItemText(i, 1);
		CString clientPort = m_listClient.GetItemText(i, 2);

		if (clientIP == strClient.strClientIP && clientPort == strPort) {
			m_listClient.DeleteItem(i); // 항목 삭제
			break;
		}
	}
}

void CTCPUDPServerDlg::callback_SendLog(LPVOID p1, LPVOID p2)
{
	CTCPUDPServerDlg* pThis = (CTCPUDPServerDlg*)p1;
	if (pThis)
	{
		pThis->callback_SendLog(p2);
	}
}

void CTCPUDPServerDlg::callback_SendLog(LPVOID p2)
{
	CString strData = *(CString*)p2;

	CTime currentTime = CTime::GetCurrentTime();
	CString strTime = currentTime.Format("[%Y-%m-%d %H:%M:%S]");

	CString strLog;
	strLog.Format(_T("%s %s"), strTime, strData);	

	m_csLog.Lock();
	m_listLog.AddString(strLog);
	m_csLog.Unlock();
}