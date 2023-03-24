
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <thread>
#include <future>
#include <iostream>
const int TASK_MAX_THREASHHOLD = INT32_MAX;
const int THREAD_MAX_THREASHHOLD = 100;
const int THREAD_MAX_IDLE_TIME = 60;
//线程池支持的模式
enum class PoolMode {
	//固定数量的线程
	MODE_FIXED,
	//线程数量可动态增长
	MODE_CACHED,
};

//线程类型
class Thread {
public:
	//线程函数对象类型
	using ThreadFunc = std::function<void(int)>;
	//线程构造
	Thread(ThreadFunc func)
		:func_(func)
		, threadId_(generatedId_++)
	{

	}
	//线程析构
	~Thread() =default;
	//启动线程
	void start()
	{
		//创建一个线程来执行一个线程函数
		std::thread t(func_, threadId_);//对于C++11来说线程分为 线程对象t和线程函数func_
		t.detach();//设置分离线程 pthread_detach 
	}

	//获取线程id 写到CPP中编译为动态库auto lastTime = std::chrono::high_resolution_clock().now();就不可见
	int getId()const
	{
		return threadId_;
	}
private:
	ThreadFunc func_;
	static int generatedId_;
	int threadId_;
};
 int Thread::generatedId_ = 0;

//线程池类型
class ThreadPool {
public:
	ThreadPool()
		:initThreadSize_(0)
		, taskSize_(0)
		, idleThreadSize_(0)
		, curThreadSize_(0)
		, taskQusMaxThreshHold_(TASK_MAX_THREASHHOLD)
		, threadSizeThreshHold_(THREAD_MAX_THREASHHOLD)
		, poolMode_(PoolMode::MODE_FIXED)
		, isPoolRunning_(false)
	{

	}
	~ThreadPool()
	{
		isPoolRunning_ = false;
		//等待线程池里面所有的线程返回 有两种状态：阻塞||正在执行任务
		std::unique_lock<std::mutex> lock(taskQueMtx_);
		notEmpty_.notify_all();
		exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });
	}
	//设置初始的线程数量
	void setInitThreadSize(int size)
	{
		if (checkRunningState())
			return;
		initThreadSize_ = size;
	}
	//设置线程池的工作模式
	void setMode(PoolMode mode)
	{
		//不允许在启动以后再设置
		if (checkRunningState())
			return;
		poolMode_ = mode;
	}
	//设置task任务队列上限阈值
	void setTaskQueMaxThreshHold(int threshhold)
	{
		if (checkRunningState())
			return;
		taskQusMaxThreshHold_ = threshhold;
	}
	//设置线程池cached模式下线程阈值
	void setThreadThreshHold(int threshhold)
	{
		if (checkRunningState())
			return;
		if (PoolMode::MODE_CACHED == poolMode_)
		{
			threadSizeThreshHold_ = threshhold;
		}
	}
	//给线程池提交任务 csdn大秦坑王 右值引用和引用折叠原理
	template<typename Func,typename... Args>
	auto submitTask(Func&& func, Args&&... args)->std::future<decltype(func(args...))>
	{
		using RType = decltype(func(args...));
		auto task = std::make_shared<std::packaged_task<RType()>>(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
		std::future<RType> result = task->get_future();
		//获取锁 队列本身并不是线程安全的
		std::unique_lock<std::mutex> lock(taskQueMtx_);
		//用户提交任务，最长不能阻塞超过1s，否则判断提交任务失败，返回
		if (!notFull_.wait_for(lock, std::chrono::seconds(1),
			[&]()->bool {return taskQue_.size() < (size_t)taskQusMaxThreshHold_; }))
		{
			std::cerr << "task queue is full ,submit task fail." << std::endl;
			auto task = std::make_shared<std::packaged_task<RType()>>(
				[]()->RType {return RType(); });
			(*task)();
			//return task->getResult() 这种方法不可取，线程执行完task，task对象就被析构掉了，那么result也会析构
			return task->get_future();
		}
		//如果有空余，把任务放入任务队列中
		//taskQue_.emplace(sp);
		//执行任务
		taskQue_.emplace([task]() {(*task)();});
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
			std::cout << ">>> create new thread...." << std::endl;
		}
		//返回任务的result对象
		return result;


	}
	
	
	//开启线程池::等于CPU核心数
	void start(int initThreadSize= std::thread::hardware_concurrency())
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

	//禁止进行拷贝构造
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	//定义线程函数
	void threadFunc(int threadId)
	{
		auto lastTime = std::chrono::high_resolution_clock().now();
		/*std::cout << "begin threadfunc:" << std::this_thread::get_id()
			<< std::endl;
		std::cout << "end threadfunc:" << std::this_thread::get_id()
			<< std::endl;*/
		for (;;)
		{

			Task task;
			{
				//先获取锁
				std::unique_lock<std::mutex> lock(taskQueMtx_);
				std::cout << "tid:" << std::this_thread::get_id()
					<< "尝试获取任务..." << std::endl;
				//在cached的模式下，有可能已经创建了很多线程，但是空闲时间超过60s
				//应把多余的线程回收掉
				//每一秒钟返回一次
				while (taskQue_.size() == 0)
				{
					if (!isPoolRunning_)
					{
						std::cout << "threadid:" << std::this_thread::get_id() << "exit!"
							<< std::endl;
						threads_.erase(threadId);
						exitCond_.notify_all();
						return;
					}
					if (PoolMode::MODE_CACHED == poolMode_)
					{
						if (std::cv_status::timeout ==
							notEmpty_.wait_for(lock, std::chrono::seconds(1)))
						{
							//超时返回
							auto now = std::chrono::high_resolution_clock().now();
							auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
							if (dur.count() >= THREAD_MAX_IDLE_TIME
								&& curThreadSize_ > initThreadSize_)
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
					else
					{
						//等待notEmpty条件
						notEmpty_.wait(lock);
					}
				}
					//从任务队列中取一个任务出来
					task = taskQue_.front();
					taskQue_.pop();
					taskSize_--;
					idleThreadSize_--;
					std::cout << "tid:" << std::this_thread::get_id()
						<< "获取任务成功..." << std::endl;
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
					task();
					//执行完成
				}
				//更新线程执行完任务的时间
				lastTime = std::chrono::high_resolution_clock().now();
				idleThreadSize_++;
		}
	}
	//检查pool的运行状态
	bool checkRunningState()const
	{
		return isPoolRunning_;
	}
private:
	//线程列表 因为vector本身不是线程安全的所以不适合使用
	//vec容器本身的size来表示线程池线程的数量
	//std::vector<std::unique_ptr<Thread>> threads_;
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;

	//初始的线程数量
	int initThreadSize_;
	//当前线程池里的线程的总数量
	std::atomic_int curThreadSize_;
	//线程数量上限阈值
	int threadSizeThreshHold_;
	//记录空闲线程的数量
	std::atomic_int idleThreadSize_;

	//任务队列
	using Task = std::function<void()>;
	std::queue<Task>  taskQue_;
	//任务的数量
	std::atomic_int taskSize_;
	// 任务队列数量上限阈值
	int taskQusMaxThreshHold_;

	//保证任务队列线程安全
	std::mutex taskQueMtx_;
	//表示任务队列的不满
	std::condition_variable notFull_;
	//表示任务队列的不空
	std::condition_variable notEmpty_;
	//等待线程资源全部回收
	std::condition_variable exitCond_;

	//当前线程池的工作模式
	PoolMode poolMode_;
	//当前线程池的启动状态
	std::atomic_bool isPoolRunning_;

};




#endif // !THREADPOOL_H
