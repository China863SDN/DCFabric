
#ifndef _SWITCH_MANAGE_PRESISTENCE_H_
#define _SWITCH_MANAGE_PRESISTENCE_H_

#include "SwitchManageMode.h"

class SwitchManagePresistence
{
	public:
		static SwitchManagePresistence* GetInstance();

	private:
		SwitchManagePresistence();
		~SwitchManagePresistence();

	public:
		void setGlobalManageMode(BOOL manage_mode){m_global_manage_mode = manage_mode; }
		BOOL isGlobalManageModeOn() const {return  m_global_manage_mode; }
		SwitchManageMode* findManageSwitchByDpid(UINT8 dpid);
		void update_switch_manage_list(UINT8 dpid, UINT1 manage_flag );

		UINT8 getCommentSwitchDpid();
		INT4 read_switch_manage_config();
		
	private:
		UINT8  m_comment_dpid;
		BOOL  m_global_manage_mode;
		std::map<UINT8, SwitchManageMode*>     m_switch_manage_map;
		static SwitchManagePresistence*  m_pInstance;
		
};



#endif
