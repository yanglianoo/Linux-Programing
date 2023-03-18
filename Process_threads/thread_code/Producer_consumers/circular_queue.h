#include <iostream>
#include <vector>
#include <pthread.h>
#include <semaphore.h>
using namespace std;

class CircularQueue
{
private:
  vector<int> v;//用作循环队列
  int _cap;//队列大小

  sem_t data;//消费者信号量（消费者关注数据数量）
  sem_t blank;//生产者信号量（生产者关注空格数量）
  
  int com_index;//消费者的索引
  int pro_index;//生产者的索引

public:
  CircularQueue(int cap=10):_cap(cap)
  {
    v.resize(_cap);
    sem_init(&data,0,0);//初始化消费者信号量
    sem_init(&blank,0,_cap);///初始化生产者信号量
    com_index=0;
    pro_index=0;//索引开始时都指向0
  }
  ~CircularQueue()
  {
    sem_destroy(&data);//销毁信号量
    sem_destroy(&blank);
  }

  void Put(const int& val)//生产者存放数据
  {   
    sem_wait(&blank);//P操作，生产者申请的是空格资源
    v[pro_index]=val;
    pro_index++;
    pro_index%=_cap;//环形队列
    sem_post(&data);//V操作，生产者释放的数据

  }
  void Get(int& val)//消费者取数据
  {
    sem_wait(&data);//P操作，消费者申请的是数据
    val=v[com_index];
    com_index++;
    com_index%=_cap;//环形
    sem_post(&blank);//V操作，消费者释放的是空格
  }
};
