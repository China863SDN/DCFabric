#include "bnc-type.h"
#include "bnc-error.h"
#include "externalport_config.h"

externalport_config* externalport_config::m_pInstance = NULL;

externalport_config::externalport_config()
{
	
}

externalport_config::~externalport_config()
{
	if(NULL != m_pInstance)
	{
		delete m_pInstance;
	}
}

INT4  externalport_config::read_externalport_config()
{
#if  0
	UINT4 seq_num = 0;
	UINT2 externalport_min_seq = 1;
	UINT2 externalport_max_seq = 20;
	const INT1* value = NULL;
	Base_External*  external_port = NULL; 

	
	for (seq_num = externalport_min_seq ; seq_num <= externalport_max_seq; seq_num++) 
	{
		UINT8 switch_dpid = 0;
		UINT2 switch_port = 0;
		UINT1 dpid_switch[8] = {0};
		char str_seq[48] = {0};
		char network[64] = {0};
		char subnet[64] = {0};
		UINT4 gateway_ip = 0;
		UINT1 gateway_mac[6] = {0};
		
		sprintf(str_seq, "switch_dpid_%d", seq_num);
		value =  CConf::getInstance()->getConfig("external_port", str_seq);
		dpid_str_to_bin((INT1 *)value, dpid_switch);
		uc8_to_ulli64 (dpid_switch, &switch_dpid);

		sprintf(str_seq, "switch_port_%d", seq_num);
		value =  CConf::getInstance()->getConfig("external_port", str_seq);
		switch_port = (NULL == value) ? 0: atoi(value);

		sprintf(str_seq, "gateway_ip_%d", seq_num);
		value =  CConf::getInstance()->getConfig("external_port", str_seq);

		
		sprintf(str_seq, "gateway_mac_%d", seq_num);
		value =  CConf::getInstance()->getConfig("external_port", str_seq);		

		sprintf(str_seq, "gateway_subnet_%d", seq_num);
		value =  CConf::getInstance()->getConfig("external_port", str_seq);		

		
		sprintf(str_seq, "gateway_network_%d", seq_num);
		value =  CConf::getInstance()->getConfig("external_port", str_seq); 	
	}
#endif
	return BNC_OK;
}

externalport_config* externalport_config::getInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new externalport_config();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
	}
	return m_pInstance;
}

