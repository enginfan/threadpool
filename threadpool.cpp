#include "threadpool.h"
#include <functional>
#include <thread>
#include <iostream>
const int TASK_MAX_THREASHHOLD =INT32_MAX;
const int THREAD_MAX_THREASHHOLD = 100;
const int THREAD_MAX_IDLE_TIME = 60;

///////////////////////////////////////////////////////////线程池方法
ThreadPool::ThreadPool()
	:initThreadSize_(4)
	, taskSize_(0)
	, idleThreadSize_(0)
	, curThreadSize_(0)
	, taskQusMaxThreshHold_(TASK_MAX_THREASHHOLD)
	, threadSizeThreshHold_(THREAD_MAX_THREASHHOLD)
	, poolMode_(PoolMode::MODE_FIXED)
	, isPoolRunning_(false)
{

}
//线程池析构
ThreadPool::~ThreadPool()
{

}
//设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode)
{
	//不允许在启动以后再设置
	if (checkRunningState())
		return;
	poolMode_ = mode;
}
//设置初始的线程数量
void ThreadPool::setInitThreadSize(int size)
{
	if (checkRunningState())
		return;
	initThreadSize_ = size;
}
//设置task任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	if (checkRunningState())
		return;
	taskQusMaxThreshHold_ = threshhold;
}
void ThreadPool::setThreadThreshHold(int threshhold)
{
	if (checkRunningState())
		return;
	if (PoolMode::MODE_CACHED == poolMode_)
	{
		threadSizeThreshHold_ = threshhold;
	}
}
//给线程池提交任务 用户调用该接口 传入任务对象 生产任务
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	//获取锁 队列本身并不是线程安全的
	std::unique_lock<std::mutex> lock(taskQueMtx_);
	//线程通信 等待任务队列有空余 
	// wait与时间无关 直到条件满足
	// wait_for 等到条件满足但有时间限制（持续等待时间）
	// wait_until 等到终止时间
	//用户提交任务，最长不能阻塞超过1s，否则判断提交任务失败，返回
	if (!notFull_.wait_for(lock, std::chrono::seconds(2),
		[&]()->bool {return taskQue_.size() < (size_t)taskQusMaxThreshHold_; }))
	{
		std::cerr << "task queue is full ,submit task fail." << std::endl;
		//return task->getResult() 这种方法不可取，线程执行完task，task对象就被析构掉了，那么result也会析构
		return Result(sp,false);
	}
	//如果有空余，把任务放入任务队列中
	taskQue_.emplace(sp);
	taskSize_++;
	//因为新放了任务，任务队列肯定不空了，在notEmpty_上进行通知,赶快分配线程执行任务
	notEmpty_.notify_all();

	//cached模式 任务处理比较紧急 场景：小而且快的任务 需要根据任务的数量和空闲线程的数量，判断是否需要创建新的线程出来
	if (PoolMode::MODE_CACHED == poolMode_
		&& taskSize_ > idleThreadSize_
		&& curThreadSize_ < threadSizeThreshHold_)
	{
		//创建新线程
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		//不要忘记启动线程
		threads_[threadId]->start();
		//相关变量
		curThreadSize_++;
		idleThreadSize_++;
		std::cout << ">>> create new thread...." <<std::endl;
	}
	//返回任务的result对象
	return Result(sp);
}

//开启线程池
void ThreadPool::start(int initThreadSize)
{
	//设置线程池的运行状态
	isPoolRunning_ = true;
	//记录初始线程个数
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	//创建线程对象
	for (int i = 0; i < initThreadSize_; i++)
	{
		//创建thread线程对象的时候，把线程函数给到thread线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		//threads_.emplace_back(std::move(ptr));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
	}

	//启动所有线程
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();
		//记录初始空闲线程的数量 因为刚开始启动全都应该是空闲的
		idleThreadSize_++;
	}
}

//定义线程函数
void ThreadPool::threadFunc(int threadId)
{
	/*std::cout << "begin threadfunc:" << std::this_thread::get_id()
		<< std::endl;
	std::cout << "end threadfunc:" << std::this_thread::get_id()
		<< std::endl;*/
	for (;;)
	{
		auto lastTime = std::chrono::high_resolution_clock().now();

		std::shared_ptr<Task> task;
		{
			//先获取锁
			std::unique_lock<std::mutex> lock(taskQueMtx_);
			std::cout << "tid:" << std::this_thread::get_id()
				<< "尝试获取任务..." << std::endl;

			//在cached的模式下，有可能已经创建了很多线程，但是空闲时间超过60s
			//应把多余的线程回收掉
			if (PoolMode::MODE_CACHED == poolMode_)
			{
				//每一秒钟返回一次
				while (taskQue_.size() == 0)
				{
					if (std::cv_status::timeout ==
						notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						//超时返回
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						if (dur.count() >= THREAD_MAX_IDLE_TIME
							&&curThreadSize_>initThreadSize_)
						{
							//开始回收当前线程
							//把线程对象从线程列表容器中删除
							threads_.erase(threadId);
							//记录线程数量的相关变量的值修改
							curThreadSize_--;
							idleThreadSize_--;
							std::cout << "threadid:" << std::this_thread::get_id() << "exit!"
								<< std::endl;
							return;
						}
					}
				}
			}
			else
			{
				//等待notEmpty条件
				notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });
			}
			idleThreadSize_--;

			std::cout << "tid:" << std::this_thread::get_id()
				<< "获取任务成功..." << std::endl;
			//从任务队列中取一个任务出来
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;
			// 如果依然有剩余任务
			if (taskQue_.size() > 0)
			{
				notEmpty_.notify_all();
			}
			//取出一个任务进行通知 可以继续提交生产任务
			notFull_.notify_all();
		}//出了作用域锁自动释放
		//当前线程负责执行这个任务
		if (task != nullptr) {
			//task->run();
			task->exec();
			//执行完成
		}
		//更新线程执行完任务的时间
		lastTime = std::chrono::high_resolution_clock().now();
		idleThreadSize_++;
	}
}

bool ThreadPool::checkRunningState()const
{
	return isPoolRunning_;
}

/////////////////////////////////////////////////////////线程Thread方法实现
int Thread::generatedId_ = 0;

//线程构造
Thread::Thread(ThreadFunc func)
	:func_(func) 
	, threadId_(generatedId_++)
{

}
//线程析构
Thread::~Thread() {

}
//启动线程
void Thread::start()
{
	//创建一个线程来执行一个线程函数
	std::thread t(func_,threadId_);//对于C++11来说线程分为 线程对象t和线程函数func_
	t.detach();//设置分离线程 pthread_detach 


}
int Thread::getId()const
{
	return threadId_;
}

///////////////////////////////////////////////////////// Task方法实现
Task::Task()
	: result_(nullptr)
{}
void Task::exec()
{
	if (result_ != nullptr) 
	{
		//加一层封装去调用run方法 
		result_->setVal(run());
	}
	
}

void Task::setResult(Result *res)
{
	result_ = res;
}

//////////////////////////////////////////////////////////Result方法实现
//isvlid不能加=true会报错
Result::Result(std::shared_ptr<Task> task, bool isValid) 
	: task_(task)
	, isValid_(isValid)
{
	task_->setResult(this);
}

Any Result::get()
{
	if (!isValid_)
	{
		return "";
	}

	sem_.wait();//如果任务没有执行完，这里会阻塞线程
	return std::move(any_);
}

void Result::setVal(Any any)
{
	//存储task的返回值
	this->any_ = std::move(any);
	//已经获取了任务的返回值，增加信号量资源
	sem_.post();
}


