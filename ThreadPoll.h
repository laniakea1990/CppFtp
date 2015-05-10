#ifndef _ThreadPoll_
#define _ThreadPoll_
#include <map>
#include <pthread.h>
#include <iostream>
class ThreadPoll
{
	private:
		typedef struct
		{
			pthread_t tid;
			void*(*work) (void*);
			void (*unwork) (void);	
			void * args;
		}job;
		std::map<pthread_t , job *> job_wait;
		std::map<pthread_t , job *> job_run;
		std::map<std::string , pthread_key_t *> keys;
		int max_num;
		job temp;
		pthread_mutex_t lock_t=PTHREAD_MUTEX_INITIALIZER;
	public:
		ThreadPoll(int num);
		void destroy();
		void add_job(void* (*wk)(void *) , void (*unwk)(void) , void * arg);
		void lock();
		void unlock();
		void start();
		void stop();
		void set_value(std::string , void *);
		void * get_value(std::string);
		void add_key(std::string);		
		
};


#endif
