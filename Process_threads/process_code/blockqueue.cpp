#include "blockqueue.h"

using namespace std;
Blockqueue* bq = new Blockqueue(10000);
pthread_mutex_t c_lock;
pthread_mutex_t p_lock;

void* con_run(void* arg)
{
    char* id = (char*)arg;
    while (1)
    {
        int data;
        pthread_mutex_lock(&c_lock);
        Task ret;
        bq->Get(ret);
        printf("消费者%s完成任务,%d+%d=%d\n",id,ret._x,ret._y,ret.sum());
        pthread_mutex_unlock(&c_lock);
    }
    
}

void* pro_run(void* arg)
{
    char* id = (char*)arg;
    while (1)
    {
        pthread_mutex_lock(&p_lock);
        int x = rand()%10000+1;
        int y = rand()%10000+1;
        Task task(x,y);
        bq->Push(task);
        printf("生产者%s派发任务: %d+%d=?\n",id,task._x,task._y);
        pthread_mutex_unlock(&p_lock);
    }
    
}

int main()
{
   
   pthread_t con1,con2,con3,con4,pro1,pro2,pro3,pro4; //多个消费者和生产者线程
   pthread_mutex_init(&c_lock,nullptr);
   pthread_mutex_init(&p_lock,nullptr);
   pthread_create(&con1,nullptr,con_run,(void*)"1号");
   pthread_create(&con2,nullptr,con_run,(void*)"2号");
   pthread_create(&con3,nullptr,con_run,(void*)"3号");
   pthread_create(&con4,nullptr,con_run,(void*)"4号");

   pthread_create(&pro1,nullptr,pro_run,(void*)"1号");
   pthread_create(&pro2,nullptr,pro_run,(void*)"2号");
   pthread_create(&pro3,nullptr,pro_run,(void*)"3号");
   pthread_create(&pro4,nullptr,pro_run,(void*)"4号");

   pthread_join(con1,nullptr);
   pthread_join(con2,nullptr);
   pthread_join(con3,nullptr);
   pthread_join(con4,nullptr);

   pthread_join(pro1,nullptr);
   pthread_join(pro2,nullptr);
   pthread_join(pro3,nullptr);
   pthread_join(pro4,nullptr);
   
   pthread_mutex_destroy(&c_lock);
   pthread_mutex_destroy(&p_lock);
   delete bq;
    return 0;
}
