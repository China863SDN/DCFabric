#ifndef BASEFLOATINGCONV_H
#define BASEFLOATINGCONV_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#include "BaseFloating.h"
#include "COpenstackResource.h"



using namespace std;

extern Base_Floating * BaseFloatingConventor_fromCOpenstackFloatingip(const COpenstackFloatingip* floatingip);

#endif
