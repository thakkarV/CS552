#ifndef SYS_TYPES
#define SYS_TYPES

#define false 0
#define true 1
#define NULL 0

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned long int uint32;
typedef unsigned long long int uint64;

typedef signed char sint8, s8;
typedef signed short int sint16, s16;
typedef signed long int sint32, s32;
typedef signed long long int sint64, s64;

#ifndef _SIZE_T
typedef unsigned size_t;
#define _SIZE_T 1
#endif

typedef signed int bool;

typedef unsigned long uint;
typedef signed long sint;

#ifndef _STDINT_
#define _STDINT_
typedef uint8 uint8_t;
typedef uint16 uint16_t;
typedef uint32 uint32_t;
typedef uint64 uint64_t;
#endif

// SCHEDULER
typedef int tid_t;


#endif // SYS_TYPES
