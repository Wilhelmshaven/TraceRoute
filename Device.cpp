#include "stdafx.h"
#include "Device.h"

Device::Device()
{
	//��ʼ������
	adhandle = NULL;
	ip = new char[16];
	netmask = new char[16];
	mac = new char[6];
	macStr = new char[17];
	gateway_ip = new char[16];
	errbuf = new char[PCAP_ERRBUF_SIZE];
	gatewayMAC = new char[6];
	gatewayMACStr = new char[17];

}

Device::~Device()
{
	if (adhandle != NULL) pcap_close(adhandle); //�رմ򿪵�����
	pcap_freealldevs(alldevs);                  //�ͷ��豸�б�
}

//���߳�
void Device::DeviceGetReady()
{
	/* ��ȡ�����豸�б�*/
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)exit(1);

	//��������
	pcap_if_t *d;
	for (d = alldevs; d; d = d->next)
	{
		//���ȶ��ж�������Ҫ�Ĳ�������
		ZeroMemory(gateway_ip, 16);
		ZeroMemory(macStr, 17);

		OpenDevice(d);                                  //������
		GetInfo(d);   //��ø�������IP���������롢MAC��ַ������IP

		BOOL chk = TRUE;
		char *invalidGate = new char[];
		invalidGate = "0.0.0.0";                       //�����ز��Ϸ�
		if (this->gateway_ip == NULL)chk = FALSE;
		if (strcmp(this->gateway_ip, invalidGate) == 0)chk = FALSE;

		if (!chk)
		{
			continue;
		}
		else break;//���ҵ����ʵ�����

	}

}

//���豸
int Device::OpenDevice(pcap_if_t *d)
{
	if ((adhandle = pcap_open(d->name,          // �豸��
		65536,            // 65535��֤�ܲ��񵽲�ͬ������·���ϵ�ÿ�����ݰ���ȫ������
		PCAP_OPENFLAG_PROMISCUOUS,    // ����ģʽ���Ա�֤ץ��ARP��
		1,             /*��ȡ��ʱʱ�䣬��λΪ���룬��׽���ݰ���ʱ���ӳ�һ����ʱ�䣬Ȼ���ٵ����ں��еĳ���
					   ����Ч�ʽϸߡ�0��ʾû���ӳ٣�û�а������ʱ���������ء�-1��ʾ�������ء�*/
					   NULL,             // Զ�̻�����֤
					   errbuf            // ���󻺳��
					   )) == NULL)
	{
		pcap_freealldevs(alldevs);//�ͷ��豸�б�
		return -1;
	}
	else return 0;
}

//����Լ���IP������
void Device::GetInfo(pcap_if_t *d)
{
	pcap_addr_t *a;
	for (a = d->addresses; a; a = a->next)
	{
		if (a->addr->sa_family == AF_INET)//internetwork: UDP, TCP, etc. ��ȡIP��
		{
			if (a->addr)
			{
				char *ipstr;
				ipstr = iptos(((sockaddr_in *)a->addr)->sin_addr.s_addr);
				memcpy(ip, ipstr, 16);
			}
			if (a->netmask)
			{
				char *netmaskstr;
				netmaskstr = iptos(((sockaddr_in *)a->netmask)->sin_addr.s_addr);
				memcpy(netmask, netmaskstr, 16);
			}
		}
	}

	/*=================================WindowsAPI����================================*/
	//PIP_ADAPTER_INFO�ṹ��ָ��洢����������Ϣ
	PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
	//�õ��ṹ���С,����GetAdaptersInfo����
	DWORD stSize = sizeof(IP_ADAPTER_INFO);
	//����GetAdaptersInfo����,���pIpAdapterInfoָ�����;����stSize��������һ��������Ҳ��һ�������
	int nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
	if (ERROR_BUFFER_OVERFLOW == nRel)
	{
		//����������ص���ERROR_BUFFER_OVERFLOW
		//��˵��GetAdaptersInfo�������ݵ��ڴ�ռ䲻��,ͬʱ�䴫��stSize,��ʾ��Ҫ�Ŀռ��С
		//��Ҳ��˵��ΪʲôstSize����һ��������Ҳ��һ�������
		//�ͷ�ԭ�����ڴ�ռ�
		delete pIpAdapterInfo;
		//���������ڴ�ռ������洢����������Ϣ
		pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];
		//�ٴε���GetAdaptersInfo����,���pIpAdapterInfoָ�����
		nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
	}
	if (ERROR_SUCCESS == nRel)
	{
		while (pIpAdapterInfo)
		{
			IP_ADDR_STRING *pIpAddrString = &(pIpAdapterInfo->IpAddressList);

			do{
				if (strcmp(ip, pIpAddrString->IpAddress.String) == 0)//WinPcapȡ���ı���IP��WindowsAPIȡ���ı���IP��ͬ��ͬһ������
				{
					//��ȡ����IP
					gateway_ip = pIpAdapterInfo->GatewayList.IpAddress.String;
					//��ȡ�Լ���MAC��ַ
					sprintf(macStr, "%02X-%02X-%02X-%02X-%02X-%02X",
						pIpAdapterInfo->Address[0],
						pIpAdapterInfo->Address[1],
						pIpAdapterInfo->Address[2],
						pIpAdapterInfo->Address[3],
						pIpAdapterInfo->Address[4],
						pIpAdapterInfo->Address[5]);
					for (int i = 0; i < 6; i++)mac[i] = pIpAdapterInfo->Address[i];

					goto endWindowsAPI;//�ԣ�����������Ŷ��
				}
				pIpAddrString = pIpAddrString->Next;
			} while (pIpAddrString);

			pIpAdapterInfo = pIpAdapterInfo->Next;
		}
	}

endWindowsAPI:
	;//���һ���WindowsAPI˵�ݰ�~
}