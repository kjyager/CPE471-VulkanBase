#ifndef _VULKAN_INTRO_COMMON_H_
#define _VULKAN_INTRO_COMMON_H_

#define _STRIFY(_VAR) "" #_VAR ""
#define STRIFY(_VAR) _STRIFY(_VAR)

#define CLAMP(_V, _LO, _HI) (_V) < (_HI) ? (_V) > (_LO) ? (_V) : (_LO) : (_HI) 

#endif