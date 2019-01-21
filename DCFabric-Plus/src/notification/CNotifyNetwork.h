#ifndef _CNOTIFYNETWORK_H_
#define _CNOTIFYNETWORK_H_

#include "CNotifyHandler.h"

class CNotifyNetwork : public CNotifyHandler
{
public:
    /*
     * 带参数的默认构造函数
     *
     * @param: center           通知中心类
     */
    CNotifyNetwork(CNotificationCenter* center): CNotifyHandler(center, bnc::notify::NETWORK) { };

    /*
     * 处理增加network
     *
     * @param: port             network指针
     *
     * @return: None
     */
    virtual void notifyAddNetwork(COpenstackNetwork* network);

    /*
     * 处理删除OpenstackPort
     *
     * @param: port_id          OpenstackPort的PortId
     *
     * @return: None
     */
    virtual void notifyDelNetwork(const std::string & network_id);

	
	 /*
     * 处理update network
     *
     * @param: port             network指针
     *
     * @return: None
     */
    virtual void notifyUpdateNetwork(COpenstackNetwork* network);

	/*
     * 处理增加network
     *
     * @param: port             network指针
     *
     * @return: None
     */
	virtual void notifyAddNetwork(Base_Network* network);
	
	/*
     * 处理update network
     *
     * @param: port             network指针
     *
     * @return: None
     */
	virtual void notifyUpdateNetwork(Base_Network* network);
private:
    /*
     * 默认构造函数
     */
    CNotifyNetwork();

    /*
     * 默认析构函数
     */
    ~CNotifyNetwork();
};
#endif
