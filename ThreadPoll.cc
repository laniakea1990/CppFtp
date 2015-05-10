#include "ThreadPoll.h"


ThreadPoll::ThreadPoll(int num)
{
	max_num = num;
	
}

void ThreadPoll::lock()
{
	pthread_mutex_lock(&lock_t);
}


void ThreadPoll::unlock()
{
	pthread_mutex_unlock(&lock_t);
}


void ThreadPoll::start()
{
	pthread_t tid = pthread_self();
	job * jp = job_wait[tid];
	job_wait.erase(tid);
	job_run.insert(std::pair<pthread_t,job *>(tid,jp));
	std::cout << "one job start | wait = " << job_wait.size() << " start = " << job_run.size() <<std::endl;
	if(job_wait.size()==0)
	{
		for(int i=0 ; i<max_num ; i++)
		{
			job * jp = (job *)malloc(sizeof(job));
			pthread_create(&jp->tid , nullptr , temp.work , temp.args);
			jp->unwork = temp.unwork;
			job_wait.insert(std::pair<pthread_t,job*>(jp->tid , jp));
		}
		std::cout << "add new thread | wait = " << job_wait.size() << " start = " <<job_run.size() << std::endl;
		max_num *= 2;
	}
}


void ThreadPoll::stop()
{
	pthread_t tid = pthread_self();
	job * jp = job_run[tid];
	job_run.erase(tid);
	job_wait.insert(std::pair<pthread_t,job *>(tid,jp));
	std::cout << "one job stop | wait = " << job_wait.size() << " start = " << job_run.size() << std::endl;
	if(job_run.size()<=(max_num/3) && (job_run.size()>3))
	{
		int i=1;
		for(auto tmp : job_wait)
		{
			pthread_cancel(tmp.first);
			job * jp = tmp.second;
			if(jp->unwork != nullptr)
			{
				jp->unwork();
			}
			job_wait.erase(tmp.first);
			free(jp);
			if(i >= (max_num/3))
			{
				break;
			}
			i++;
		}
		max_num = max_num - i;
		std::cout << "remove free thread | wait = " << job_wait.size() << " start = " <<job_run.size() << std::endl;
	}
}


void ThreadPoll::add_job(void* (*wk)(void*) , void (*unwk)(void) , void * arg)
{
	temp.work = wk;
	temp.unwork = unwk;
	temp.args = arg;
	for(int i=0 ; i<max_num ; i++)
	{
		job * jp = (job *)malloc(sizeof(job));
		pthread_create(&jp->tid , nullptr , wk , arg);
		jp->unwork = unwk;
		job_wait.insert(std::pair<pthread_t,job*>(jp->tid , jp));

	}
	std::cout << "add job | wait = " << job_wait.size() << " start = " << job_run.size() << std::endl;

}


void ThreadPoll::destroy()
{
	for(auto tmp : job_run)
	{
		pthread_cancel(tmp.first);
		job * jp = tmp.second;
		if(jp->unwork != nullptr)
		{
			jp->unwork();
		}
		job_run.erase(tmp.first);
		free(jp);
	}
	for(auto tmp : job_wait)
	{
		pthread_cancel(tmp.first);
		job * jp = tmp.second;
		if(jp->unwork != nullptr)
		{
			jp->unwork();
		}
		job_wait.erase(tmp.first);
		free(jp);
	}
	for(auto tmp : keys)
	{
		pthread_key_t * key = tmp.second;
		free(key);
	keys.clear();
	}
	max_num = 0;
	std::cout << "max_num = " << max_num << " job_wait = " << job_wait.size() << " job_run = " << job_run.size() << std::endl;
}


void ThreadPoll::set_value(std::string name , void * data)
{
	pthread_key_t * key = keys[name];
	if(pthread_setspecific(*key , data))
	{
		std::cout << "set data error" << std::endl;
	}
	return;
}


void * ThreadPoll::get_value(std::string name)
{
	pthread_key_t * key = keys[name];
	return pthread_getspecific(*key);
}


void ThreadPoll::add_key(std::string name)
{
	pthread_key_t * key = (pthread_key_t *)malloc(sizeof(pthread_key_t));
	pthread_key_create(key , nullptr);
	keys.insert(std::pair<std::string,pthread_key_t *>(name , key));
	return ;
}
