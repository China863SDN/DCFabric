
#include "bnc-type.h"
#include "bnc-error.h"
#include "CConf.h"
#include "comm-util.h"
#include "SwitchManageMode.h"
#include "SwitchManagePresistence.h"

SwitchManagePresistence*  SwitchManagePresistence::m_pInstance = NULL;

SwitchManagePresistence::SwitchManagePresistence():m_global_manage_mode(FALSE),m_comment_dpid(0)
{
	const INT1* value = NULL;
	UINT1 dpid_switch[8] = {0};
	const INT1* result =  CConf::getInstance()->getConfig("switch_manage", "manage_mode_on");
    m_global_manage_mode = (result == NULL) ? FALSE : ( atoi(result) != 0);

	
	value =  CConf::getInstance()->getConfig("switch_manage", "comment_switch_dpid");
	dpid_str_to_bin((INT1 *)value, dpid_switch);
	uc8_to_ulli64 (dpid_switch, &m_comment_dpid);
}
SwitchManagePresistence::~SwitchManagePresistence()
{
	
}

SwitchManageMode* SwitchManagePresistence::findManageSwitchByDpid(UINT8 dpid)
{
	STL_FOR_LOOP(m_switch_manage_map, iter)
	{
		if((dpid == iter->first)&&(NULL != iter->second))
		{
			return iter->second;
		}
	}
	return NULL;
}


void SwitchManagePresistence::update_switch_manage_list(UINT8 dpid, UINT1 manage_flag )
{
	BOOL bFindFlag = FALSE;
	SwitchManageMode*  switch_manage = NULL; 

	STL_FOR_LOOP(m_switch_manage_map, iter)
	{
		if((dpid == iter->first)&&(NULL != iter->second))
		{
			iter->second->SetSwitchManageMode(manage_flag);
			bFindFlag = TRUE;
			return ;
		}
	}
	if(FALSE == bFindFlag)
	{
		switch_manage = new SwitchManageMode(dpid, manage_flag);
		m_switch_manage_map.insert(std::make_pair(dpid, switch_manage));
	}
}

INT4 SwitchManagePresistence::read_switch_manage_config()
{
	UINT4 seq_num = 0;
	UINT2 sw_manage_min_seq = 1;
	UINT2 sw_manage_max_seq = 1000;
	SwitchManageMode*  switch_manage = NULL; 
	
	if(FALSE == m_global_manage_mode)
	{
		return BNC_ERR;
	}
	
	const INT1* value =  CConf::getInstance()->getConfig("switch_manage", "manage_mode_on");
    m_global_manage_mode = (value == NULL) ? FALSE : ( atoi(value) != 0);


	
	for (seq_num = sw_manage_min_seq ; seq_num <= sw_manage_max_seq; seq_num++) 
	{
		UINT8 switch_dpid = 0;
		UINT1 dpid_switch[8] = {0};
		UINT1 manage_mode = 0;
		char switch_dpid_seq[48] = {0};
		char switch_manage_mode_seq[48] = {0};
		sprintf(switch_dpid_seq, "switch_dpid_%d", seq_num);
		value =  CConf::getInstance()->getConfig("switch_manage", switch_dpid_seq);
		dpid_str_to_bin((INT1 *)value, dpid_switch);
		uc8_to_ulli64 (dpid_switch, &switch_dpid);
		
		sprintf(switch_manage_mode_seq, "switch_%d_mode", seq_num);
		manage_mode = (NULL ==(value = CConf::getInstance()->getConfig("switch_manage", switch_manage_mode_seq)))?0 : atoi(value);
		
		

		if(0 != switch_dpid)
		{
			switch_manage = new SwitchManageMode(switch_dpid, manage_mode);
			m_switch_manage_map.insert(std::make_pair(switch_dpid, switch_manage));
		}
	}
	return BNC_OK;
}

UINT8 SwitchManagePresistence::getCommentSwitchDpid()
{
#if 0
	const INT1* value = NULL;
	UINT8 switch_dpid = 0;
	UINT1 dpid_switch[8] = {0};
	if(FALSE == m_global_manage_mode)
	{
		return 0;
	}

	
	value =  CConf::getInstance()->getConfig("switch_manage", "comment_switch_dpid");
	dpid_str_to_bin((INT1 *)value, dpid_switch);
	uc8_to_ulli64 (dpid_switch, &switch_dpid);
	return switch_dpid;
#endif
    return m_comment_dpid;
}

SwitchManagePresistence*  SwitchManagePresistence::GetInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new SwitchManagePresistence();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
	}
	return m_pInstance;
}


