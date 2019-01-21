


#ifndef  _CFLOATINGFLOW_H_
#define  _CFLOATINGFLOW_H_
#include "CControl.h"
#include "CHost.h"
#include "BaseExternal.h"
#include "CFloatingIp.h"





class CFloatingFlowInstall
{
	private :
		CFloatingFlowInstall():m_FlowInstall_CheckTime(0){};
	public:
		~CFloatingFlowInstall(){if(NULL != m_pInstance) {delete m_pInstance; m_pInstance=NULL;}}
		INT4 create_proactive_floating_flows_by_floating(CFloatingIp* fip);
		INT4 remove_proactive_floating_flows_by_floating(CFloatingIp* fip);
		static CFloatingFlowInstall*	getInstance();

	private:
		INT4 Init();
		Base_External * get_externalport_by_floatingip(CFloatingIp* fip);
		
		INT4 process_floating_internal_subnet_flow(CSmartPtr<CHost>& fixed_port, CSubnet* subnet, INT4 type);
		INT4 process_proactive_internal_subnet_flow_by_fixed_port(CSmartPtr<CHost>& fixed_port, INT4 type);
		INT4 create_proactive_floating_with_host_flows(CSmartPtr<CHost>& fixed_port, CSmartPtr<CSwitch>& ext_sw, Base_External* ext_port, UINT4 floatingip);
	private:
		UINT4  m_FlowInstall_CheckTime;
		CTimer m_FlowInstall_timer;
		static CFloatingFlowInstall* m_pInstance;
		
};
#endif
