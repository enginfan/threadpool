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

/*
example:
ThreadPool pool;
pool.start(4);

class MyTask:public Task
{
	public:
		void run(){//线程代码...}
}；

pool.submitTask(std::make_shared<MyTask>());
*/

class Semaphore
{
public:
	Semaphore(int limit = 0)
		:resLimit_(limit)
	{

	}
	~Semaphore() = default;

	//获取一个信号量资源
	void wait()
	{
		std::unique_lock<std::mutex> lock(mtx_);
		cond_.wait(lock, [&]()->bool {return resLimit_ > 0; });
		resLimit_--;
	}
	//增加一个信号量资源
	void post()
	{
		std::unique_lock<std::mutex> lock(mtx_);
		resLimit_++;
		cond_.notify_all();
	}
private:
	int resLimit_;
	std::mutex mtx_;
	std::condition_variable cond_;


};


//Any类型：可以接收任意数据类型(模仿C++17库中的Any)
//模板类的函数只能写在头文件当中
class Any
{
public:
	Any()= default;
	~Any() = default;
	//因为含有unique_ptr成员所以其本身也不可以进行拷贝构造和赋值但可以进行移动构造
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	//这个构造函数可以让Any接受任意类型的数据
	template<typename T>
	Any(T data) :base_(std::make_unique<Derive<T>>(data)) {};
	//这个方法能把Any对象里面存储的data数据提取出来
	template<typename T>
	T cast_()
	{
		//我们怎么从base_找到它所指向的Derive对象，从它里面取出data成员变量
		//基类指针-》派生类指针 RTTI（Run-Time Type Identification)，通
		//过运行时类型信息程序能够使用
		//基类的指针或引用来检查这些指针或引用所指的对象的实际派生类型。
		//此处get是获取原生指针，因为本来就是父类指针指向的子类所以转换是安全的
		Derive<T>* pd = dynamic_cast<Derive<T>*> (base_.get());
		if (pd == nullptr)
		{
			throw "type is unmatch";
		}
		return pd->data_;
	}


private:
	class Base
	{
	public:
		virtual ~Base() = default;//增加编译器的指令优化
	};
	//派生类类型
	template<typename T>
	class Derive :public Base
	{
	public:
		Derive(T data):data_(data){}
		T data_;//保存了任意的其他类型
	};
private:
	//定义一个基类指针
	std::unique_ptr<Base> base_;//拷贝构造和赋值都被delete，只有右值构造
};


//Task前置声明
class Task;

//实现接收提交到线程池的task任务执行完成后的返回值类型Result
class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;

	//获取任务执行完的返回值
	void setVal(Any any);
	//获取task的返回值
	Any get();
private:
	//存储任务的返回值
	Any any_;
	//线程通信
	Semaphore sem_;
	//指向将来获取返回值的任务对象
	std::shared_ptr<Task> task_;
	//返回值是否有效
	std::atomic_bool isValid_;
};

//任务抽象基类
class Task {
public:
	Task();
	~Task() = default;
	void exec();
	void setResult(Result* res);
	//用户可以自定义任意任务，从Task继承，重写run方法，实现自定义任务处理
	virtual Any run() = 0;
private:
	//注意要使用裸指针，否则会引起智能指针的交叉引用问题出了作用域也永远得不到释放
	//Res的生命周期是长于task的
	Result* result_;
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
	//线程函数对象类型
	using ThreadFunc = std::function<void(int)>;
	//线程构造
	Thread(ThreadFunc func);
	//线程析构
	~Thread();
	//启动线程
	void start();

	//获取线程id 写到CPP中编译为动态库就不可见
	int getId()const;
private:
	ThreadFunc func_;
	static int generatedId_;
	int threadId_;
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
	//设置线程池cached模式下线程阈值
	void setThreadThreshHold(int threshhold);
	//给线程池提交任务
	Result submitTask(std::shared_ptr<Task> sp);
	//开启线程池
	void start(int initThreadSize = 4);

	//禁止进行拷贝构造
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	//定义线程函数
	void threadFunc(int threadId);
	//检查pool的运行状态
	bool checkRunningState() const;
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
	std::queue<std::shared_ptr<Task>>  taskQue_;
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

	//当前线程池的工作模式
	PoolMode poolMode_;
	//当前线程池的启动状态
	std::atomic_bool isPoolRunning_;
	
};




#endif // !THREADPOOL_H

