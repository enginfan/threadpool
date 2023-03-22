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
      //std::this_thread::sleep_for(std::chrono::seconds(3));
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

        

int main()
{
    ThreadPool pool;
    pool.setMode(PoolMode::MODE_CACHED);
    pool.start(4);
    Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
    Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
    Result res3 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
    Result res4 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
    Result res5 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
    Result res6 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
    Result res7 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
    ULong sum1 = res1.get().cast_<ULong>();
    ULong sum2 = res2.get().cast_<ULong>();
    ULong sum3 = res3.get().cast_<ULong>();

    //master-slave �߳�ģ��
    //master �߳������ֽ�����Ȼ�������Slave�̷߳�������
    //�ȴ������߳�Slave�߳�ִ�����񣬷��ؽ��
    //Master�̺߳ϲ����������������
    std::cout << (sum1 + sum2 + sum3) << std::endl;
    getchar();
    return 0;
}

// ���г���: Ctrl + F5 ����� >����ʼִ��(������)���˵�
// ���Գ���: F5 ����� >����ʼ���ԡ��˵�

// ����ʹ�ü���: 
//   1. ʹ�ý��������Դ�������������/�����ļ�
//   2. ʹ���Ŷ���Դ�������������ӵ�Դ�������
//   3. ʹ��������ڲ鿴���������������Ϣ
//   4. ʹ�ô����б��ڲ鿴����
//   5. ת������Ŀ��>���������Դ����µĴ����ļ�����ת������Ŀ��>�����������Խ����д����ļ���ӵ���Ŀ
//   6. ��������Ҫ�ٴδ򿪴���Ŀ����ת�����ļ���>���򿪡�>����Ŀ����ѡ�� .sln �ļ�
