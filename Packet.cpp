#include "stdafx.h"
#include "Packet.h"

//��ʼ����̫��֡ͷ
ethernet_head::ethernet_head()
{
	memset(dest_mac_add, 0xff, 6);
	memset(source_mac_add, 0xff, 6);
}

//��ʼ��ARP���ṹ
ARP_head::ARP_head()
{
	memset(dest_mac_add, 0xff, 6);  //��ʼ��Ϊ�㲥��ַ
	hardware_add_len = 6;
	protocol_add_len = 4;
}

WORD ICMP_frame::cks(WORD* buffer, int size)
{
	unsigned long cksum = 0;
	while (size>1)
	{
		cksum += *buffer++;
		size -= sizeof(WORD);                //��˫�ֽڣ�16λ������
	}
	if (size)
	{
		cksum += *(BYTE*)buffer;             //�����Ʒ������
	}
	cksum = (cksum >> 16) + (cksum & 0xffff);//����16bit���16bit���
	cksum += (cksum >> 16);                  //����λ����λ��16bit���16bit �����
	return (WORD)(~cksum);
}