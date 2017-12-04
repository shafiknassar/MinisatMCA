/*
 * global_defs.h
 *
 *  Created on: Dec 4, 2017
 *      Author: shafiknassar
 */

#ifndef CORE_GLOBAL_DEFS_H_
#define CORE_GLOBAL_DEFS_H_

#define DEBUG 1
//#undef DEBUG

#ifdef DEBUG


#define TRACE(s)	std::cout << __FUNCTION__ << ": " << s << std::endl
#include <iostream>

#else //if not DEBUG
#define TRACE(s) ;
#endif


#endif /* CORE_GLOBAL_DEFS_H_ */
