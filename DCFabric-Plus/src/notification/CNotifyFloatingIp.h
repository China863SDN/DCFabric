#ifndef _CNOTIFYFLOATINGIP_H_
#define _CNOTIFYFLOATINGIP_H_
#include "CNotifyHandler.h"

class CNotifyFloatingIp : public CNotifyHandler
{
	public:

		/*
	     * ��������Ĭ�Ϲ��캯��
	     *
	     * @param: center           ֪ͨ������
	     */
        CNotifyFloatingIp(CNotificationCenter* center)
                : CNotifyHandler(center, bnc::notify::FLOATINGIP) { }
		/*
		 * ��������Floatingip
		 *
		 * @param: Floatingip			  Floatingipָ��
		 *
		 * @return: None
		 */
		virtual void notifyAddFloatingIp(COpenstackFloatingip* floatingip) ;
		
		
		/*
		 * ����ɾ��Floatingip
		 *
		 * @param: Floatingip id		  Floatingip��Floatingip id
		 *
		 * @return: None
		 */
		virtual void notifyDelFloatingIp(const std::string & floatingip_id) ;
		
		/*
		 * ����update Floatingip
		 *
		 * @param: Floatingip			  Floatingipָ��
		 *
		 * @return: None
		 */
		virtual void notifyUpdateFloatingIp(COpenstackFloatingip* floatingip) ;


		/*
		 * ��������Floatingip
		 *
		 * @param: Floatingip			  Floatingipָ��
		 *
		 * @return: None
		 */
		virtual void notifyAddFloatingIp(Base_Floating * floatingip);

		/*
		 * ����update Floatingip
		 *
		 * @param: Floatingip			  Floatingipָ��
		 *
		 * @return: None
		 */
		virtual void notifyUpdateFloatingIp(Base_Floating * floatingip);
	private:
		CNotifyFloatingIp();
		~CNotifyFloatingIp();
};

#endif
