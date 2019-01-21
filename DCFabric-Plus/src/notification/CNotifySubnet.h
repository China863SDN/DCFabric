#ifndef _CNOTIFYSUBNET_H_
#define  _CNOTIFYSUBNET_H_
#include "CNotifyHandler.h"

class CNotifySubnet: public CNotifyHandler
{
	public:
    /*
     * 带参数的默认构造函数
     *
     * @param: center           通知中心类
     */
    CNotifySubnet(CNotificationCenter* center): CNotifyHandler(center, bnc::notify::SUBNET) { };

    /*
     * 处理增加subnet
     *
     * @param: port             subnet指针
     *
     * @return: None
     */
    virtual void notifyAddSubnet(COpenstackSubnet* subnet);

    /*
     * 处理删除subnet
     *
     * @param: port_id          subnet的PortId
     *
     * @return: None
     */
    virtual void notifyDelSubnet(const std::string & subnet_id);

	/*
     * 处理update subnet
     *
     * @param: network             subnet指针
     *
     * @return: None
     */
    virtual void notifyUpdateSubnet(COpenstackSubnet* subnet);

	/*
     * 处理增加subnet
     *
     * @param: port             subnet指针
     *
     * @return: None
     */
    virtual void notifyAddSubnet(Base_Subnet* subnet);

	/*
     * 处理update subnet
     *
     * @param: network             subnet指针
     *
     * @return: None
     */
    virtual void notifyUpdateSubnet(Base_Subnet* subnet);
private:
    /*
     * 默认构造函数
     */
    CNotifySubnet();

    /*
     * 默认析构函数
     */
    ~CNotifySubnet();
};
#endif
