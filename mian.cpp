#include "threadpool.h"
#include <iostream>
#include <chrono>



class MyTask :public Task
{
public:
    Any run() 
    {
        std::cout << "tid:" << std::this_thread::get_id()
            <<"begin!" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "tid:" << std::this_thread::get_id()
            <<"end!" << std::endl;
    }
private:
  
};

        

int main()
{
    ThreadPool pool;
    pool.start(4);
    
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
