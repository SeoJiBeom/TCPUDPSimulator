
// TCPUDPClientDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "TCPUDPClient.h"
#include "TCPUDPClientDlg.h"
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


// CTCPUDPClientDlg 대화 상자



CTCPUDPClientDlg::CTCPUDPClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TCPUDPCLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTCPUDPClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_ID, m_editID);
	DDX_Control(pDX, IDC_BUTTON_LOGIN, m_btnLogin);
	DDX_Control(pDX, IDC_BUTTON_CONNECT, m_btnConnect);
	DDX_Control(pDX, IDC_BUTTON_DISCONNECT, m_btnDisconnect);
	DDX_Control(pDX, IDC_EDIT_MYCHAT, m_editChat);
	DDX_Control(pDX, IDC_BUTTON_SEND_CHAT, m_btnSendChat);
	DDX_Control(pDX, IDC_LIST_CHATBOX, m_listChatBox);
}

BEGIN_MESSAGE_MAP(CTCPUDPClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CTCPUDPClientDlg::OnBnClickedButtonLogin)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CTCPUDPClientDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT, &CTCPUDPClientDlg::OnBnClickedButtonDisconnect)
	ON_BN_CLICKED(IDC_BUTTON_SEND_CHAT, &CTCPUDPClientDlg::OnBnClickedButtonSendChat)
END_MESSAGE_MAP()


// CTCPUDPClientDlg 메시지 처리기

BOOL CTCPUDPClientDlg::OnInitDialog()
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
	m_btnConnect.EnableWindow(false);
	m_btnDisconnect.EnableWindow(false);
	m_btnSendChat.EnableWindow(false);

	m_pCentralMgr = new CCentralMgr();

	((CCommTcpMgr*)m_pCentralMgr->GetCommTcpMgrPtr())->SetCallbackFunction_DisconnectServer(this, callback_DisconnectServer);
	((CCommTcpMgr*)m_pCentralMgr->GetCommTcpMgrPtr())->SetCallbackFunction_PrintChatData(this, callback_PrintChatData);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CTCPUDPClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CTCPUDPClientDlg::OnPaint()
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
HCURSOR CTCPUDPClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



BOOL CTCPUDPClientDlg::PreTranslateMessage(MSG* pMsg)
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


void CTCPUDPClientDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	ShowWindow(SW_HIDE);
	EnableWindow(FALSE);

	delete m_pCentralMgr;
}


void CTCPUDPClientDlg::OnBnClickedButtonLogin()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString strText;
	m_editID.GetWindowText(strText); // 아이디 입력했는지 확인

	if (!strText.IsEmpty())
	{
		m_editID.EnableWindow(false);
		m_btnLogin.EnableWindow(false);
		m_btnConnect.EnableWindow(true);
		m_btnDisconnect.EnableWindow(false);
	}
}


void CTCPUDPClientDlg::OnBnClickedButtonConnect()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString strClientID;
	m_editID.GetWindowText(strClientID); // 아이디 입력했는지 확인

	if (((CCommTcpMgr*)m_pCentralMgr->GetCommTcpMgrPtr())->Initialize(strClientID))
	{
		m_btnConnect.EnableWindow(false);
		m_btnDisconnect.EnableWindow(true);
		m_btnSendChat.EnableWindow(true);

		((CCommUdpMgr*)m_pCentralMgr->GetCommUdpMgrPtr())->Initialize();
	}
}


void CTCPUDPClientDlg::OnBnClickedButtonDisconnect()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	((CCommTcpMgr*)m_pCentralMgr->GetCommTcpMgrPtr())->DisconnectClient();

	m_btnConnect.EnableWindow(true);
	m_btnDisconnect.EnableWindow(false);
	m_btnSendChat.EnableWindow(false);
}

void CTCPUDPClientDlg::OnBnClickedButtonSendChat()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString strClientID, strChatData;
	m_editID.GetWindowText(strClientID);
	m_editChat.GetWindowText(strChatData);

	((CCommTcpMgr*)m_pCentralMgr->GetCommTcpMgrPtr())->SendChatData(strClientID, strChatData);
}

//Callback Function
void CTCPUDPClientDlg::callback_DisconnectServer(LPVOID p1, LPVOID p2)
{
	CTCPUDPClientDlg* pThis = (CTCPUDPClientDlg*)p1;
	if (pThis)
	{
		pThis->callback_DisconnectServer(p2);
	}
}

void CTCPUDPClientDlg::callback_DisconnectServer(LPVOID p2)
{
	m_btnConnect.EnableWindow(true);
	m_btnDisconnect.EnableWindow(false);
	m_btnSendChat.EnableWindow(false);
}

void CTCPUDPClientDlg::callback_PrintChatData(LPVOID p1, LPVOID p2)
{
	CTCPUDPClientDlg* pThis = (CTCPUDPClientDlg*)p1;
	if (pThis)
	{
		pThis->callback_PrintChatData(p2);
	}
}

void CTCPUDPClientDlg::callback_PrintChatData(LPVOID p2)
{
	stCallbackChat stChat = *(stCallbackChat*)p2;

	CString strChat;
	strChat.Format(_T("%s : %s"), stChat.strClientID, stChat.strChatData);
	m_listChatBox.AddString(strChat);
}