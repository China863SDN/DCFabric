#ifndef SERVICE_EXTERNALDETECTING_H
#define SERVICE_EXTERNALDETECTING_H

#include "comm-util.h"
#include <cstring>
#include <string>
#include <vector>
#include <list>
#include <arpa/inet.h>
#include "CArpFloodMgr.h"
#include "BaseExternal.h"
#include "BaseExternalManager.h"
#include "BasePort.h"
#include "BasePortManager.h"
#include "CTimer.h"
#include "bnc-error.h"
#define RETURN_OK       1
#define RETURN_ERR      0

#define RETURN_TRUE     1
#define RETURN_FALSE    0

#define EXTERNAL_DETECT_INTERVAL    5

using namespace std;

class ArpFlood_info
{
    public:
        ArpFlood_info(UINT4 p_src_IP,UINT1* p_src_MAC,UINT4 p_dst_IP);
        UINT1 checkifMatched(UINT4 p_src_IP,UINT1* p_src_MAC,UINT4 p_dst_IP);
		UINT4 get_src_IP(void);
        UINT1* get_src_MAC(void);
        UINT4 get_dst_IP(void);
    private:
        UINT4 src_IP;
        UINT1 src_MAC[6];
        UINT4 dst_IP;
};

class Service_ExternalDetecting
{
    public:
        static Service_ExternalDetecting* Get_Instance(void);
        UINT1 CheckARPReply_UpdateExternal(UINT4 p_reply_dst_IP,UINT1* p_reply_dst_MAC,UINT4 p_reply_src_IP,UINT1 * p_reply_src_MAC,UINT8 p_switch_DPID,UINT4 p_switch_port);     //judge if para match node in floods. if true then remove matched node
		void init(void);
    private:
        Service_ExternalDetecting();
        ~Service_ExternalDetecting();
        Service_ExternalDetecting(const Service_ExternalDetecting&);
        Service_ExternalDetecting& operator =(const Service_ExternalDetecting&);

    private:
        static void  timedTask(void* param);                                                      //each time, it will empty floods and generate new floods
        UINT1 generateFloodNode(void);
        UINT1 generateARPFlood(void);
        UINT1 insertNode_ByInfo(UINT4 p_src_IP,UINT1* p_src_MAC,UINT4 p_dst_IP);
        UINT1 deleteNode_ByFlood(ArpFlood_info * p_flood);
        UINT1 clearNodes(void);
        void lock_Mutex(void);
        void unlock_Mutex(void);

    private:
        static Service_ExternalDetecting* Instance;

    private:
        pthread_mutex_t BaseSrvMutex;
        CTimer ExternalDetectingTimer;
        list<ArpFlood_info *> floods;
};

#endif // SERVICE_EXTERNALDETECTING_H
