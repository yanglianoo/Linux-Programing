#include "pool.h"
#include <iostream>
#include <string.h>
#include <string>
#include "unistd.h"
using namespace std;

template<typename T>
Threadpool<T>::Threadpool(int min,int max)
{
    do
    {
        taskQ = new TaskQueue<T>;
        if(taskQ == nullptr)
        {
            cout<<"malloc taskQ fail...\n";
            break;
        }
        threadIDs = new pthread_t[max];
        if(threadIDs == nullptr)
        {
            cout<<"malloc Threadpool fail...\n";
            break;
        }

        memset(threadIDs, 0 ,sizeof(pthread_t) * max );

        minNum = min;
        maxNum = max;
        busyNum = 0;
        liveNum = min;
        exitNum = 0;

        if(pthread_mutex_init(&m_mutexpool,NULL) !=0 ||
        pthread_cond_init(&m_notEmpty,NULL) !=0 )
        {
            cout<<"mutex or condition init fail ....\n";
        }

        shutdown = false;

        pthread_create(&managerID,NULL,manager,this);
        for(int i = 0; i < min; ++i)
        {
            pthread_create(&threadIDs[i],NULL,worker,this);
        }
        return ;
    }while(0);

    //释放资源
    if(threadIDs) delete [] threadIDs;
    if(taskQ) delete taskQ;
}

template<typename T>
void Threadpool<T>::threadExit()
{
    pthread_t tid = pthread_self();
    for(int i = 0; i < maxNum; ++i)
    {
        if(threadIDs[i] == tid)
        {
            threadIDs[i] = 0;
            cout<<"threadExit() called"<< to_string(tid) <<"exiting...\n";
            break;
        }
    }
    pthread_exit(NULL);
}

template<typename T>
void* Threadpool<T>::worker(void* arg)
{
    Threadpool* pool = static_cast<Threadpool*>(arg);

    while (true)
    {
        pthread_mutex_lock(&pool->m_mutexpool);
        //判断当前任务队列是否为空
        while (pool->taskQ->taskNumber() == 0 && !pool->shutdown)
        {
            //阻塞工作线程
            pthread_cond_wait(&pool->m_notEmpty, &pool->m_mutexpool);
            
            //判断是否要销毁线程
            if(pool->exitNum > 0 )
            {
                pool->exitNum--;
                if(pool->liveNum > pool->minNum)
                {
                    pool->liveNum--;
                    pthread_mutex_unlock(&pool->m_mutexpool);
                    pool->threadExit();
                }
            }

        }

        //判断线程池是否关闭了
        if(pool->shutdown)
        {
            pthread_mutex_unlock(&pool->m_mutexpool);
            pool->threadExit();
        }

        //从任务队列中取出一个任务
        Task<T> task = pool->taskQ->takeTask();

        pool->busyNum++;
        //解锁
        pthread_mutex_unlock(&pool->m_mutexpool);

        cout<<"thread "<< to_string(pthread_self()) << "start working .....\n";
        //执行函数
        task.func(task.arg);
        delete task.arg;
        task.arg = nullptr;

        cout<<"thread"<<to_string(pthread_self())<<"end working .....\n";

        pthread_mutex_lock(&pool->m_mutexpool);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->m_mutexpool);
    }
    return NULL;
}

template<typename T>
void* Threadpool<T>::manager(void* arg)
{
    Threadpool* pool = static_cast<Threadpool*>(arg);
    while (!pool->shutdown)
    {   
        //每隔3s检测一次
        sleep(3);
        // 取出线程池中任务的数量和当前线程的数量
        pthread_mutex_lock(&pool->m_mutexpool);
        int queueSize = pool->taskQ->taskNumber();
        int liveNum = pool->liveNum;
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->m_mutexpool);

        //添加线程 任务的个事故>存活的线程个数 && 存活的线程数 < 最大线程数
        if(queueSize > liveNum && liveNum < pool->maxNum)
        {
            pthread_mutex_lock(&pool->m_mutexpool);
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
            pthread_mutex_unlock(&pool->m_mutexpool);
        }

        //销毁线程 忙的线程*2 < 存活的线程 && 存活的线程 > 最小线程数
        if(busyNum*2 < liveNum && liveNum > pool->minNum)
        {
            pthread_mutex_lock(&pool->m_mutexpool);
            pool->exitNum = NUMBER;
            pthread_mutex_unlock(&pool->m_mutexpool);
            //让工作的线程自杀
            for(int i=0;i<NUMBER; ++i)
            {
                pthread_cond_signal(&pool->m_notEmpty);
            }
        }
    }
    
    return NULL;
}

template<typename T>
void Threadpool<T>::addTask(Task<T> task)
{
    if(shutdown)
    {
        return;
    }
    //添加任务
    taskQ->addTask(task);
    //唤醒消费者
    pthread_cond_signal(&m_notEmpty);
}
template<typename T>
int Threadpool<T>::getAliveNum()
{
    pthread_mutex_lock(&m_mutexpool);
    int aliveNum = this->liveNum;
    pthread_mutex_unlock(&m_mutexpool);
    return aliveNum;
}
template<typename T>
int Threadpool<T>::getBusyNum()
{
    pthread_mutex_lock(&m_mutexpool);
    int busyNum = this->busyNum;
    pthread_mutex_unlock(&m_mutexpool);
    return busyNum;
}

template<typename T>
Threadpool<T>::~Threadpool()
{
    shutdown = 1;
    pthread_join(managerID,NULL);
        //唤醒阻塞的消费者线程
    for(int i = 0; i< liveNum;++i)
    {
        pthread_cond_signal(&m_notEmpty);
    }
        //释放堆内存
    if(taskQ)
    {
        delete taskQ;
    }
    if(threadIDs)
    {
        delete threadIDs;
    }

    pthread_mutex_destroy(&m_mutexpool);
    pthread_cond_destroy(&m_notEmpty);

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
    Threadpool<int> pool(3,10);
    for(int i=0; i<100; ++i)
    {
        int* num = new int(i+100);
        pool.addTask(Task<int>(taskFunc,num));
    }

    sleep(20);
    return 0;
}