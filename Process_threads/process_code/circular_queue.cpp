#include "circular_queue.h"
#include <unistd.h>

CircularQueue* bq=new CircularQueue(10);

void* consumer_run(void* arg)
{
  while(1)
  {
    int data;
    bq->Get(data);
    printf("%s拿到数据：%d\n",(char*)arg,data);
  }
}

void* productor_run(void* arg)
{

  int cout=1;
  while(1)
  {
    if(cout==11)
      cout=1;
    bq->Put(cout);
    printf("%s放进数据：%d\n",(char*)arg,cout);
    cout++;
    sleep(1);

  }
}

int main()
{
  pthread_t con,pro;
  pthread_create(&con,nullptr,consumer_run,(void*)"消费者");
  pthread_create(&pro,nullptr,productor_run,(void*)"生产者");

  pthread_join(con,nullptr);
  pthread_join(pro,nullptr);
  delete bq;
  return 0;

}

