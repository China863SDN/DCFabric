
#ifndef _CROUTERMGR_H_
#define _CROUTERMGR_H_
#include    "CRouter.h"
#include    "CGateway.h"

typedef std::map<std::string, CRouter*>   CRouterMap;
typedef std::map<std::string, CGateway*>  CGatewayMap;
class  CRouterGateMgr
{
	private:
		CRouterGateMgr();

	public:
		~CRouterGateMgr();
		INT4   AddNode(const std::string & deviceid, const std::string & networkid, const std::string & subnetid, UINT4 ip, UINT1 type);
		INT4   DelNode(const std::string & deviceid, const std::string & networkid,const std::string & subnetid, UINT4 ip, UINT1 type);

		INT4   AddRouterNode(CRouter*  routerNode);
		INT4   DelRouterNode(const std::string & deviceid);
		
		INT4   AddRouterNode(const std::string & deviceid, const std::string & networkid, const std::string & subnetid, UINT4 ip);
		CRouter*   FindRouterNodeByDeviceId(const std::string & deviceid);
		CRouter*   FindRouterNodeByHostIp(UINT4 HostIp);
		CRouter*   FindRouterNodeByHostMac(UINT1* mac);
		INT4   AddGatewayNode(CGateway*  GatewayNode);
		INT4   DelGatewayNode(const std::string & subnetid);
		
		INT4   AddGatewayNode(const std::string & deviceid, const std::string & networkid,const std::string & subnetid, UINT4 ip);
		CGateway*   FindGatewayNodeBySubnetId(const std::string & subnetid);
		CGateway*	FindGatewayNodeByHostIp(UINT4 hostip);
		CGateway*	FindGatewayNodeByHostMac(UINT1* mac);
		CRouterMap  getRouterMapHeader();
		CGatewayMap getGatewayMapHeader();
		
		static CRouterGateMgr*  getInstance();
		
	private:
		CRouterMap         m_routerMap;
		CGatewayMap        m_gatewayMap;
		static  CRouterGateMgr*      m_pInstance;
};
#endif
