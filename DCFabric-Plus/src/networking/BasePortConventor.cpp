#include "BasePortConventor.h"
extern UINT1 MACStringtoArray(string&p_StringMAC,UINT1 p_MACArray[]);
Base_Port * BasePortConventor_fromCOpenstackPort(const COpenstackPort& port)
{
	string ID			="";
    string name			="";
    string tenant_ID	="";
    string device_owner	="";
	string device_ID	="";
    string network_ID	="";
    string string_MAC	="";
    string subnet_ID	="";
	string IPstring 	="";
    UINT4 IP			=INADDR_NONE;
    UINT1 status		=BASE_PORT_STATUS_ACTIVE;
    UINT1 MAC[6]		={0,0,0,0,0,0};
	
	ID=port.getId();
	name=port.getName();
	tenant_ID   =port.getTenantId();
	device_owner=port.getDeviceOwner();
	device_ID	=port.getDeviceId();
	network_ID	=port.getNetworkId();
	string_MAC	=port.getMacAddress();
	MACStringtoArray(string_MAC,MAC);
	
	Base_Port* CreatedPort =new Base_Port(ID,name,tenant_ID,network_ID,device_ID,device_owner,status,MAC);
	
	map<string, string> FixedIPs=port.getFixedIps();
	map<string, string> ::iterator FixedIP =FixedIPs.begin();
	while(FixedIP!= FixedIPs.end())
	{
		IPstring  =FixedIP->first;
		IP=ip2number(IPstring.c_str());
		subnet_ID =FixedIP->second;
		CreatedPort->add_fixed_IP(IP,subnet_ID);
		FixedIP++;
	}
	return CreatedPort;
}
