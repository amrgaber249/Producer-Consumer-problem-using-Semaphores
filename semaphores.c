/*
=====================
||PROBLEM STATMENT:||
=====================
You are required to write a C program to solve the following synchronization problem using
POSIX and “semaphore.h” libraries.
N mcounter threads count independent incoming messages in a system, and another thread
mmonitor gets the count of threads at time intervals of size t1, and then resets the counter to
0. The mmonitor then places this value in a buffer of size b, and a mmcollector thread reads
the values from the buffer.
Any thread will have to wait if the counter is being locked by any other thread. Also, the
mmonitor and mcollecttor threads will not be able to access the buffer at the same time or to
add another entry if the buffer is full.
Assume that the messages come randomly to the system, this can be realized if the mcounter
threads sleep for random times, and their activation (sleep time ends) corresponds to an email
arrival. Similarly, the mmonitor and mcollector will be activated at random time intervals.

===============
||SAMPLE RUN:||
===============
>> gcc semaphores.c -o semaphores -lpthread -lrt
>> ./semaphores
counter #: 5
Counter thread 3: received a message
Counter thread 4: received a message
Counter thread 4: waiting to write
Counter thread 1: received a message
Counter thread 1: waiting to write
Collector thread: nothing is in the buffer!
Counter thread 2: received a message
Counter thread 2: waiting to write
Counter thread 3: now adding to counter, counter value=1
Counter thread 0: received a message
Counter thread 0: waiting to write
Counter thread 4: now adding to counter, counter value=2
Counter thread 1: now adding to counter, counter value=3
Monitor thread: reading a count value of 3
Monitor thread: writing to buffer at position 0
Counter thread 2: now adding to counter, counter value=1
Counter thread 0: now adding to counter, counter value=2
Monitor thread: waiting to read counter
Collector thread: reading from buffer at position 0
Monitor thread: reading a count value of 2
Monitor thread: writing to buffer at position 0
Collector thread: reading from buffer at position 0
*/

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

//delay time of each thread
#define t 2

//maximum buffer size
#define MAX 1

//# of execution for mCollector and mMonitor
#define exec 2

//# of continues runs of mCounter
int counter = 0;

/*
(e,s,n) for producer/consumer problem
(x) for mutual exclusion on counter
*/
sem_t e, s, n, x;

//implementation of queue
int intArray[MAX];
int front = 0;
int rear = -1;
int itemCount = 0;

bool isEmpty()
{
    return itemCount == 0;
}

bool isFull()
{
    return itemCount == MAX;
}

int size()
{
    return itemCount;
}

void insert(int data)
{

    if (!isFull())
    {

        if (rear == MAX - 1)
        {
            rear = -1;
        }

        intArray[++rear] = data;
        itemCount++;
    }
}

int removeData()
{
    int data = intArray[front++];

    if (front == MAX)
    {
        front = 0;
    }

    itemCount--;
    return data;
}

/* N mCounter */
/* count independent incoming messages in a system */
void *mC(void *arg)
{
    /* random sleep time between [0,4] */
    sleep(rand() % 5);
    //receive i from main loop
    int k = (int)arg;
    printf("Counter thread %d: received a message\n", k);
    /*
    int sem_trywait(sem_t *sem);

    is used for situations where you can't afford to wait 
    for the lock and you'd rather fail the operation 
    than wait until the lock becomes available. 

    This might happen in hard real-time systems or in 
    some very complex locking schemes where failing 
    is the only way to avoid lock ordering problems.

    it shall lock the semaphore referenced by sem only if the semaphore is currently 
    not locked (if the semaphore value is currently positive). 
    Otherwise, it shall not lock the semaphore.

    >>return zero if the calling is successful
    >>return -1 if the calling is unsuccessful
    */
    if (sem_trywait(&x) == 0)
    {
        /*
        to let the thread take as much time as possible in its critical section
        to avoid race condition.
        */
        sleep(t);
        counter++;
        printf("Counter thread %d: now adding to counter, counter value=%d\n", k, counter);

        //increment the semaphore
        sem_post(&x);
    }
    else
    {
        printf("Counter thread %d: waiting to write\n", k);
        //decrement the semaphore
        sem_wait(&x);
        /*
        to let the thread take as much time as possible in its critical section
        to avoid race condition.
        */
        sleep(t);
        counter++;
        printf("Counter thread %d: now adding to counter, counter value=%d\n", k, counter);
        //increment the semaphore
        sem_post(&x);
    }
}

/* mMonitor */
/* gets the count of threads at time intervals of size */
void *nM(void *arg)
{
    for (int i = 0; i < exec; i++)
    {
        /* random sleep time between [0,4] */
        sleep(rand() % 5);
        /* refer to mCounter function for full DESCRIPTION */
        if (sem_trywait(&e) == 0)
        {
            sleep(t);
            if (sem_trywait(&s) == 0)
            {
                sleep(t);
                printf("Monitor thread: reading a count value of %d\n", counter);
                printf("Monitor thread: writing to buffer at position %d\n", itemCount);
                int temp = counter;
                insert(temp);
                counter = 0;
                sem_post(&s);
                sem_post(&n);
            }
            else
            {
                printf("Monitor thread: waiting to read counter\n");
                sem_wait(&s);
                sleep(t);
                printf("Monitor thread: reading a count value of %d\n", counter);
                printf("Monitor thread: writing to buffer at position %d\n", itemCount);
                int temp = counter;
                insert(temp);
                counter = 0;
                sem_post(&s);
                sem_post(&n);
            }
        }
        else
        {
            printf("Monitor thread: Buffer full!!\n");
            sem_wait(&e);
            sleep(t);
            if (sem_trywait(&s) == 0)
            {
                sleep(t);
                printf("Monitor thread: reading a count value of %d\n", counter);
                printf("Monitor thread: writing to buffer at position %d\n", itemCount);
                int temp = counter;
                insert(temp);
                counter = 0;
                sem_post(&s);
                sem_post(&n);
            }
            else
            {
                printf("Monitor thread: waiting to read counter\n");
                sem_wait(&s);
                sleep(t);
                printf("Monitor thread: reading a count value of %d\n", counter);
                printf("Monitor thread: writing to buffer at position %d\n", itemCount);
                int temp = counter;
                insert(temp);
                counter = 0;
                sem_post(&s);
                sem_post(&n);
            }
        }
    }
}

/* mCollector */
/* reads the values from the buffer */
void *nC(void *arg)
{
    for (int i = 0; i < exec; i++)
    {
        /* random sleep time between [0,4] */
        sleep(rand() % 5);
        /*refer to mCounter function for full DESCRIPTION */
        if (sem_trywait(&n) == 0)
        {
            sleep(t);
            if (sem_trywait(&s) == 0)
            {
                sleep(t);
                printf("Collector thread: reading from buffer at position %d\n", itemCount - 1);
                int num = removeData();
                sem_post(&s);
                sem_post(&e);
            }
            else
            {
                printf("Collector thread: waiting to read from buffer\n");
                sem_wait(&s);
                sleep(t);
                printf("Collector thread: reading from buffer at position %d\n", itemCount - 1);
                int num = removeData();
                sem_post(&s);
                sem_post(&e);
            }
        }
        else
        {
            printf("Collector thread: nothing is in the buffer!\n");
            sem_wait(&n);
            sleep(t);
            if (sem_trywait(&s) == 0)
            {
                sleep(t);
                printf("Collector thread: reading from buffer at position %d\n", itemCount - 1);
                int num = removeData();
                sem_post(&s);
                sem_post(&e);
            }
            else
            {
                printf("Collector thread: waiting to read from buffer\n");
                sem_wait(&s);
                sleep(t);
                printf("Collector thread: reading from buffer at position %d\n", itemCount - 1);
                int num = removeData();
                sem_post(&s);
                sem_post(&e);
            }
        }
    }
}



int main()
{
    pthread_t mCollector, mMonitor;
    pthread_t *mCounter;

    int N;
    printf("counter #: ");
    scanf("%d", &N);

    sem_init(&s, 0, 1); /* sem s = 1  */
    sem_init(&n, 0, 0); /* sem n = 0  */
    sem_init(&e, 0, N); /* sem e = buffer size  */
    sem_init(&x, 0, 1); /* sem x = 1  */

    mCounter = (pthread_t *)malloc(N * sizeof(pthread_t));

    for (int i = 0; i < N; i++)
        pthread_create(&mCounter[i], NULL, mC, (void *)(i));

    pthread_create(&mMonitor, NULL, nM, NULL);
    pthread_create(&mCollector, NULL, nC, NULL);

    pthread_join(mMonitor, NULL);
    pthread_join(mCollector, NULL);

    for (int i = 0; i < N; i++)
        pthread_join(mCounter[i], NULL);

    return 0;
}