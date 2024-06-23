#ifndef     	 _THREADPOOL_H_
#define 	 _THREADPOOL_H_



int pool_init (int max_thread_num);
int pool_add_worker (void (*process) (int arg), int  arg);
int pool_destroy ();
void * thread_routine (void *arg);

#endif
