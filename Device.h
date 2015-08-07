//�豸��
class Device
{
public:
	pcap_if_t *alldevs;   //�豸�б�
	pcap_t *adhandle;     //��ǰ�豸
	char *ip;             //�Լ���IP
	char *netmask;        //�Լ�����������
	char *mac;            //�Լ���MAC��ַ��ʮ�����ƣ�
	char *macStr;         //�Լ���MAC��ַ���ַ�����
	char *gateway_ip;     //����IP��ַ
	char *gatewayMAC;
	char *gatewayMACStr;

private:
	char *errbuf;         //���󻺴�

private:

	int OpenDevice(pcap_if_t *d);//���豸
	void GetInfo(pcap_if_t *d);  //��ø�������IP���������롢MAC��ַ������IP

public:
	void DeviceGetReady(int option); //�������

	//���������͵�IP��ַת�����ַ������͵�
	char *iptos(u_long in)
	{
		char *ipstr = new char[16];
		u_char *p;
		p = (u_char *)&in;//�ⲿ��ͨ��ָ�����͵ĸı�ʵ����ת������
		sprintf(ipstr, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
		return ipstr;
	}

	Device();          //���캯��
	~Device();         //�������������ͷű����豸�б��رմ򿪵�����
};

//��װ������
class sparam
{
public:
	char *dest_ip;
	HWND handle;

public:
	sparam()
	{
		dest_ip = new char[16];
		handle = NULL;
	}
	~sparam(){};
};