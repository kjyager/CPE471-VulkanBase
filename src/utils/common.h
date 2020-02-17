#pragma once 
#ifndef COMMON_H_
#define COMMON_H_

#include <assert.h>
#include <exception>

#define MAX(_A, _B) ((_A) > (_B) ? (_A) : (_B))
#define MIN(_A, _B) ((_A) < (_B) ? (_A) : (_B))
#define CLAMP(_A, _MIN, _MAX) ( _A > _MAX ? (_MAX) : (_A < _MIN ? (_MIN) : (_A) ) )
#define FNOT(_F) (1.0f - (_F))

#ifndef STRIFY
	#define _STRIFY(_PD) #_PD
	#define STRIFY(_PD) _STRIFY(_PD)
#endif

#define PING() {fprintf(stderr,"PING! (%d:%s)\n",__LINE__,__FILE__);}
#define PINGMSG(_MSG) {fprintf(stderr,"PING! (%d:%s): " STRIFY(_MSG) "\n",__LINE__,__FILE__);}

#ifndef BASETYPES
	#define BASETYPES
	typedef unsigned UINT;
	typedef unsigned long ULONG;
	typedef ULONG *PULONG;
	typedef unsigned short USHORT;
	typedef USHORT *PUSHORT;
	typedef unsigned char UCHAR;
	typedef UCHAR *PUCHAR;
#endif

#define TODO_IMPLEMENT() throw std::runtime_error("TODO: Not implemented! " STRIFY(__FILE__) ":" STRIFY(__LINE__) "");

#define CPE471_VULKAN_SAFETY_RAILS

#endif