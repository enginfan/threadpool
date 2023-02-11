#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
//任务抽象基类

class Task {
public:
	//用户可以自定义任意任务，从Task继承，重写run方法，实现自定义任务处理
	virtual void run() = 0;
};

//线程池支持的模式
enum class PoolMode {
	MODE_FIXED,//固定数量的线程
	MODE_CACHED,//线程数量可动态增长
};

//线程类型
class Thread {
public:
private:
};

//线程池类型
class ThreadPool {
public:
	ThreadPool();
	~ThreadPool();

	void setMode(PoolMode mode);

	void start();//开启线程池
private:
	std::vector<Thread*> threads_;//线程列表
	size_t initThreadSize_;//初始的线程数量

	std::queue<std::shared_ptr<Task>>  taskQue_;//任务队列
	std::atomic_uint taskSize_;//任务的数量
	int taskQusMaxThreshHold_; // 任务队列数量上限阈值
	
	std::mutex taskQueMtx_;//保证任务队列线程安全
	std::condition_variable notFull_;//表示任务队列的不满
	std::condition_variable notEmpty_;//表示任务队列的不空

	PoolMode poolMode_;//当前线程池的工作模式
};




#endif // !THREADPOOL_H

