#include<iostream>
#include<functional>
#include<thread>
#include<future>
#include"newThreadPool.h"
/*
采用新语言标准对线程池进行优化
1.pool.submitTask(sum,10,20);
	submitTask:可变参数模板编程
2.旧版本采用Result以及相关类型，代码量较大
	C++11 线程库 thread packaged_task(function函数对象) async
	使用future来代替Result节省线程池代码
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


