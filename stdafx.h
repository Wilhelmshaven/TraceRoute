// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  �� Windows ͷ�ļ����ų�����ʹ�õ���Ϣ
// Windows ͷ�ļ�: 
#include <windows.h>
#include <windowsx.h>

// C ����ʱͷ�ļ�
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�
#include <stdio.h>
#include <iostream>
#include <WinSock2.h>
#include <cstring>
#include "pcap.h"
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"wpcap.lib")
#include <commctrl.h>  //Listview
#include <math.h>

//Windows API
#include <Iphlpapi.h>
#pragma comment(lib,"Iphlpapi.lib") //��Ҫ���Iphlpapi.lib��

#pragma pack(1)  //��һ���ֽ��ڴ����

using namespace std;