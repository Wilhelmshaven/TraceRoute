//设备类
class Device
{
public:
	pcap_if_t *alldevs;   //设备列表
	pcap_t *adhandle;     //当前设备
	char *ip;             //自己的IP
	char *netmask;        //自己的子网掩码
	char *mac;            //自己的MAC地址（十六进制）
	char *macStr;         //自己的MAC地址（字符串）
	char *gateway_ip;     //网关IP地址
	char *gatewayMAC;
	char *gatewayMACStr;

private:
	char *errbuf;         //错误缓存

private:

	int OpenDevice(pcap_if_t *d);//打开设备
	void GetInfo(pcap_if_t *d);  //获得该网卡的IP、子网掩码、MAC地址和网关IP

public:
	void DeviceGetReady(int option); //功能入口

	//将数字类型的IP地址转换成字符串类型的
	char *iptos(u_long in)
	{
		char *ipstr = new char[16];
		u_char *p;
		p = (u_char *)&in;//这部分通过指针类型的改变实现了转换过程
		sprintf(ipstr, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
		return ipstr;
	}

	Device();          //构造函数
	~Device();         //析构函数，并释放本机设备列表及关闭打开的网卡
};

//封装参数表
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