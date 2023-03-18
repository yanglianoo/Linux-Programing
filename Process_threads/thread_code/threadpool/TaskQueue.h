#ifndef _TASKQUEUE_H
#define _TASKQUEUE_H
#include <pthread.h>
#include  <queue>
using namespace std;

using callback = void (*) (void* arg);
template<typename T>
struct Task
{
    Task()
    {
        func = nullptr;
        arg = nullptr;
    }
    Task(callback f, void* arg)
    {
        this->arg = (T*)arg;
        func = f;
    }

    callback func;
    T* arg;
};

template<typename T>
class TaskQueue
{
private:
    queue<Task<T>> m_taskQ;
    pthread_mutex_t m_mutex;
public:
    TaskQueue();
    ~TaskQueue();
    //添加任务
    void addTask(Task<T> task);
    void addTask(callback f, void * arg);
    //取出一个任务
    Task<T> takeTask();
    //获取当前任务的个数
    inline size_t taskNumber()
    {
        return m_taskQ.size();
    }
};




#endif