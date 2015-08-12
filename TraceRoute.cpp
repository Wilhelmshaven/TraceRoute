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
// TraceRoute.cpp : 定义应用程序的入口点。
//
#include "stdafx.h"
#include "TraceRoute.h"
#include "Packet.h"
#include "Device.h"

//风格变换
#pragma comment(linker, "\"/manifestdependency:type='Win32'\
 name='Microsoft.Windows.Common-Controls' version='6.0.0.0'\
 processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// 全局变量: 
enum CustomDefine
{
	MAX_LOADSTRING = 100,
	MAX_ROUTER = 30          // 自定义最大跳数
};
enum IPDefine
{
	ETH_IP = 0x0800,         // IP数据包
	TCP = 0x06,              // IP数据报的TCP协议
	UDP = 0x11               // IP数据报的UDP协议
};
enum ARPDefine
{
	ETH_ARP = 0x0806,        // ARP数据包
	ARP_HARDWARE = 1,        // 硬件类型字段值，表示以太网地址
	ARP_REQUEST = 1,         // ARP请求字段
	ARP_REPLY = 2            // ARP应答字段
};
enum ICMPDefine
{
	ICMP = 0x01,             // IP数据报的ICMP协议
	ICMP_REQ = 0x08,         // ICMP ECHO REQUEST
	ICMP_REPLY = 0x00,       // ICMP ECHO REPLY
	ICMP_TIME_EXP = 0x0b     // ICMP TIME EXPIRED
};                   
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名
Device myDevice;                                // 设备类            
HANDLE hBeginEvent = CreateEvent(NULL, TRUE, FALSE, NULL);       // 开始事件
HANDLE hFinishEvent = CreateEvent(NULL, TRUE, FALSE, NULL);      // 结束事件
HANDLE hMaxHopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
HANDLE hMutex;// = CreateMutex(NULL, FALSE, NULL); // 使用互斥对象完成线程的交替运作
HWND myhdlg = NULL;                             // 对话框（用作主窗口）的句柄
sparam sp;                                      // 封装参数表
int TTL = 0;                                    // TTL，数值上同时也等于跳数
char *realIP = new char[16];                    // 存储IP（输入IP或者域名解析出来的IP）
BOOL HaveMAC = FALSE;
int SelectedNIC;
// 此代码模块中包含的函数的前向声明: 
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);                                //主窗口消息处理
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);                                  //关于窗口消息处理
INT_PTR CALLBACK    DlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);         //子窗口消息处理
BOOL AddListViewItems(HWND hwndListView, int hop, double time, char *ip_add);           //把结果输出到ListView中
BOOL CheckInput(char *input, int len);                                                  //检查输入合法性
UINT SendPacket(LPVOID lpParameter);                                                    //发包方法
UINT AnalyzePacket(LPVOID lpParameter);                                                 //收包方法

HANDLE sendThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendPacket, NULL, 0, NULL);     // 发包线程
HANDLE recvThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AnalyzePacket, NULL, 0, NULL);  // 收包线程

/*============================== WinMain ==============================*/
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO:  在此放置代码。
	MSG msg;
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TRACEROUTE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化: 
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TRACEROUTE));

	// 主消息循环: 
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

//注册窗口类。
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

// 保存实例句柄并创建主窗口
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // 将实例句柄存储在全局变量中

   // 主窗口不可变大小，同时禁用最大化
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

// 处理主窗口的消息。
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	//HWND myhdlg = NULL;

	switch (message)
	{
	case WM_CREATE:
	{
		// 创建子对话框并将其作为主窗口
		myhdlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_FORMVIEW), hWnd, (DLGPROC)DlgProc);
		ShowWindow(myhdlg, SW_SHOW);// 显示对话框

		// 设置标题字体样式
		LOGFONT TitleFont;
		ZeroMemory(&TitleFont, sizeof(TitleFont));                    // 这个必须做，清除乱七八糟的初值
		lstrcpy(TitleFont.lfFaceName, "Segoe Script");                // 设置字体
		TitleFont.lfWeight = FW_BOLD;                                 // 粗细，BOLD=700，写过CSS都知道
		TitleFont.lfHeight = -24;                                     // 字体大小，这个很有讲究……
		TitleFont.lfCharSet = DEFAULT_CHARSET;                        // 默认字符集
		TitleFont.lfOutPrecision = OUT_DEVICE_PRECIS;                 // 输出精度

		HFONT hFont = CreateFontIndirect(&TitleFont);
		HWND hWndStatic = GetDlgItem(myhdlg, IDC_TITLE);
		SendMessage(hWndStatic, WM_SETFONT, (WPARAM)hFont, 0);

		//设置类目字体样式
		LOGFONT TextFont;
		ZeroMemory(&TextFont, sizeof(TextFont));
		lstrcpy(TextFont.lfFaceName, "Gabriola");
		TextFont.lfHeight = -18;
		hFont = CreateFontIndirect(&TextFont);

		//设置控件字体
		hWndStatic = GetDlgItem(myhdlg, IDC_TEXT_1);
		SendMessage(hWndStatic, WM_SETFONT, (WPARAM)hFont, 0);
		hWndStatic = GetDlgItem(myhdlg, IDC_TEXT_2);
		SendMessage(hWndStatic, WM_SETFONT, (WPARAM)hFont, 0);
	}//WM_CREATE

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 分析菜单选择: 
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

// “关于”框的消息处理程序。
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

// 处理对话框消息  
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
		// 限制文本输入框的输入长度
		Edit_LimitText(hEditBox, 32);

		// 放置一个提示文本：涉及ANSI转宽字符的问题
		char *tmp = new char[64];
		strcpy(tmp, "请输入IP地址如：1.1.1.1");                         // 设置提示文本
		int dwNum = MultiByteToWideChar(CP_ACP, 0, tmp, -1, NULL, 0);     // 获得所要转成的宽字符的长度
		wchar_t *tip = new wchar_t[dwNum];                                // 用上一步得到的长度进行初始化
		MultiByteToWideChar(CP_ACP, 0, tmp, -1, tip, dwNum);              // 多字节转换成宽字节，sizeof不好使的说
		Edit_SetCueBannerText(hEditBox, tip);                             // 输出提示文本（这里要求文本为Unicode格式）
		delete[]tmp;
		delete[]tip;

		// 添加Listview的列与下拉框数据

		// 设置ListView的列  
		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		ListView_SetTextColor(hListview, RGB(0, 0, 255));                // 设置文字颜色
		ListView_SetTextBkColor(hListview, RGB(199, 237, 204));          // 设置文字背景颜色
		ListView_SetExtendedListViewStyle(hListview, LVS_EX_GRIDLINES);  // 添加导航线

		lvc.pszText = "Hop";                        // 列标题  
		lvc.cx = 0;                                 // 列宽  
		lvc.iSubItem = 0;                           // 子项索引，第一列无子项 (0) 
		lvc.fmt = LVCFMT_CENTER;
		ListView_InsertColumn(hListview, 0, &lvc);  // 插入无意义第一列占位，以便居中
		lvc.cx = 40;
		ListView_InsertColumn(hListview, 1, &lvc);  // 第一列

		lvc.pszText = "Times(ms)";
		lvc.cx = 100;
		lvc.iSubItem = 1;                           // 子项索引  
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

		// 给下拉列表填充项目
		pcap_if_t *d;
		for (d = myDevice.alldevs; d; d = d->next)
		{
			SendMessage(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)d->description);
		}

		break;
	}//WM_INITIALIZE

	case WM_CREATE:
	{
		//创建按钮
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
			// 按钮功能的实现
		case IDC_BTN_TRACE:
		{
			// 避免用户乱搞，禁用按钮
			hButton = GetDlgItem(hdlg, IDC_BTN_TRACE);
			Button_Enable(hButton, FALSE);
			Edit_Enable(hEditBox, FALSE);

			// 清空需要清空的信息（如上一次的扫描结果）并进行相关置位操作
			SendMessage(hListview, LVM_DELETEALLITEMS, 0, 0);
			TTL = 0;

			// 获取输入并检查输入合法性
			char *input = new char[64];		
			BOOL chk = FALSE;
			int len = 0;

			ZeroMemory(input, 64);
			Edit_GetText(hEditBox, input, 64);
			len = Edit_GetTextLength(hEditBox);

			if (len != 0)
			{
			    // 合法性检查：若是域名，必须能够解析IP；若是IP，必须符合IP格式
				chk = CheckInput(input, len);
			}
			if (!chk || SelectedNIC < 0)
			{
				// 不合法则重新输入
				MessageBox(hdlg, (LPCSTR)"您的输入不合法或未选择合适的网卡，请检查！", NULL, MB_OK);
				Button_Enable(hButton, TRUE);
				Edit_Enable(hEditBox, TRUE);

				delete []input;

				break;
			}
			else
			{
				// 若通过合法性检查，则开始干活咯
				hMutex = CreateMutex(NULL, FALSE, NULL);

				// 把解析出来的IP显示在输入框
				sprintf_s(input, 64, "%s(%s)", input, realIP);
				Edit_SetText(hEditBox, input);				

				//开启并绑定网卡
				myDevice.DeviceGetReady(SelectedNIC);
				sp.dest_ip = realIP;
				sp.handle = hListview;

				// 复位
				ResetEvent(hMaxHopEvent);
				// 设置开始事件
				SetEvent(hBeginEvent);		

				delete []input;
				break;
			}
		}//IDC_BTN_TRACE

		case IDC_BTN_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hdlg, About);
			break;

		case IDC_BTN_QUIT:
			//设置事件，终止线程
			SetEvent(hFinishEvent);
			SetEvent(hBeginEvent);
			PostQuitMessage(0);
			break;

		default:
			break;
		}// wmID

		//处理控件消息
		switch (wmEvent)
		{
			// 下拉列表选择发生变化
		case CBN_SELCHANGE:
		{
			SelectedNIC = -1;
			SelectedNIC = (int)SendMessage(hWndComboBox, CB_GETCURSEL, 0, 0); // 获得选中的选项编号
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

// 检查输入合法性
BOOL CheckInput(char *input, int len)
{
	ZeroMemory(realIP, 16);

	// 先处理域名的情况，转成IP，再用IP判断函数。由于域名处理是后面加的，这部分有一点代码冗余
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

	// 必须初始化Winsock
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(1, 1);
	WSAStartup(wVersionRequested, &wsaData);

	errCode = getaddrinfo(input, NULL, &hostInfo, &res);

	// 正确会返回0，且资源非空
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

	/*若输入的是域名，上面已经转成了IP；若输入IP，则上面依旧是IP；若非法（包括非法域名和非法IP，以及合法但是不存在的IP），
	则realIP一定是NULL。也就是说，我用域名解析的方法替代了我原有的差错检测代码，甚是方便*/
	if (*realIP == NULL)return FALSE;
	else return TRUE;

}

// 发包线程：发送TTL递增的UDP包
UINT SendPacket(LPVOID lpParameter)
{
	while (1)
	{
		Beep(440, 200);
		// 获得互斥对象
		WaitForSingleObject(hMutex, INFINITE);

		WaitForSingleObject(hBeginEvent, INFINITE);
		// 如果在运行过程中点退出，则终止
		if (WaitForSingleObject(hFinishEvent, 0) == WAIT_OBJECT_0)break;
		Beep(440, 200);

		BYTE *sendbuf = new BYTE[74];                      // 缓存大小
		ICMP_frame ICMPFrame;

		char *xip = sp.dest_ip;
		ZeroMemory(sendbuf, 74);

		if (!HaveMAC)
		{
			Beep(660, 100);
			// 首先发送ARP包获取网关MAC地址
			ARP_frame ARPFrame;

			// 填充内容
			memcpy(ARPFrame.eh.source_mac_add, myDevice.mac, 6);
			//memset(ARPFrame.eh.dest_mac_add, 0xff, 6);         //MAC的广播地址为FF-FF-FF-FF-FF-FF
			ARPFrame.eh.type = htons(ETH_ARP);                 // 以太网帧头协议类型

			ARPFrame.ah.hardware_type = htons(ARP_HARDWARE);   // 硬件地址
			ARPFrame.ah.protocol_type = htons(ETH_IP);         // ARP包协议类型
			ARPFrame.ah.source_ip_add = inet_addr(myDevice.ip);         // 请求方的IP地址为自身的IP地址	
			memcpy(ARPFrame.ah.source_mac_add, myDevice.mac, 6);
			ARPFrame.ah.operation_field = htons(ARP_REQUEST);  //ARP请求包
			ARPFrame.ah.dest_ip_add = inet_addr(myDevice.gateway_ip);   // 目的IP填写为网关IP

			// 把做好的数据包装入缓存
			memset(sendbuf, 0, sizeof(sendbuf));
			memcpy(sendbuf, &ARPFrame, sizeof(ARPFrame));

			pcap_sendpacket(myDevice.adhandle, sendbuf, 60);            // 发包

			// 让收包线程干活
			ReleaseMutex(hMutex);
			WaitForSingleObject(hMutex, INFINITE);
		}

		// 获得网关MAC地址后，填写并发送ICMP数据报
		while (true)
		{
			TTL++;         //TTL+1
			// 最大跳数不能大于自定义最大跳数MAX_ROUTER
			if (TTL > MAX_ROUTER)
			{
				SetEvent(hMaxHopEvent);
				break;
			}

			ZeroMemory(sendbuf, 74);
			// 填充内容
			// 以太网头部
			memcpy(ICMPFrame.eh.source_mac_add, myDevice.mac, 6);        // 源MAC地址为自己的MAC地址
			memcpy(ICMPFrame.eh.dest_mac_add, myDevice.gatewayMAC, 6);   // 目的MAC地址为网关的MAC地址
			ICMPFrame.eh.type = htons(ETH_IP);                           // 以太网帧头协议类型

			// IP头部
			ICMPFrame.ih.versionAndIHL = 0x45;                           // IPV4为4，长度0101（20字节）
			ICMPFrame.ih.service = 0x00;                                 // 区分服务，置零即可
			ICMPFrame.ih.length = htons(0x003C);                         // 总长度设为60
			ICMPFrame.ih.id = htons(GetCurrentProcessId());              // ID，从系统获得
			ICMPFrame.ih.flagAndOffset = 0x00;                           // 标志和偏移，置零即可
			ICMPFrame.ih.TTL = TTL;                                      // 动态设置TTL
			ICMPFrame.ih.protocol = ICMP;                                // 协议名设置为ICMP
			ICMPFrame.ih.checksum = 0x0000;                              // 校验和置零，填充完头部后计算之
			ICMPFrame.ih.source_add = inet_addr(myDevice.ip);            // 源IP地址为自己的IP
			ICMPFrame.ih.dest_add = inet_addr(xip);                      // 目的IP地址，由用户给出
			// IP头部校验和
			char *cksbuf = new char[20];
			memcpy(cksbuf, &ICMPFrame.ih, 20);
			ICMPFrame.ih.checksum = ICMPFrame.cks((WORD *)cksbuf, 20);

			// ICMP数据
			ICMPFrame.ICMPHead.type = ICMP_REQ;                          // ECHO
			ICMPFrame.ICMPHead.code = 0x00;                              // ECHO：code必须置零
			ICMPFrame.ICMPHead.checksum = 0x0000;                        // 先置零，计算后填上
			ICMPFrame.ICMPHead.data = 0x0c000100;                        // 这个据说随意
			// 计算ICMP的校验和
			memcpy(cksbuf, &ICMPFrame.ICMPHead, 8);
			ICMPFrame.ICMPHead.checksum = ICMPFrame.cks((WORD *)cksbuf, 8);
			delete[]cksbuf;

			// 把做好的数据包装入缓存
			memset(sendbuf, 0, sizeof(sendbuf));
			memcpy(sendbuf, &ICMPFrame, sizeof(ICMPFrame));

			pcap_sendpacket(myDevice.adhandle, sendbuf, 74);            // 发包

			// 让收包线程干活
			ReleaseMutex(hMutex);
			//Beep(660, 200);
			// 若未终止，则等待收包线程收到数据报并处理完成
			WaitForSingleObject(hMutex, INFINITE);

			// 如果已经搞定，事件状态就会被改变，则终止
			if (WaitForSingleObject(hFinishEvent, 0) == WAIT_OBJECT_0 || WaitForSingleObject(hMaxHopEvent, 0) == WAIT_OBJECT_0)break;
		}

		delete[]sendbuf;
		ResetEvent(hBeginEvent);
	}

	return 0;
}

/* 分析截留的数据包中的ICMP数据报 */
UINT AnalyzePacket(LPVOID lpParameter)
{
	while (1)
	{		
		Beep(880, 200);
		WaitForSingleObject(hBeginEvent, INFINITE);		
		// 如果在运行过程中点退出，则终止
		if (WaitForSingleObject(hFinishEvent, 0) == WAIT_OBJECT_0)break;
		
		char *xip = sp.dest_ip;                   // 终点IP
		char *source_ip = new char[16];           // 用来存放数据包中的源IP
		char *dest_ip = new char[16];             // 用来存放数据包中的目的IP
		double delayTime = 0;                     // 延迟时间
		char *hop_ip = new char[16];              // 路由IP
		int res;                                  // 数据流
		pcap_pkthdr * pkt_header;
		const u_char * pkt_data;
		ICMP_frame *recvICMP;                     // 用来存放数据流的ICMP结构
		int flag = 0;
		int TimeOut = 0;                          // 超时计数器

		// 等待互斥对象被发包线程释放，收取ARP应答包
		WaitForSingleObject(hMutex, INFINITE);
		Beep(880, 200);

		if (!HaveMAC)
		{
			Beep(660, 100);
			while (true)
			{
				if ((res = pcap_next_ex(myDevice.adhandle, &pkt_header, &pkt_data)) > 0)// 使用非回调方法捕获数据包
				{
					if (*(WORD *)(pkt_data + 12) == htons(ETH_ARP))// 判断ARP包的第13,14位（Type）是否等于0x0806，目的是滤出ARP包			
					{
						// 把流数据装进ARP帧结构
						ARP_frame *recvARP = (ARP_frame *)pkt_data;

						// 格式化IP以进行比较
						sprintf_s(source_ip, 16, "%d.%d.%d.%d", recvARP->ah.source_ip_add & 255, recvARP->ah.source_ip_add >> 8 & 255,
							recvARP->ah.source_ip_add >> 16 & 255, recvARP->ah.source_ip_add >> 24 & 255);

						// 判断操作符位是否是ARP_REPLY，即滤出ARP应答包并确认是网关答复的ARP包
						if (recvARP->ah.operation_field == htons(ARP_REPLY) && (strcmp(source_ip, myDevice.gateway_ip) == 0))
						{
							// 保存获取到的MAC地址
							sprintf_s(myDevice.gatewayMACStr, 18, "%02X-%02X-%02X-%02X-%02X-%02X", recvARP->ah.source_mac_add[0],
								recvARP->ah.source_mac_add[1], recvARP->ah.source_mac_add[2], recvARP->ah.source_mac_add[3],
								recvARP->ah.source_mac_add[4], recvARP->ah.source_mac_add[5]);

							u_char *p;
							p = (u_char *)&recvARP->ah.source_mac_add;// 这部分通过指针类型的改变实现了转换过程
							for (int i = 0; i < 6; i++)myDevice.gatewayMAC[i] = p[i];

							// 通知发包线程干活，并等待
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
			// 如果在运行过程中点退出，则终止
			if (WaitForSingleObject(hFinishEvent, 0) == WAIT_OBJECT_0 || WaitForSingleObject(hMaxHopEvent, 0) == WAIT_OBJECT_0)break;

			TimeOut++;
			if ((res = pcap_next_ex(myDevice.adhandle, &pkt_header, &pkt_data)) > 0)                  // 使用非回调方法捕获数据包
			{
				if (*(WORD *)(pkt_data + 12) == htons(ETH_IP) && (*(BYTE *)(pkt_data + 23) == ICMP))  // 选出发给自己的ICMP的时间超过报文
				{
					// 把数据流装入ICMP结构里
					recvICMP = (ICMP_frame *)pkt_data;

					// 格式化IP以进行比较
					sprintf_s(dest_ip, 16, "%d.%d.%d.%d", recvICMP->ih.dest_add & 255, recvICMP->ih.dest_add >> 8 & 255,
						recvICMP->ih.dest_add >> 16 & 255, recvICMP->ih.dest_add >> 24 & 255);

					if (strcmp(dest_ip, myDevice.ip) == 0)                        // 选出自己的ICMP包
					{
						// 若是时间超过的ICMP包
						if (recvICMP->ICMPHead.type == ICMP_TIME_EXP)
						{
							// 计算延迟时间并格式化：ts.tv_usec的单位是微秒，转成毫秒输出
							delayTime = (double)pkt_header->ts.tv_usec / 1000;
							// 格式化IP
							sprintf_s(hop_ip, 16, "%d.%d.%d.%d", recvICMP->ih.source_add & 255, recvICMP->ih.source_add >> 8 & 255,
								recvICMP->ih.source_add >> 16 & 255, recvICMP->ih.source_add >> 24 & 255);

							// 给列表增加项，这里跳数直接用TTL
							AddListViewItems(sp.handle, TTL, delayTime, hop_ip);
							flag = 1;
						}

						// 若是ECHO REPLY包，即到达终点
						if (recvICMP->ICMPHead.type == ICMP_REPLY)
						{
							delayTime = (double)pkt_header->ts.tv_usec / 1000;
							sprintf_s(hop_ip, 16, "%d.%d.%d.%d", recvICMP->ih.source_add & 255, recvICMP->ih.source_add >> 8 & 255,
								recvICMP->ih.source_add >> 16 & 255, recvICMP->ih.source_add >> 24 & 255);
							AddListViewItems(sp.handle, TTL, delayTime, hop_ip);

							// 恢复按钮
							HWND hButton = GetDlgItem(myhdlg, IDC_BTN_TRACE);
							HWND hEditBox = GetDlgItem(myhdlg, IDC_INPUT);
							Button_Enable(hButton, TRUE);
							Edit_Enable(hEditBox, TRUE);
							SetEvent(hMaxHopEvent); // 结束发包
							//MessageBox(NULL, "追踪结束!", "", MB_OK);
							break;// BREAK ICMP
						}
					}
				}
			}

			// 抓到中间的包或者超时才同步，没抓到不同步
			if ((flag == 1) || (TimeOut > 1000))
			{
				// 超时了……
				if (TimeOut > 1000)
				{
					char *out = "*";
					delayTime = -1;
					AddListViewItems(sp.handle, TTL, delayTime, out);
				}
				flag = 0;
				TimeOut = 0;

				// 抓到包了，通知发包线程干活并等待
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

// 在ListView里面增加项
BOOL AddListViewItems(HWND hwndListView, int hop, double time, char *ip_add)
{
	char *chop = new char[8];
	char *ctime = new char[14];		

	sprintf_s(chop, 8, "%d", hop);
	if (time != -1)
	{
		sprintf_s(ctime, 14, "%6.3f", time);// 控制时间部分输出流显示浮点数的数字个数为3
	}
	else sprintf_s(ctime, 14, "Time Out!");	

	int ListItemCount = ListView_GetItemCount(hwndListView);
	LVITEM lvi;
	ZeroMemory(&lvi, sizeof(lvi));// 这个必须做，清除乱七八糟的初值
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
	// 项的文本和长度
	lvi.pszText = chop;
	lvi.cchTextMax = lstrlen(lvi.pszText) + 1;
	lvi.iItem = ListItemCount;
	// 插入列，最后一个确实是1
	ListView_InsertItem(hwndListView, &lvi);
	ListView_SetItemText(hwndListView, ListItemCount, 0, chop);
	ListView_SetItemText(hwndListView, ListItemCount, 1, chop);
	ListView_SetItemText(hwndListView, ListItemCount, 2, ctime);
	ListView_SetItemText(hwndListView, ListItemCount, 3, ip_add);

	// 获取主机名，加上这一段之后代码就变得很慢……原因：查询 /etc/hosts 等文件及 DNS or NIS 服务器
	// 老方法：gethostbyaddr，速度要比新方法慢，效果也差；这是新方法：getnameinfo
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