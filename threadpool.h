#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
//任务抽象基类

class Task {
public:
	//用户可以自定义任意任务，从Task继承，重写run方法，实现自定义任务处理
	virtual void run() = 0;
}; 

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
	using ThreadFunc = std::function<void>;
	//线程构造
	//线程析构
	//启动线程
	void start();
private:
	ThreadFunc func_;
};

//线程池类型
class ThreadPool {
public:
	ThreadPool();
	~ThreadPool();
	//设置初始的线程数量
	void setInitThreadSize(int size);
	//设置线程池的工作模式
	void setMode(PoolMode mode);
	//设置task任务队列上限阈值
	void setTaskQueMaxThreshHold(int threshhold);
	//给线程池提交任务
	void submitTask(std::shared_ptr<Task> sp);
	//开启线程池
	void start(int initThreadSize=4);
	
	//禁止进行拷贝构造
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	//定义线程函数
	void threadFunc();
private:
	//线程列表
	std::vector<Thread*> threads_;
	//初始的线程数量
	size_t initThreadSize_;
	//任务队列
	std::queue<std::shared_ptr<Task>>  taskQue_;
	//任务的数量
	std::atomic_uint taskSize_;
	// 任务队列数量上限阈值
	int taskQusMaxThreshHold_;
	//保证任务队列线程安全
	std::mutex taskQueMtx_;
	//表示任务队列的不满
	std::condition_variable notFull_;
	//表示任务队列的不空
	std::condition_variable notEmpty_;
	//当前线程池的工作模式
	PoolMode poolMode_;
};




#endif // !THREADPOOL_H

