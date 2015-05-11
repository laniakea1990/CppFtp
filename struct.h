#ifndef _STRUCT_
#define _STRUCT_

#include "ThreadPoll.h"
typedef struct
{
	char command[5];
	void (*funp)(int fd , char * args , ThreadPoll * p);
}COM;

#endif
