#ifndef CHOSTTRACK_H_
#define CHOSTTRACK_H_

#include "CTimer.h"

class CHostTrack
{
public:
	CHostTrack();
	~CHostTrack();

	INT4 init();

private:
	CTimer m_timer;
};

#endif
