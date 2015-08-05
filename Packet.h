#pragma once

//28字节ARP数据报结构
class ARP_head
{
public:
	WORD hardware_type;      //硬件类型,2字节
	WORD protocol_type;      //协议类型，2字节
	BYTE hardware_add_len;   //硬件地址长度，1字节
	BYTE protocol_add_len;   //协议地址长度，1字节
	WORD operation_field;    //操作字段，2字节
	BYTE source_mac_add[6];  //源mac地址，6字节
	DWORD source_ip_add;     //源ip地址，4字节
	BYTE dest_mac_add[6];    //目的mac地址，6字节
	DWORD dest_ip_add;       //目的ip地址，4字节

public:
	ARP_head();
	~ARP_head(){};
};

//8字节ICMP数据报头部
class ICMP_head
{
public:
	BYTE type;
	BYTE code;
	WORD checksum;
	DWORD data;

public:	
	ICMP_head(){};
	~ICMP_head(){};

};

//20字节IP数据报头结构
class IP_head
{
public:
	BYTE versionAndIHL;         //版本与首部长度，1字节，前4位后4位分开
	BYTE service;               //区分服务，1字节
	WORD length;                //总长度，2字节
	WORD id;                    //标识，2字节
	WORD flagAndOffset;         //标志和片偏移，2字节，前3位后13位分开
	BYTE TTL;                   //生存时间，1字节
	BYTE protocol;              //协议，1字节
	WORD checksum;              //首部校验和，2字节
	DWORD source_add;           //源地址，4字节
	DWORD dest_add;             //目的地址，4字节

public:
	IP_head(){};
	~IP_head(){};

};

//14字节以太网帧结构
class ethernet_head
{
public:
	BYTE dest_mac_add[6];    //目的mac地址，6字节
	BYTE source_mac_add[6];  //源mac地址，6字节
	WORD type;               //帧类型，2字节

public:
	ethernet_head();
	~ethernet_head(){};
};

//ICMP数据报
class ICMP_frame
{
public:
	ethernet_head eh;
	IP_head ih;
	ICMP_head ICMPHead;         //ICMP数据报头部，至少8字节

public:
	WORD cks(WORD* buffer, int size);
	ICMP_frame(){};
	~ICMP_frame(){};
};

//arp数据帧
class ARP_frame
{
public:
	ethernet_head eh;
	ARP_head ah;
	BYTE padding[18];
	//BYTE fcs[4];       //不能有这一段，否则会出错（只能被自动加上）

public:
	ARP_frame(){};
	~ARP_frame(){};

};