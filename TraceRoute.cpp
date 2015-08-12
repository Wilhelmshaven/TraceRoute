/*************************************************************
*   I declare that the assignment here submitted is original *
* except for source material explicitly acknowledged. I also *
* acknowledge that I am aware of University policy and       *
* regulations on honesty in academic work, and of the        *
* disciplinary guidelines and procedures applicable to       *
* breaches of such policy and regulations.                   *
*                                                            *
* Hongjie Li                    2014.11.13                   *
* Signature						Date                         *
*************************************************************/
// TraceRoute.cpp : ����Ӧ�ó������ڵ㡣
//
#include "stdafx.h"
#include "TraceRoute.h"
#include "Packet.h"
#include "Device.h"

//���任
#pragma comment(linker, "\"/manifestdependency:type='Win32'\
 name='Microsoft.Windows.Common-Controls' version='6.0.0.0'\
 processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// ȫ�ֱ���: 
enum CustomDefine
{
	MAX_LOADSTRING = 100,
	MAX_ROUTER = 30          // �Զ����������
};
enum IPDefine
{
	ETH_IP = 0x0800,         // IP���ݰ�
	TCP = 0x06,              // IP���ݱ���TCPЭ��
	UDP = 0x11               // IP���ݱ���UDPЭ��
};
enum ARPDefine
{
	ETH_ARP = 0x0806,        // ARP���ݰ�
	ARP_HARDWARE = 1,        // Ӳ�������ֶ�ֵ����ʾ��̫����ַ
	ARP_REQUEST = 1,         // ARP�����ֶ�
	ARP_REPLY = 2            // ARPӦ���ֶ�
};
enum ICMPDefine
{
	ICMP = 0x01,             // IP���ݱ���ICMPЭ��
	ICMP_REQ = 0x08,         // ICMP ECHO REQUEST
	ICMP_REPLY = 0x00,       // ICMP ECHO REPLY
	ICMP_TIME_EXP = 0x0b     // ICMP TIME EXPIRED
};                   
HINSTANCE hInst;								// ��ǰʵ��
TCHAR szTitle[MAX_LOADSTRING];					// �������ı�
TCHAR szWindowClass[MAX_LOADSTRING];			// ����������
Device myDevice;                                // �豸��            
HANDLE hBeginEvent = CreateEvent(NULL, TRUE, FALSE, NULL);       // ��ʼ�¼�
HANDLE hFinishEvent = CreateEvent(NULL, TRUE, FALSE, NULL);      // �����¼�
HANDLE hMaxHopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
HANDLE hMutex;// = CreateMutex(NULL, FALSE, NULL); // ʹ�û����������̵߳Ľ�������
HWND myhdlg = NULL;                             // �Ի������������ڣ��ľ��
sparam sp;                                      // ��װ������
int TTL = 0;                                    // TTL����ֵ��ͬʱҲ��������
char *realIP = new char[16];                    // �洢IP������IP������������������IP��
BOOL HaveMAC = FALSE;
int SelectedNIC;
// �˴���ģ���а����ĺ�����ǰ������: 
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);                                //��������Ϣ����
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);                                  //���ڴ�����Ϣ����
INT_PTR CALLBACK    DlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);         //�Ӵ�����Ϣ����
BOOL AddListViewItems(HWND hwndListView, int hop, double time, char *ip_add);           //�ѽ�������ListView��
BOOL CheckInput(char *input, int len);                                                  //�������Ϸ���
UINT SendPacket(LPVOID lpParameter);                                                    //��������
UINT AnalyzePacket(LPVOID lpParameter);                                                 //�հ�����

HANDLE sendThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendPacket, NULL, 0, NULL);     // �����߳�
HANDLE recvThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AnalyzePacket, NULL, 0, NULL);  // �հ��߳�

/*============================== WinMain ==============================*/
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO:  �ڴ˷��ô��롣
	MSG msg;
	HACCEL hAccelTable;

	// ��ʼ��ȫ���ַ���
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TRACEROUTE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// ִ��Ӧ�ó����ʼ��: 
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TRACEROUTE));

	// ����Ϣѭ��: 
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//ע�ᴰ���ࡣ
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TRACEROUTE));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TRACEROUTE);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

// ����ʵ�����������������
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

   // �����ڲ��ɱ��С��ͬʱ�������
   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
	   CW_USEDEFAULT, 0, 620, 500, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

// ���������ڵ���Ϣ��
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	//HWND myhdlg = NULL;

	switch (message)
	{
	case WM_CREATE:
	{
		// �����ӶԻ��򲢽�����Ϊ������
		myhdlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_FORMVIEW), hWnd, (DLGPROC)DlgProc);
		ShowWindow(myhdlg, SW_SHOW);// ��ʾ�Ի���

		// ���ñ���������ʽ
		LOGFONT TitleFont;
		ZeroMemory(&TitleFont, sizeof(TitleFont));                    // �����������������߰���ĳ�ֵ
		lstrcpy(TitleFont.lfFaceName, "Segoe Script");                // ��������
		TitleFont.lfWeight = FW_BOLD;                                 // ��ϸ��BOLD=700��д��CSS��֪��
		TitleFont.lfHeight = -24;                                     // �����С��������н�������
		TitleFont.lfCharSet = DEFAULT_CHARSET;                        // Ĭ���ַ���
		TitleFont.lfOutPrecision = OUT_DEVICE_PRECIS;                 // �������

		HFONT hFont = CreateFontIndirect(&TitleFont);
		HWND hWndStatic = GetDlgItem(myhdlg, IDC_TITLE);
		SendMessage(hWndStatic, WM_SETFONT, (WPARAM)hFont, 0);

		//������Ŀ������ʽ
		LOGFONT TextFont;
		ZeroMemory(&TextFont, sizeof(TextFont));
		lstrcpy(TextFont.lfFaceName, "Gabriola");
		TextFont.lfHeight = -18;
		hFont = CreateFontIndirect(&TextFont);

		//���ÿؼ�����
		hWndStatic = GetDlgItem(myhdlg, IDC_TEXT_1);
		SendMessage(hWndStatic, WM_SETFONT, (WPARAM)hFont, 0);
		hWndStatic = GetDlgItem(myhdlg, IDC_TEXT_2);
		SendMessage(hWndStatic, WM_SETFONT, (WPARAM)hFont, 0);
	}//WM_CREATE

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// �����˵�ѡ��: 
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// �����ڡ������Ϣ�������
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// ����Ի�����Ϣ  
INT_PTR CALLBACK DlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	HWND hListview = GetDlgItem(hdlg, IDC_LIST1);       // ListView
	HWND hButton = NULL;                                // Button
	HWND hEditBox = GetDlgItem(hdlg, IDC_INPUT);        // Editbox
	HWND hWndComboBox = GetDlgItem(hdlg, IDC_COMBO1);   // ComboBox

	switch (msg)
	{
	case WM_INITDIALOG:
	{
		// �����ı����������볤��
		Edit_LimitText(hEditBox, 32);

		// ����һ����ʾ�ı����漰ANSIת���ַ�������
		char *tmp = new char[64];
		strcpy(tmp, "������IP��ַ�磺1.1.1.1");                         // ������ʾ�ı�
		int dwNum = MultiByteToWideChar(CP_ACP, 0, tmp, -1, NULL, 0);     // �����Ҫת�ɵĿ��ַ��ĳ���
		wchar_t *tip = new wchar_t[dwNum];                                // ����һ���õ��ĳ��Ƚ��г�ʼ��
		MultiByteToWideChar(CP_ACP, 0, tmp, -1, tip, dwNum);              // ���ֽ�ת���ɿ��ֽڣ�sizeof����ʹ��˵
		Edit_SetCueBannerText(hEditBox, tip);                             // �����ʾ�ı�������Ҫ���ı�ΪUnicode��ʽ��
		delete[]tmp;
		delete[]tip;

		// ���Listview����������������

		// ����ListView����  
		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		ListView_SetTextColor(hListview, RGB(0, 0, 255));                // ����������ɫ
		ListView_SetTextBkColor(hListview, RGB(199, 237, 204));          // �������ֱ�����ɫ
		ListView_SetExtendedListViewStyle(hListview, LVS_EX_GRIDLINES);  // ��ӵ�����

		lvc.pszText = "Hop";                        // �б���  
		lvc.cx = 0;                                 // �п�  
		lvc.iSubItem = 0;                           // ������������һ�������� (0) 
		lvc.fmt = LVCFMT_CENTER;
		ListView_InsertColumn(hListview, 0, &lvc);  // �����������һ��ռλ���Ա����
		lvc.cx = 40;
		ListView_InsertColumn(hListview, 1, &lvc);  // ��һ��

		lvc.pszText = "Times(ms)";
		lvc.cx = 100;
		lvc.iSubItem = 1;                           // ��������  
		lvc.fmt = LVCFMT_CENTER;
		ListView_InsertColumn(hListview, 2, &lvc);

		lvc.pszText = "IP Address";
		lvc.cx = 120;
		lvc.iSubItem = 2;
		lvc.fmt = LVCFMT_CENTER;
		ListView_InsertColumn(hListview, 3, &lvc);

		lvc.pszText = "Router Name";
		lvc.cx = 280;
		lvc.iSubItem = 3;
		lvc.fmt = LVCFMT_CENTER;
		ListView_InsertColumn(hListview, 4, &lvc);

		// �������б������Ŀ
		pcap_if_t *d;
		for (d = myDevice.alldevs; d; d = d->next)
		{
			SendMessage(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)d->description);
		}

		break;
	}//WM_INITIALIZE

	case WM_CREATE:
	{
		//������ť
		hButton = CreateWindow(
			"BUTTON",                                               // Predefined class; Unicode assumed 
			"OK",                                                   // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
			100,                                                    // x position 
			100,                                                    // y position 
			100,                                                    // Button width
			100,                                                    // Button height
			hdlg,                                                   // Parent window
			(HMENU)IDC_BTN_TRACE,                                   // No menu.
			(HINSTANCE)GetWindowLong(hdlg, GWL_HINSTANCE),
			NULL);                                                  // Pointer not needed.

		hButton = CreateWindow(
			"BUTTON", "OK",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			100, 100, 100, 100, hdlg, (HMENU)IDC_BTN_ABOUT,
			(HINSTANCE)GetWindowLong(hdlg, GWL_HINSTANCE), NULL);

		hButton = CreateWindow(
			"BUTTON", "OK",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			100, 100, 100, 100, hdlg, (HMENU)IDC_BTN_QUIT,
			(HINSTANCE)GetWindowLong(hdlg, GWL_HINSTANCE), NULL);

		break;
	}// WM_CREATE

	case WM_COMMAND:
	{
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
			// ��ť���ܵ�ʵ��
		case IDC_BTN_TRACE:
		{
			// �����û��Ҹ㣬���ð�ť
			hButton = GetDlgItem(hdlg, IDC_BTN_TRACE);
			Button_Enable(hButton, FALSE);
			Edit_Enable(hEditBox, FALSE);

			// �����Ҫ��յ���Ϣ������һ�ε�ɨ�����������������λ����
			SendMessage(hListview, LVM_DELETEALLITEMS, 0, 0);
			TTL = 0;

			// ��ȡ���벢�������Ϸ���
			char *input = new char[64];		
			BOOL chk = FALSE;
			int len = 0;

			ZeroMemory(input, 64);
			Edit_GetText(hEditBox, input, 64);
			len = Edit_GetTextLength(hEditBox);

			if (len != 0)
			{
			    // �Ϸ��Լ�飺���������������ܹ�����IP������IP���������IP��ʽ
				chk = CheckInput(input, len);
			}
			if (!chk || SelectedNIC < 0)
			{
				// ���Ϸ�����������
				MessageBox(hdlg, (LPCSTR)"�������벻�Ϸ���δѡ����ʵ����������飡", NULL, MB_OK);
				Button_Enable(hButton, TRUE);
				Edit_Enable(hEditBox, TRUE);

				delete []input;

				break;
			}
			else
			{
				// ��ͨ���Ϸ��Լ�飬��ʼ�ɻ
				hMutex = CreateMutex(NULL, FALSE, NULL);

				// �ѽ���������IP��ʾ�������
				sprintf_s(input, 64, "%s(%s)", input, realIP);
				Edit_SetText(hEditBox, input);				

				//������������
				myDevice.DeviceGetReady(SelectedNIC);
				sp.dest_ip = realIP;
				sp.handle = hListview;

				// ��λ
				ResetEvent(hMaxHopEvent);
				// ���ÿ�ʼ�¼�
				SetEvent(hBeginEvent);		

				delete []input;
				break;
			}
		}//IDC_BTN_TRACE

		case IDC_BTN_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hdlg, About);
			break;

		case IDC_BTN_QUIT:
			//�����¼�����ֹ�߳�
			SetEvent(hFinishEvent);
			SetEvent(hBeginEvent);
			PostQuitMessage(0);
			break;

		default:
			break;
		}// wmID

		//����ؼ���Ϣ
		switch (wmEvent)
		{
			// �����б�ѡ�����仯
		case CBN_SELCHANGE:
		{
			SelectedNIC = -1;
			SelectedNIC = (int)SendMessage(hWndComboBox, CB_GETCURSEL, 0, 0); // ���ѡ�е�ѡ����
			break;
		}
		default:
			break;
		}

		break;
	}// WM_COMMAND
	}// msg

	return (INT_PTR)FALSE;
}

// �������Ϸ���
BOOL CheckInput(char *input, int len)
{
	ZeroMemory(realIP, 16);

	// �ȴ��������������ת��IP������IP�жϺ������������������Ǻ���ӵģ��ⲿ����һ���������
	addrinfo hostInfo;
	addrinfo *res = NULL;
	addrinfo *cur = NULL;
	sockaddr_in *addr;
	int errCode;

	ZeroMemory(&hostInfo, sizeof(hostInfo));
	hostInfo.ai_family = AF_INET;               /* Allow IPv4 */
	hostInfo.ai_flags = AI_PASSIVE;             /* For wildcard IP address */
	hostInfo.ai_protocol = 0;                   /* Any protocol */
	hostInfo.ai_socktype = SOCK_STREAM;

	// �����ʼ��Winsock
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(1, 1);
	WSAStartup(wVersionRequested, &wsaData);

	errCode = getaddrinfo(input, NULL, &hostInfo, &res);

	// ��ȷ�᷵��0������Դ�ǿ�
	if ((errCode == 0) && (res != NULL))
	{
		for (cur = res; cur != NULL; cur = cur->ai_next)
		{
			addr = (sockaddr_in *)cur->ai_addr;
			inet_ntop(AF_INET, &addr->sin_addr, realIP, 16);
		}
	}

	freeaddrinfo(cur);
	freeaddrinfo(res);

	/*��������������������Ѿ�ת����IP��������IP��������������IP�����Ƿ��������Ƿ������ͷǷ�IP���Լ��Ϸ����ǲ����ڵ�IP����
	��realIPһ����NULL��Ҳ����˵���������������ķ����������ԭ�еĲ������룬���Ƿ���*/
	if (*realIP == NULL)return FALSE;
	else return TRUE;

}

// �����̣߳�����TTL������UDP��
UINT SendPacket(LPVOID lpParameter)
{
	while (1)
	{
		Beep(440, 200);
		// ��û������
		WaitForSingleObject(hMutex, INFINITE);

		WaitForSingleObject(hBeginEvent, INFINITE);
		// ��������й����е��˳�������ֹ
		if (WaitForSingleObject(hFinishEvent, 0) == WAIT_OBJECT_0)break;
		Beep(440, 200);

		BYTE *sendbuf = new BYTE[74];                      // �����С
		ICMP_frame ICMPFrame;

		char *xip = sp.dest_ip;
		ZeroMemory(sendbuf, 74);

		if (!HaveMAC)
		{
			Beep(660, 100);
			// ���ȷ���ARP����ȡ����MAC��ַ
			ARP_frame ARPFrame;

			// �������
			memcpy(ARPFrame.eh.source_mac_add, myDevice.mac, 6);
			//memset(ARPFrame.eh.dest_mac_add, 0xff, 6);         //MAC�Ĺ㲥��ַΪFF-FF-FF-FF-FF-FF
			ARPFrame.eh.type = htons(ETH_ARP);                 // ��̫��֡ͷЭ������

			ARPFrame.ah.hardware_type = htons(ARP_HARDWARE);   // Ӳ����ַ
			ARPFrame.ah.protocol_type = htons(ETH_IP);         // ARP��Э������
			ARPFrame.ah.source_ip_add = inet_addr(myDevice.ip);         // ���󷽵�IP��ַΪ�����IP��ַ	
			memcpy(ARPFrame.ah.source_mac_add, myDevice.mac, 6);
			ARPFrame.ah.operation_field = htons(ARP_REQUEST);  //ARP�����
			ARPFrame.ah.dest_ip_add = inet_addr(myDevice.gateway_ip);   // Ŀ��IP��дΪ����IP

			// �����õ����ݰ�װ�뻺��
			memset(sendbuf, 0, sizeof(sendbuf));
			memcpy(sendbuf, &ARPFrame, sizeof(ARPFrame));

			pcap_sendpacket(myDevice.adhandle, sendbuf, 60);            // ����

			// ���հ��̸߳ɻ�
			ReleaseMutex(hMutex);
			WaitForSingleObject(hMutex, INFINITE);
		}

		// �������MAC��ַ����д������ICMP���ݱ�
		while (true)
		{
			TTL++;         //TTL+1
			// ����������ܴ����Զ����������MAX_ROUTER
			if (TTL > MAX_ROUTER)
			{
				SetEvent(hMaxHopEvent);
				break;
			}

			ZeroMemory(sendbuf, 74);
			// �������
			// ��̫��ͷ��
			memcpy(ICMPFrame.eh.source_mac_add, myDevice.mac, 6);        // ԴMAC��ַΪ�Լ���MAC��ַ
			memcpy(ICMPFrame.eh.dest_mac_add, myDevice.gatewayMAC, 6);   // Ŀ��MAC��ַΪ���ص�MAC��ַ
			ICMPFrame.eh.type = htons(ETH_IP);                           // ��̫��֡ͷЭ������

			// IPͷ��
			ICMPFrame.ih.versionAndIHL = 0x45;                           // IPV4Ϊ4������0101��20�ֽڣ�
			ICMPFrame.ih.service = 0x00;                                 // ���ַ������㼴��
			ICMPFrame.ih.length = htons(0x003C);                         // �ܳ�����Ϊ60
			ICMPFrame.ih.id = htons(GetCurrentProcessId());              // ID����ϵͳ���
			ICMPFrame.ih.flagAndOffset = 0x00;                           // ��־��ƫ�ƣ����㼴��
			ICMPFrame.ih.TTL = TTL;                                      // ��̬����TTL
			ICMPFrame.ih.protocol = ICMP;                                // Э��������ΪICMP
			ICMPFrame.ih.checksum = 0x0000;                              // У������㣬�����ͷ�������֮
			ICMPFrame.ih.source_add = inet_addr(myDevice.ip);            // ԴIP��ַΪ�Լ���IP
			ICMPFrame.ih.dest_add = inet_addr(xip);                      // Ŀ��IP��ַ�����û�����
			// IPͷ��У���
			char *cksbuf = new char[20];
			memcpy(cksbuf, &ICMPFrame.ih, 20);
			ICMPFrame.ih.checksum = ICMPFrame.cks((WORD *)cksbuf, 20);

			// ICMP����
			ICMPFrame.ICMPHead.type = ICMP_REQ;                          // ECHO
			ICMPFrame.ICMPHead.code = 0x00;                              // ECHO��code��������
			ICMPFrame.ICMPHead.checksum = 0x0000;                        // �����㣬���������
			ICMPFrame.ICMPHead.data = 0x0c000100;                        // �����˵����
			// ����ICMP��У���
			memcpy(cksbuf, &ICMPFrame.ICMPHead, 8);
			ICMPFrame.ICMPHead.checksum = ICMPFrame.cks((WORD *)cksbuf, 8);
			delete[]cksbuf;

			// �����õ����ݰ�װ�뻺��
			memset(sendbuf, 0, sizeof(sendbuf));
			memcpy(sendbuf, &ICMPFrame, sizeof(ICMPFrame));

			pcap_sendpacket(myDevice.adhandle, sendbuf, 74);            // ����

			// ���հ��̸߳ɻ�
			ReleaseMutex(hMutex);
			//Beep(660, 200);
			// ��δ��ֹ����ȴ��հ��߳��յ����ݱ����������
			WaitForSingleObject(hMutex, INFINITE);

			// ����Ѿ��㶨���¼�״̬�ͻᱻ�ı䣬����ֹ
			if (WaitForSingleObject(hFinishEvent, 0) == WAIT_OBJECT_0 || WaitForSingleObject(hMaxHopEvent, 0) == WAIT_OBJECT_0)break;
		}

		delete[]sendbuf;
		ResetEvent(hBeginEvent);
	}

	return 0;
}

/* �������������ݰ��е�ICMP���ݱ� */
UINT AnalyzePacket(LPVOID lpParameter)
{
	while (1)
	{		
		Beep(880, 200);
		WaitForSingleObject(hBeginEvent, INFINITE);		
		// ��������й����е��˳�������ֹ
		if (WaitForSingleObject(hFinishEvent, 0) == WAIT_OBJECT_0)break;
		
		char *xip = sp.dest_ip;                   // �յ�IP
		char *source_ip = new char[16];           // ����������ݰ��е�ԴIP
		char *dest_ip = new char[16];             // ����������ݰ��е�Ŀ��IP
		double delayTime = 0;                     // �ӳ�ʱ��
		char *hop_ip = new char[16];              // ·��IP
		int res;                                  // ������
		pcap_pkthdr * pkt_header;
		const u_char * pkt_data;
		ICMP_frame *recvICMP;                     // ���������������ICMP�ṹ
		int flag = 0;
		int TimeOut = 0;                          // ��ʱ������

		// �ȴ�������󱻷����߳��ͷţ���ȡARPӦ���
		WaitForSingleObject(hMutex, INFINITE);
		Beep(880, 200);

		if (!HaveMAC)
		{
			Beep(660, 100);
			while (true)
			{
				if ((res = pcap_next_ex(myDevice.adhandle, &pkt_header, &pkt_data)) > 0)// ʹ�÷ǻص������������ݰ�
				{
					if (*(WORD *)(pkt_data + 12) == htons(ETH_ARP))// �ж�ARP���ĵ�13,14λ��Type���Ƿ����0x0806��Ŀ�����˳�ARP��			
					{
						// ��������װ��ARP֡�ṹ
						ARP_frame *recvARP = (ARP_frame *)pkt_data;

						// ��ʽ��IP�Խ��бȽ�
						sprintf_s(source_ip, 16, "%d.%d.%d.%d", recvARP->ah.source_ip_add & 255, recvARP->ah.source_ip_add >> 8 & 255,
							recvARP->ah.source_ip_add >> 16 & 255, recvARP->ah.source_ip_add >> 24 & 255);

						// �жϲ�����λ�Ƿ���ARP_REPLY�����˳�ARPӦ�����ȷ�������ش𸴵�ARP��
						if (recvARP->ah.operation_field == htons(ARP_REPLY) && (strcmp(source_ip, myDevice.gateway_ip) == 0))
						{
							// �����ȡ����MAC��ַ
							sprintf_s(myDevice.gatewayMACStr, 18, "%02X-%02X-%02X-%02X-%02X-%02X", recvARP->ah.source_mac_add[0],
								recvARP->ah.source_mac_add[1], recvARP->ah.source_mac_add[2], recvARP->ah.source_mac_add[3],
								recvARP->ah.source_mac_add[4], recvARP->ah.source_mac_add[5]);

							u_char *p;
							p = (u_char *)&recvARP->ah.source_mac_add;// �ⲿ��ͨ��ָ�����͵ĸı�ʵ����ת������
							for (int i = 0; i < 6; i++)myDevice.gatewayMAC[i] = p[i];

							// ֪ͨ�����̸߳ɻ���ȴ�
							HaveMAC = TRUE;
							ReleaseMutex(hMutex);
							WaitForSingleObject(hMutex, INFINITE);

							break;// BREAK APR RECV.
						}
					}
				}
			}
		}// ARP

		while (true)
		{
			// ��������й����е��˳�������ֹ
			if (WaitForSingleObject(hFinishEvent, 0) == WAIT_OBJECT_0 || WaitForSingleObject(hMaxHopEvent, 0) == WAIT_OBJECT_0)break;

			TimeOut++;
			if ((res = pcap_next_ex(myDevice.adhandle, &pkt_header, &pkt_data)) > 0)                  // ʹ�÷ǻص������������ݰ�
			{
				if (*(WORD *)(pkt_data + 12) == htons(ETH_IP) && (*(BYTE *)(pkt_data + 23) == ICMP))  // ѡ�������Լ���ICMP��ʱ�䳬������
				{
					// ��������װ��ICMP�ṹ��
					recvICMP = (ICMP_frame *)pkt_data;

					// ��ʽ��IP�Խ��бȽ�
					sprintf_s(dest_ip, 16, "%d.%d.%d.%d", recvICMP->ih.dest_add & 255, recvICMP->ih.dest_add >> 8 & 255,
						recvICMP->ih.dest_add >> 16 & 255, recvICMP->ih.dest_add >> 24 & 255);

					if (strcmp(dest_ip, myDevice.ip) == 0)                        // ѡ���Լ���ICMP��
					{
						// ����ʱ�䳬����ICMP��
						if (recvICMP->ICMPHead.type == ICMP_TIME_EXP)
						{
							// �����ӳ�ʱ�䲢��ʽ����ts.tv_usec�ĵ�λ��΢�룬ת�ɺ������
							delayTime = (double)pkt_header->ts.tv_usec / 1000;
							// ��ʽ��IP
							sprintf_s(hop_ip, 16, "%d.%d.%d.%d", recvICMP->ih.source_add & 255, recvICMP->ih.source_add >> 8 & 255,
								recvICMP->ih.source_add >> 16 & 255, recvICMP->ih.source_add >> 24 & 255);

							// ���б��������������ֱ����TTL
							AddListViewItems(sp.handle, TTL, delayTime, hop_ip);
							flag = 1;
						}

						// ����ECHO REPLY�����������յ�
						if (recvICMP->ICMPHead.type == ICMP_REPLY)
						{
							delayTime = (double)pkt_header->ts.tv_usec / 1000;
							sprintf_s(hop_ip, 16, "%d.%d.%d.%d", recvICMP->ih.source_add & 255, recvICMP->ih.source_add >> 8 & 255,
								recvICMP->ih.source_add >> 16 & 255, recvICMP->ih.source_add >> 24 & 255);
							AddListViewItems(sp.handle, TTL, delayTime, hop_ip);

							// �ָ���ť
							HWND hButton = GetDlgItem(myhdlg, IDC_BTN_TRACE);
							HWND hEditBox = GetDlgItem(myhdlg, IDC_INPUT);
							Button_Enable(hButton, TRUE);
							Edit_Enable(hEditBox, TRUE);
							SetEvent(hMaxHopEvent); // ��������
							//MessageBox(NULL, "׷�ٽ���!", "", MB_OK);
							break;// BREAK ICMP
						}
					}
				}
			}

			// ץ���м�İ����߳�ʱ��ͬ����ûץ����ͬ��
			if ((flag == 1) || (TimeOut > 1000))
			{
				// ��ʱ�ˡ���
				if (TimeOut > 1000)
				{
					char *out = "*";
					delayTime = -1;
					AddListViewItems(sp.handle, TTL, delayTime, out);
				}
				flag = 0;
				TimeOut = 0;

				// ץ�����ˣ�֪ͨ�����̸߳ɻ�ȴ�
				ReleaseMutex(hMutex);
				WaitForSingleObject(hMutex, INFINITE);
			}
		}// ICMP

		ReleaseMutex(hMutex);
		delete[]source_ip;
		delete[]dest_ip;
		delete[]hop_ip;
		ResetEvent(hBeginEvent);
		//CloseHandle(hMutex);
	}

	return 0;
}

// ��ListView����������
BOOL AddListViewItems(HWND hwndListView, int hop, double time, char *ip_add)
{
	char *chop = new char[8];
	char *ctime = new char[14];		

	sprintf_s(chop, 8, "%d", hop);
	if (time != -1)
	{
		sprintf_s(ctime, 14, "%6.3f", time);// ����ʱ�䲿���������ʾ�����������ָ���Ϊ3
	}
	else sprintf_s(ctime, 14, "Time Out!");	

	int ListItemCount = ListView_GetItemCount(hwndListView);
	LVITEM lvi;
	ZeroMemory(&lvi, sizeof(lvi));// �����������������߰���ĳ�ֵ
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
	// ����ı��ͳ���
	lvi.pszText = chop;
	lvi.cchTextMax = lstrlen(lvi.pszText) + 1;
	lvi.iItem = ListItemCount;
	// �����У����һ��ȷʵ��1
	ListView_InsertItem(hwndListView, &lvi);
	ListView_SetItemText(hwndListView, ListItemCount, 0, chop);
	ListView_SetItemText(hwndListView, ListItemCount, 1, chop);
	ListView_SetItemText(hwndListView, ListItemCount, 2, ctime);
	ListView_SetItemText(hwndListView, ListItemCount, 3, ip_add);

	// ��ȡ��������������һ��֮�����ͱ�ú�������ԭ�򣺲�ѯ /etc/hosts ���ļ��� DNS or NIS ������
	// �Ϸ�����gethostbyaddr���ٶ�Ҫ���·�������Ч��Ҳ������·�����getnameinfo
	sockaddr_in host;
	char hostname[1024];
	char service[20];

	ZeroMemory(&host, sizeof(host));
	host.sin_family = AF_INET;
	host.sin_port = htons(2049);
	host.sin_addr.S_un.S_addr = inet_addr(ip_add);

	getnameinfo((sockaddr *)(&host), sizeof(host), hostname, sizeof(hostname), service, sizeof(service), 0);

	if (strcmp(hostname, "255.255.255.255") == 0)memcpy(hostname, "Unknown Router", 16);

	ListView_SetItemText(hwndListView, ListItemCount, 4, hostname);

	delete[]chop;
	delete[]ctime;

	return TRUE;
}