#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define MAX_IN_CRIT_REGION 7
#define MAX_LENGTH_SEM_LIST 100
#define MAX_PIPE 20
#define READ_END 0
#define WRITE_END 1

typedef struct LOCK{
	int nr;
}pthread_lock;

typedef struct COND{
	int semIndex;
	int semList[MAX_LENGTH_SEM_LIST+1];//maximum 100 waiting
}pthread_cond;

int critical,nrLock=-1,nrCond=-1,fd_mutex[MAX_PIPE+1][2],fd_cond[MAX_PIPE+1][2];
pthread_lock lock;
pthread_cond cond;

/*IMPORTANT
initMutex() function is not sincronyzed itself so it should not be called in threads etc.
*/
void initMutex(pthread_lock *lock){
	if(nrLock>=MAX_PIPE){
		printf("Cannot create more locks\n");
		return;
	}
	nrLock++;
	pipe(fd_mutex[nrLock]);
	write(fd_mutex[nrLock][WRITE_END],"a",1);//initialise it with 1
	lock->nr=nrLock;
}

void lock_mutex(pthread_lock *lock)
{
	char c;
	read(fd_mutex[nrLock][READ_END],&c,1);
}


void unlock_mutex(pthread_lock *lock)
{
	write(fd_mutex[nrLock][WRITE_END],"a",1);
}

void initCond(pthread_cond *cond){
	cond->semIndex=-1;
}

void wait_cond(pthread_cond *cond,pthread_lock *lock)
{
	unlock_mutex(lock);
	if(nrCond>=MAX_PIPE){
		printf("Cannot create more conditional variables\n");
		return;
	}
	nrCond++;
	pipe(fd_cond[nrCond]);
	if(cond->semIndex>=MAX_LENGTH_SEM_LIST){
		printf("Cannot add more items into condition variable's list\n");
		return;
	}
	//add a 'semaphore' to the list
	cond->semList[++cond->semIndex]=nrCond;
	char c;
	read(fd_cond[nrCond][READ_END],&c,1);
	lock_mutex(lock);
}

void cond_signal(pthread_cond *cond)
{
	//if 'semaphore' list not empty
	if(cond->semIndex>=0){
		//get last 'semaphore' from list and remove it
		write(fd_cond[cond->semList[cond->semIndex]][WRITE_END],"a",1);
		cond->semIndex--;
	}
}

void cond_broadcast(pthread_cond *cond)
{
	//signal all 'semaphores' in the list and then remove them
	int i;
	for(i=0;i<=cond->semIndex;i++){
		write(fd_cond[cond->semList[i]][WRITE_END],"a",1);
	}
	cond->semIndex=-1;
}

void* example(void* arg){
	lock_mutex(&lock);
	while (critical >= MAX_IN_CRIT_REGION) {
		wait_cond(&cond,&lock);
	}
	critical++;
	unlock_mutex(&lock);

	int r=rand()%10;
	usleep(100000 *r);
	printf("Number of threads in critical region is %d\n",critical);

	lock_mutex(&lock);
	critical--;
	cond_signal(&cond);
	unlock_mutex(&lock);
}





int main(int argc, char *argv[])
{
	int i=0;
	pthread_t t[10];
	initMutex(&lock);
	initCond(&cond);
	srand(time(NULL));

	for(i=0;i<10;i++)
	pthread_create(&t[i],NULL,example,NULL);

	for(i=0;i<10;i++)
	pthread_join(t[i],NULL);


	return 0;
}
