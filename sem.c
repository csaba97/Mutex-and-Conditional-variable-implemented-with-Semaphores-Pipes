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

#define MAX_IN_CRIT_REGION 8
#define MAX_LENGTH_SEM_LIST 100
#define MAX_SEMS 20

typedef struct LOCK{
	int nr;
}pthread_lock;

typedef struct COND{
	int semIndex;
	int semList[MAX_LENGTH_SEM_LIST+1];//maximum 100 waiting
}pthread_cond;

int critical,sem_id1,sem_id2,nrLock=-1,nrCond=-1;
pthread_lock lock;
pthread_cond cond;

void P(int semId, int semNr)
{
	struct sembuf op = {semNr, -1, 0};
	semop(semId, &op, 1);
}

void V(int semId, int semNr)
{
	struct sembuf op = {semNr, +1, 0};
	semop(semId, &op, 1);
}




void initMutex(pthread_lock *lock){
	if(nrLock>=MAX_SEMS){
		printf("Cannot create more locks\n");
		return;
	}
	nrLock++;
	semctl(sem_id1,nrLock, SETVAL, 1);
	lock->nr=nrLock;
}

void lock_mutex(pthread_lock *lock)
{
	P(sem_id1,lock->nr);
}


void unlock_mutex(pthread_lock *lock)
{
	V(sem_id1,lock->nr);
}

void initCond(pthread_cond *cond){
	cond->semIndex=-1;
}

void wait_cond(pthread_cond *cond,pthread_lock *lock)
{
	unlock_mutex(lock);
	if(nrCond>=MAX_SEMS){
		printf("Cannot create more conditional variables\n");
		return;
	}
	nrCond++;
	semctl(sem_id2,nrCond,SETVAL, 0);
	if(cond->semIndex>=MAX_LENGTH_SEM_LIST){
		printf("Cannot add more items into condition variable's list\n");
		return;
	}
	//add a semaphore to the list
	cond->semList[++cond->semIndex]=nrCond;
	P(sem_id2,nrCond);
	lock_mutex(lock);
}

void cond_signal(pthread_cond *cond)
{
	//if semaphore list not empty
	if(cond->semIndex>=0){
		//get last semaphore from list and remove it
		V(sem_id2,cond->semList[cond->semIndex]);
		cond->semIndex--;
	}
}

void cond_broadcast(pthread_cond *cond)
{
	//signal all semaphores in the list and then remove them
	int i;
	for(i=0;i<=cond->semIndex;i++){
		V(sem_id2,cond->semList[i]);
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
	sem_id1 = semget(IPC_PRIVATE, MAX_SEMS+1, IPC_CREAT | 0600);
	sem_id2 = semget(IPC_PRIVATE, MAX_SEMS+1, IPC_CREAT | 0600);
	initMutex(&lock);
	initCond(&cond);
	srand(time(NULL));

	for(i=0;i<10;i++)
	pthread_create(&t[i],NULL,example,NULL);

	for(i=0;i<10;i++)
	pthread_join(t[i],NULL);


	return 0;
}
