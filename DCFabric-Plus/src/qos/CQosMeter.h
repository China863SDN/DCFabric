
#ifndef _CQOS_METER_H_
#define _CQOS_METER_H

class CQosMeter
{
	public:
		CQosMeter(UINT4 meterId, UINT2 flags, UINT2 type, UINT4 rate, UINT4 burst_size, UINT1 priorty_level):
			m_meter_id(meterId),m_flags(flags),m_type(type), m_rate(rate), m_burst_size(burst_size),m_priority_level(priorty_level){};
	private:
		UINT4 m_meter_id;
	    UINT2 m_flags;				//流量计数单位
	    UINT2 m_type;				//qos超速后执行动作
	    UINT4 m_rate;				//最大速率值
	    UINT4 m_burst_size;			//速率浮动值
	    UINT1 m_priority_level;
};
#endif
