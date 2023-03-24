#include "threadpool.h"
#include <iostream>
#include <chrono>

using ULong = unsigned long long;


class MyTask :public Task
{
public:
  MyTask(int begin, int end)
    :begin_(begin)
    , end_(end)
  {};
  Any run() 
  {
      std::cout << "tid:" << std::this_thread::get_id()
          <<"begin!" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(3));
      ULong sum = 0;
      for (ULong i = begin_; i <= end_; i++)
        sum += i;
      std::cout << "tid:" << std::this_thread::get_id()
          <<"end!" << std::endl;
      return sum;
  }
private:
  int begin_;
  int end_;
};
//知识漏洞---如何调试多线程，如何去定位死锁
        
//
//int main()
//{
//  {
//    ThreadPool pool;
//    pool.setMode(PoolMode::MODE_CACHED);
//    pool.start(4);
//    for (int i = 0; i < 200; i++)
//    {
//      pool.submitTask(std::make_shared<MyTask>(1, 100000000));
//    }
//    
//
//  }
//    
//
//    
//  std::cout << "main over" << std::endl;
// 
//  //{
//  //  ThreadPool pool;
//  //  pool.setMode(PoolMode::MODE_CACHED);
//  //  pool.start(4);
//  //  Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
//  //  Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
//  //  Result res3 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
//  //  Result res4 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
//  //  Result res5 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
//  //  Result res6 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
//  //  Result res7 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
//  //  ULong sum1 = res1.get().cast_<ULong>();
//  //  ULong sum2 = res2.get().cast_<ULong>();
//  //  ULong sum3 = res3.get().cast_<ULong>();
//
//  //  //master-slave 线程模型
//  //  //master 线程用来分解任务，然后给各个Slave线程分配任务
//  //  //等待各个线程Slave线程执行任务，返回结果
//  //  //Master线程合并各个任务结果，输出
//  //  std::cout << (sum1 + sum2 + sum3) << std::endl;
//  //}
//  return 0;
//}
//
//
