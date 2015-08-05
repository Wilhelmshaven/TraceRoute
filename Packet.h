#pragma once

//28�ֽ�ARP���ݱ��ṹ
class ARP_head
{
public:
	WORD hardware_type;      //Ӳ������,2�ֽ�
	WORD protocol_type;      //Э�����ͣ�2�ֽ�
	BYTE hardware_add_len;   //Ӳ����ַ���ȣ�1�ֽ�
	BYTE protocol_add_len;   //Э���ַ���ȣ�1�ֽ�
	WORD operation_field;    //�����ֶΣ�2�ֽ�
	BYTE source_mac_add[6];  //Դmac��ַ��6�ֽ�
	DWORD source_ip_add;     //Դip��ַ��4�ֽ�
	BYTE dest_mac_add[6];    //Ŀ��mac��ַ��6�ֽ�
	DWORD dest_ip_add;       //Ŀ��ip��ַ��4�ֽ�

public:
	ARP_head();
	~ARP_head(){};
};

//8�ֽ�ICMP���ݱ�ͷ��
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

//20�ֽ�IP���ݱ�ͷ�ṹ
class IP_head
{
public:
	BYTE versionAndIHL;         //�汾���ײ����ȣ�1�ֽڣ�ǰ4λ��4λ�ֿ�
	BYTE service;               //���ַ���1�ֽ�
	WORD length;                //�ܳ��ȣ�2�ֽ�
	WORD id;                    //��ʶ��2�ֽ�
	WORD flagAndOffset;         //��־��Ƭƫ�ƣ�2�ֽڣ�ǰ3λ��13λ�ֿ�
	BYTE TTL;                   //����ʱ�䣬1�ֽ�
	BYTE protocol;              //Э�飬1�ֽ�
	WORD checksum;              //�ײ�У��ͣ�2�ֽ�
	DWORD source_add;           //Դ��ַ��4�ֽ�
	DWORD dest_add;             //Ŀ�ĵ�ַ��4�ֽ�

public:
	IP_head(){};
	~IP_head(){};

};

//14�ֽ���̫��֡�ṹ
class ethernet_head
{
public:
	BYTE dest_mac_add[6];    //Ŀ��mac��ַ��6�ֽ�
	BYTE source_mac_add[6];  //Դmac��ַ��6�ֽ�
	WORD type;               //֡���ͣ�2�ֽ�

public:
	ethernet_head();
	~ethernet_head(){};
};

//ICMP���ݱ�
class ICMP_frame
{
public:
	ethernet_head eh;
	IP_head ih;
	ICMP_head ICMPHead;         //ICMP���ݱ�ͷ��������8�ֽ�

public:
	WORD cks(WORD* buffer, int size);
	ICMP_frame(){};
	~ICMP_frame(){};
};

//arp����֡
class ARP_frame
{
public:
	ethernet_head eh;
	ARP_head ah;
	BYTE padding[18];
	//BYTE fcs[4];       //��������һ�Σ���������ֻ�ܱ��Զ����ϣ�

public:
	ARP_frame(){};
	~ARP_frame(){};

};