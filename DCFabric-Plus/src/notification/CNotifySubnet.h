#ifndef _CNOTIFYSUBNET_H_
#define  _CNOTIFYSUBNET_H_
#include "CNotifyHandler.h"

class CNotifySubnet: public CNotifyHandler
{
	public:
    /*
     * ��������Ĭ�Ϲ��캯��
     *
     * @param: center           ֪ͨ������
     */
    CNotifySubnet(CNotificationCenter* center): CNotifyHandler(center, bnc::notify::SUBNET) { };

    /*
     * ��������subnet
     *
     * @param: port             subnetָ��
     *
     * @return: None
     */
    virtual void notifyAddSubnet(COpenstackSubnet* subnet);

    /*
     * ����ɾ��subnet
     *
     * @param: port_id          subnet��PortId
     *
     * @return: None
     */
    virtual void notifyDelSubnet(const std::string & subnet_id);

	/*
     * ����update subnet
     *
     * @param: network             subnetָ��
     *
     * @return: None
     */
    virtual void notifyUpdateSubnet(COpenstackSubnet* subnet);

	/*
     * ��������subnet
     *
     * @param: port             subnetָ��
     *
     * @return: None
     */
    virtual void notifyAddSubnet(Base_Subnet* subnet);

	/*
     * ����update subnet
     *
     * @param: network             subnetָ��
     *
     * @return: None
     */
    virtual void notifyUpdateSubnet(Base_Subnet* subnet);
private:
    /*
     * Ĭ�Ϲ��캯��
     */
    CNotifySubnet();

    /*
     * Ĭ����������
     */
    ~CNotifySubnet();
};
#endif
