#include<iostream>
#include<functional>
#include<thread>
#include<future>
#include"newThreadPool.h"
/*
���������Ա�׼���̳߳ؽ����Ż�
1.pool.submitTask(sum,10,20);
	submitTask:�ɱ����ģ����
2.�ɰ汾����Result�Լ�������ͣ��������ϴ�
	C++11 �߳̿� thread packaged_task(function��������) async
	ʹ��future������Result��ʡ�̳߳ش���
*/
using namespace std;
int sum1(int a, int b)
{
	return a + b;
}
int sum2(int a, int b, int c)
{
	return a + b + c;
}
int main()
{
	ThreadPool pool;
	pool.start(4);
	future<int> r=pool.submitTask(sum1, 1, 2);
	cout << r.get() << endl;

}


