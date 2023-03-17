#include "threadpool.h"
#include  <pthread.h>
#include  <stdio.h>
#include  <unistd.h>
#include  <string.h>
const int NUMBER = 2;
Threadpool *threadPoolCreate(int min,int max,int queueSize)
{
    Threadpool *pool = (Threadpool *)malloc(sizeof(Threadpool));
    do
    {
        if(pool == NULL)
        {
            printf("malloc Threadpool fail...\n");
            break;
        }

        pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max );
        if(pool->threadIDs == NULL)
        {
            printf("malloc threadIDs fail...\n");
            break;
        }
        memset(pool->threadIDs, 0 ,sizeof(pthread_t) * max );

        pool->minNum = min;
        pool->maxNum = max;
        pool->busyNum = 0;
        pool->liveNum = min;
        pool->exitNum = 0;

        if(pthread_mutex_init(&pool->mutexpool,NULL) !=0 ||
        pthread_mutex_init(&pool->mutexBusy,NULL) !=0 ||
        pthread_cond_init(&pool->notEmpty,NULL) !=0 ||
        pthread_cond_init(&pool->notFull,NULL) !=0 )
        {
            printf("mutex or condition init fail ....\n");
        }

        //任务队列
        pool->taskQ = (Task*)malloc(sizeof(Task) * queueSize);
        pool->queueCapacity = queueSize;
        pool->queueSize = 0;
        pool->queueRear = 0;
        pool->queueFront = 0;
        

        pool->shutdown = 0;

        pthread_create(&pool->managerID,NULL,manager,pool);
        for(int i = 0; i < min; ++i)
        {
            pthread_create(&pool->threadIDs[i],NULL,worker,pool);
        }
        return pool;
    }while(0);

    //释放资源
    if(pool->threadIDs) free(pool->threadIDs);
    if(pool->taskQ) free(pool->taskQ);
    if(pool) free(pool);

    return NULL;
}

void* worker(void* arg)
{
    Threadpool* pool = (Threadpool*)arg;
    while (1)
    {
        pthread_mutex_lock(&pool->mutexpool);
        //判断当前任务队列是否为空
        while (pool->queueSize == 0 && !pool->shutdown)
        {
            //阻塞工作线程
            pthread_cond_wait(&pool->notEmpty, &pool->mutexpool);
            
            //判断是否要销毁线程
            if(pool->exitNum > 0 )
            {
                pool->exitNum--;
                if(pool->liveNum > pool->minNum)
                {
                    pool->liveNum--;
                    pthread_mutex_unlock(&pool->mutexpool);
                    threadExit(pool);
                }
            }

        }

        //判断线程池是否关闭了
        if(pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexpool);
            threadExit(pool);
        }

        //从任务队列中取出一个任务
        Task task;
        task.function = pool->taskQ[pool->queueFront].function;
        task.arg = pool->taskQ[pool->queueFront].arg;

        //移动头节点
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;
        //唤醒生产者
        pthread_cond_signal(&pool->notFull);
        //解锁
        pthread_mutex_unlock(&pool->mutexpool);

        printf("thread %ld start working .....\n",pthread_self());
        //执行函数
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);

        task.function(task.arg);

        free(task.arg);
        task.arg = NULL;

        printf("thread %ld end working .....\n",pthread_self());

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }
    return NULL;
}


void* manager(void* arg)
{
    Threadpool* pool = (Threadpool*)arg;
    while (!pool->shutdown)
    {   
        //每隔3s检测一次
        sleep(3);
        // 取出线程池中任务的数量和当前线程的数量
        pthread_mutex_lock(&pool->mutexpool);
        int queueSize = pool->queueSize;
        int liveNum = pool->liveNum;
        pthread_mutex_unlock(&pool->mutexpool);

        //取出忙的线程的数量
        pthread_mutex_lock(&pool->mutexBusy);
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexBusy);

        //添加线程 任务的个事故>存活的线程个数 && 存活的线程数 < 最大线程数
        if(queueSize > liveNum && liveNum < pool->maxNum)
        {
            pthread_mutex_lock(&pool->mutexpool);
            int counter = 0;
            for(int i = 0;i<pool->maxNum && counter < NUMBER && pool->liveNum < pool-> maxNum; ++i)
            {
                if(pool->threadIDs[i] == 0)
                {
                    pthread_create(&pool->threadIDs[i],NULL,worker,pool);
                    counter++;
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexpool);
        }

        //销毁线程 忙的线程*2 < 存活的线程 && 存活的线程 > 最小线程数
        if(busyNum*2 < liveNum && liveNum > pool->minNum)
        {
            pthread_mutex_lock(&pool->mutexpool);
            pool->exitNum = NUMBER;
            pthread_mutex_unlock(&pool->mutexpool);
            //让工作的线程自杀
            for(int i=0;i<NUMBER; ++i)
            {
                pthread_cond_signal(&pool->notEmpty);
            }
        }
    }
    
    return NULL;
}


void threadExit(Threadpool* pool)
{
    pthread_t tid = pthread_self();
    for(int i = 0; i < pool->maxNum; ++i)
    {
        if(pool->threadIDs[i] == tid)
        {
            pool->threadIDs[i] = 0;
            printf("threadExit() called, %ld exiting...\n",tid);
            break;
        }
    }
    pthread_exit(NULL);
}


void threadPoolAdd(Threadpool* pool,void(*func)(void*), void* arg)
{
    pthread_mutex_lock(&pool->mutexpool);
    
    while (pool->queueSize == pool->queueCapacity && !pool->shutdown)
    {
        //阻塞生产者线程
       
        pthread_cond_wait(&pool->notFull, &pool->mutexpool);
    }
    if(pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutexpool);
        return;
    }
    
    //添加任务
    pool->taskQ[pool->queueRear].function = func;
    pool->taskQ[pool->queueRear].arg = arg;
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    pool->queueSize++;
    //唤醒消费者
    pthread_cond_signal(&pool->notEmpty);

    pthread_mutex_unlock(&pool->mutexpool);
    
}

int threadPoolBusyNum(Threadpool* pool)
{
    pthread_mutex_lock(&pool->mutexBusy);
    int busyNum = pool->busyNum;
    pthread_mutex_unlock(&pool->mutexBusy);
    return busyNum;
}

int threadPoolAliveNum(Threadpool* pool)
{
    pthread_mutex_lock(&pool->mutexpool);
    int AliveNum = pool->liveNum;
    pthread_mutex_unlock(&pool->mutexpool);
    return AliveNum;
}

int threadPoolDestroy(Threadpool* pool)
{
    if(pool == NULL)
    {
        return -1;
    }

    //关闭线程池
    pool->shutdown = 1;
    //阻塞回收管理者线程
    pthread_join(pool->managerID,NULL);
    //唤醒阻塞的消费者线程
    for(int i = 0; i< pool->liveNum;++i)
    {
        pthread_cond_signal(&pool->notEmpty);
    }
    //释放堆内存
    if(pool->taskQ)
    {
        free(pool->taskQ);
    }
    if(pool->threadIDs)
    {
        free(pool->threadIDs);
    }

    pthread_mutex_destroy(&pool->mutexpool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);

    free(pool);
    pool = NULL;

    return 0;
}
void taskFunc(void* arg)
{
    int num = *(int*) arg;
    printf("thread %ld is working,number = %d\n",pthread_self(),num);
    sleep(1);
}
int main()
{
    //创建线程池
    Threadpool* pool =  threadPoolCreate(3,10,100);
    for(int i=0; i<100; ++i)
    {
        int* num = (int*)malloc(sizeof(int));
        *num = i + 100;
        threadPoolAdd(pool,taskFunc,num);
    }

    sleep(30);
    threadPoolDestroy(pool);
    return 0;
}