// TCPUDPClientDlg.h: 헤더 파일
//

#pragma once

// CTCPUDPClientDlg 대화 상자
class CCentralMgr;
class CTCPUDPClientDlg : public CDialogEx
{
// 생성입니다.
	CCentralMgr* m_pCentralMgr;

public:
	CTCPUDPClientDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TCPUDPCLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonLogin();
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonDisconnect();
	afx_msg void OnBnClickedButtonSendChat();

	CEdit m_editID;
	CButton m_btnLogin;
	CButton m_btnConnect;
	CButton m_btnDisconnect;
	CEdit m_editChat;
	CButton m_btnSendChat;
	CListBox m_listChatBox;

	//Callback
	static void callback_DisconnectServer(LPVOID p1, LPVOID p2);
	void callback_DisconnectServer(LPVOID p2);
	static void callback_PrintChatData(LPVOID p1, LPVOID p2);
	void callback_PrintChatData(LPVOID p2);
};
