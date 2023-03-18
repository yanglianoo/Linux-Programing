#ifndef _POOL_H
#define _POOL_H

#include "TaskQueue.h"
#include "TaskQueue.cpp"
template<typename T>
class Threadpool
{
private:
    TaskQueue<T>* taskQ;
    pthread_t managerID;  //管理者的线程ID
    pthread_t *threadIDs; //工作的线程ID

    int minNum;          //最小线程数量
    int maxNum;          //最大线程数量
    int busyNum;         //忙的线程的个数
    int liveNum;         //存活的线程的个数   
    int exitNum;         //要销毁的线程的个数
     
    pthread_mutex_t m_mutexpool;  //锁整个的线程池
    pthread_cond_t m_notEmpty;  //信号量
    bool shutdown;       //是不是要销毁线程池，销毁为1，不销毁为0
    static const int NUMBER = 2;
private:
    //创建线程池并初始化
    static void* worker(void* arg);
    static void* manager(void* arg);
    void threadExit();
public:
    Threadpool(int min,int max);
    ~Threadpool();

    //给线程池添加任务
    void addTask(Task<T> task);
    //获取线程池中工作的线程的个数
    int getBusyNum();
    //获取线程池中活着的线程的个数
    int getAliveNum();
};



#endif