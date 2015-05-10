typedef struct
{
	char command[5];
	void (*funp)(int fd , char * args);
}COM;
