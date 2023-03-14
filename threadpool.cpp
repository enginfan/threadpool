#include "threadpool.h"
#include <functional>
#include <thread>
#include <iostream>
const int TASK_MAX_THREASHHOLD = 1024;

ThreadPool::ThreadPool() 
	:initThreadSize_(4)
	,taskSize_(0)
	,taskQusMaxThreshHold_(TASK_MAX_THREASHHOLD)
	,poolMode_(PoolMode::MODE_FIXED)
{

}
//线程池析构
ThreadPool::~ThreadPool()
{

}
//设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode)
{
	poolMode_ = mode;
}
//设置初始的线程数量
void ThreadPool::setInitThreadSize(int size)
{
	initThreadSize_ = size;
}
//设置task任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	taskQusMaxThreshHold_ = threshhold;
}
//给线程池提交任务
void ThreadPool::submitTask(std::shared_ptr<Task> sp)
{

}
//开启线程池//在声明处设置默认就行这里可以不写
void ThreadPool::start(int initThreadSize)
{
	//记录初始线程个数
	initThreadSize_ = initThreadSize;

	//创建线程对象
	for (int i = 0; i < initThreadSize_; i++) 
	{ 
		//创建thread线程对象的时候，把线程函数给到thread线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
		threads_.emplace_back(std::move(ptr));
	}

	//启动所有线程
	for (int i = 0; i < initThreadSize_; i++) 
	{
		threads_[i]->start();
	}
}

//定义线程函数

void ThreadPool::threadFunc()
{
	std::cout << "begin threadfunc:" << std::this_thread::get_id() 
		<< std::endl;
	std::cout << "end threadfunc:" << std::this_thread::get_id()
		<< std::endl;
}

/////线程方法实现

//线程构造
Thread::Thread(ThreadFunc func) 
	:func_(func)
{

}
//线程析构
Thread::~Thread() {

}
//启动线程
void Thread::start()
{
	//创建一个线程来执行一个线程函数
	std::thread t(func_);//对于C++11来说线程分为 线程对象t和线程函数func_
	t.detach();//设置分离线程 pthread_detach 


}