#ifndef _CNOTIFYNETWORK_H_
#define _CNOTIFYNETWORK_H_

#include "CNotifyHandler.h"

class CNotifyNetwork : public CNotifyHandler
{
public:
    /*
     * ��������Ĭ�Ϲ��캯��
     *
     * @param: center           ֪ͨ������
     */
    CNotifyNetwork(CNotificationCenter* center): CNotifyHandler(center, bnc::notify::NETWORK) { };

    /*
     * ��������network
     *
     * @param: port             networkָ��
     *
     * @return: None
     */
    virtual void notifyAddNetwork(COpenstackNetwork* network);

    /*
     * ����ɾ��OpenstackPort
     *
     * @param: port_id          OpenstackPort��PortId
     *
     * @return: None
     */
    virtual void notifyDelNetwork(const std::string & network_id);

	
	 /*
     * ����update network
     *
     * @param: port             networkָ��
     *
     * @return: None
     */
    virtual void notifyUpdateNetwork(COpenstackNetwork* network);

	/*
     * ��������network
     *
     * @param: port             networkָ��
     *
     * @return: None
     */
	virtual void notifyAddNetwork(Base_Network* network);
	
	/*
     * ����update network
     *
     * @param: port             networkָ��
     *
     * @return: None
     */
	virtual void notifyUpdateNetwork(Base_Network* network);
private:
    /*
     * Ĭ�Ϲ��캯��
     */
    CNotifyNetwork();

    /*
     * Ĭ����������
     */
    ~CNotifyNetwork();
};
#endif
