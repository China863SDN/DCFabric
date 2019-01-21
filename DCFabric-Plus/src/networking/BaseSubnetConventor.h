#ifndef BASESUBNETCONV_H
#define BASESUBNETCONV_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#include "BaseSubnet.h"
#include "COpenstackResource.h"



using namespace std;

extern Base_Subnet * BaseSubnetConventor_fromCOpenstackSubnet(COpenstackSubnet* subnet);

#endif
