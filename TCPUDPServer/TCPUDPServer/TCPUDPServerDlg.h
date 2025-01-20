
// TCPUDPServerDlg.h: 헤더 파일
//

#pragma once


// CTCPUDPServerDlg 대화 상자
class CCentralMgr;
class CTCPUDPServerDlg : public CDialogEx
{
// 생성입니다.
	CCentralMgr* m_pCentralMgr;

public:
	CTCPUDPServerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TCPUDPSERVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;
	CCriticalSection m_csLog;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnDestroy();
	void CreateHorizontalScroll();
	CListCtrl m_listClient;
	CListBox m_listLog;

	//Callback
	static void callback_ConnectClient(LPVOID p1, LPVOID p2);
	void callback_ConnectClient(LPVOID p2);
	static void callback_DisconnectClient(LPVOID p1, LPVOID p2);
	void callback_DisconnectClient(LPVOID p2);
	static void callback_SendLog(LPVOID p1, LPVOID p2);
	void callback_SendLog(LPVOID p2);
};
