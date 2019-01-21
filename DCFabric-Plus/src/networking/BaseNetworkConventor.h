#ifndef BASENETWORKCONV_H
#define BASENETWORKCONV_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#include "BaseNetwork.h"
#include "COpenstackResource.h"



using namespace std;

extern Base_Network * BaseNetworkConventor_fromCOpenstackNetwork(COpenstackNetwork* network);

#endif
