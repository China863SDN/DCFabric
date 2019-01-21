#ifndef BASEPORTCONV_H
#define BASEPORTCONV_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#include "BasePort.h"
#include "COpenstackResource.h"



using namespace std;

extern Base_Port * BasePortConventor_fromCOpenstackPort(const COpenstackPort& port);

#endif

