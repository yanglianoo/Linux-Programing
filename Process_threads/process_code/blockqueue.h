#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <queue>

using namespace std;
class Task
{
public:
   int _x;
   int _y;
public:
    Task(int x,int y):_x(x),_y(y)
    {}
    Task()
    {}
    int sum()
    {
        return _x+_y;
    }
};


class Blockqueue
{
private:
    pthread_mutex_t lock; //定义锁
    pthread_cond_t consumer_cond; //消费者的等待条件
    pthread_cond_t productor_cond; //生产者的等待条件
    queue<Task> q; //队列
    size_t _cpacity; //队列最大容量
private:

    void QueueLock()//加锁
    {
        pthread_mutex_lock(&lock);
    }
    void QueueUnlock()//解锁
    {
        pthread_mutex_unlock(&lock);
    }
    bool IsFull()
    {
        return q.size()>=(_cpacity*0.95); //如果大于容量的95%就算满了
    }
    bool IsEmpty()
    {
        return q.size()<=(_cpacity*0.05); //如果小于容量的%5就算空了
    }
    void WakeUpConsumber() //唤醒消费者
    {
        cout<<"【唤醒消费者】"<<endl;
        pthread_cond_signal(&consumer_cond);
    }
    void WakeUpProductor() //唤醒生产者
    {
        cout<<"【唤醒生产者】"<<endl;
        pthread_cond_signal(&productor_cond);
    }
    void ConsumberWait()
    {
        cout<<"队列已经空了，消费者睡觉了"<<endl;
        pthread_cond_wait(&consumer_cond,&lock);
    }
    void ProductorWait()
    {
        cout<<"队列已经满了，生产者睡觉了"<<endl;
        pthread_cond_wait(&productor_cond,&lock);
    }
public:
    Blockqueue(int capacity):_cpacity(capacity)
    {
        pthread_mutex_init(&lock,nullptr);
        pthread_cond_init(&consumer_cond,nullptr);
        pthread_cond_init(&productor_cond,nullptr);
    }
    ~Blockqueue()
    {
        pthread_mutex_destroy(&lock);
        pthread_cond_destroy(&consumer_cond);
        pthread_cond_destroy(&productor_cond);
    }
    size_t getcapacity()
    {
        return _cpacity-q.size();
    }
    void Push(Task val)
    {
        QueueLock();
        if(IsFull())
        {
            WakeUpConsumber();//唤醒消费者
            ProductorWait();  //生产者睡觉
        }
        q.push(val);
        QueueUnlock();
    }
    void Get(Task& val)
    {
        QueueLock();
        if(IsEmpty())
        {
            WakeUpProductor(); //唤醒生产者
            ConsumberWait();   //消费者睡觉
        }
        val = q.front();
        q.pop();
        QueueUnlock();
    }
};
