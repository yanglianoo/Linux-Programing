#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <stdlib.h>


typedef struct Task
{
    void (*function) (void* arg);
    void* arg;
}Task;

//线程池结构体
typedef struct Threadpool
{
    
    Task* taskQ;
    int queueCapacity;  //容量
    int queueSize;      //当前任务个数
    int queueFront;     //对头 - > 取数据
    int queueRear;      //队尾 - > 放数据

    pthread_t managerID;  //管理者的线程ID
    pthread_t *threadIDs; //工作的线程ID

    int minNum;          //最小线程数量
    int maxNum;          //最大线程数量
    int busyNum;         //忙的线程的个数
    int liveNum;         //存活的线程的个数   
    int exitNum;         //要销毁的线程的个数
     
    pthread_mutex_t mutexpool;  //锁整个的线程池
    pthread_mutex_t mutexBusy;  //锁busyNum 变量

    pthread_cond_t notFull;    //任务队列是不是满了
    pthread_cond_t notEmpty;   //任务队列是不是空了 

    int shutdown;       //是不是要销毁线程池，销毁为1，不销毁为0
    

}Threadpool;
//创建线程池并初始化

Threadpool *threadPoolCreate(int min,int max,int queueSize);
void* worker(void* arg);
void* manager(void* arg);
void threadExit(Threadpool* pool);

//销毁线程池
int threadPoolDestroy(Threadpool* pool);
//给线程池添加任务
void threadPoolAdd(Threadpool* pool,void(*func)(void*), void* arg);
//获取线程池中工作的线程的个数
int threadPoolBusyNum(Threadpool* pool);
//获取线程池中活着的线程的个数
int threadPoolAliveNum(Threadpool* pool);

#endif