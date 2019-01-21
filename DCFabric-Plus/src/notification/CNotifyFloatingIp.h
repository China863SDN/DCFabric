#ifndef _CNOTIFYFLOATINGIP_H_
#define _CNOTIFYFLOATINGIP_H_
#include "CNotifyHandler.h"

class CNotifyFloatingIp : public CNotifyHandler
{
	public:

		/*
	     * 带参数的默认构造函数
	     *
	     * @param: center           通知中心类
	     */
        CNotifyFloatingIp(CNotificationCenter* center)
                : CNotifyHandler(center, bnc::notify::FLOATINGIP) { }
		/*
		 * 处理增加Floatingip
		 *
		 * @param: Floatingip			  Floatingip指针
		 *
		 * @return: None
		 */
		virtual void notifyAddFloatingIp(COpenstackFloatingip* floatingip) ;
		
		
		/*
		 * 处理删除Floatingip
		 *
		 * @param: Floatingip id		  Floatingip的Floatingip id
		 *
		 * @return: None
		 */
		virtual void notifyDelFloatingIp(const std::string & floatingip_id) ;
		
		/*
		 * 处理update Floatingip
		 *
		 * @param: Floatingip			  Floatingip指针
		 *
		 * @return: None
		 */
		virtual void notifyUpdateFloatingIp(COpenstackFloatingip* floatingip) ;


		/*
		 * 处理增加Floatingip
		 *
		 * @param: Floatingip			  Floatingip指针
		 *
		 * @return: None
		 */
		virtual void notifyAddFloatingIp(Base_Floating * floatingip);

		/*
		 * 处理update Floatingip
		 *
		 * @param: Floatingip			  Floatingip指针
		 *
		 * @return: None
		 */
		virtual void notifyUpdateFloatingIp(Base_Floating * floatingip);
	private:
		CNotifyFloatingIp();
		~CNotifyFloatingIp();
};

#endif
