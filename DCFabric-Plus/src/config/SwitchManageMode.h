
#ifndef _SWITCH_MANAGE_MODE_H_
#define _SWITCH_MANAGE_MODE_H_

#define  SWITCH_MONITOR_FLAG 0
#define  SWITCH_MANAGE_FLAG 1
class SwitchManageMode
{
	public:
		SwitchManageMode(UINT8 dpid, UINT1 mode){ m_dpid = dpid; m_mode = mode;}
		~SwitchManageMode();
	public:
		UINT8 GetSwitchDpid() const {return m_dpid; }
		void  SetSwitchDpid(UINT8 dpid) {m_dpid = dpid; }
		UINT1 GetSwitchManageMode() const { return m_mode; }
		void  SetSwitchManageMode(UINT1 manage_mode){m_mode = manage_mode; }
	private:
		UINT8  m_dpid;
		UINT1  m_mode;
};
#endif
