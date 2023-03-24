
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
//�̳߳�֧�ֵ�ģʽ
enum class PoolMode {
	//�̶��������߳�
	MODE_FIXED,
	//�߳������ɶ�̬����
	MODE_CACHED,
};

//�߳�����
class Thread {
public:
	//�̺߳�����������
	using ThreadFunc = std::function<void(int)>;
	//�̹߳���
	Thread(ThreadFunc func)
		:func_(func)
		, threadId_(generatedId_++)
	{

	}
	//�߳�����
	~Thread() =default;
	//�����߳�
	void start()
	{
		//����һ���߳���ִ��һ���̺߳���
		std::thread t(func_, threadId_);//����C++11��˵�̷߳�Ϊ �̶߳���t���̺߳���func_
		t.detach();//���÷����߳� pthread_detach 
	}

	//��ȡ�߳�id д��CPP�б���Ϊ��̬��auto lastTime = std::chrono::high_resolution_clock().now();�Ͳ��ɼ�
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

//�̳߳�����
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
		//�ȴ��̳߳��������е��̷߳��� ������״̬������||����ִ������
		std::unique_lock<std::mutex> lock(taskQueMtx_);
		notEmpty_.notify_all();
		exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });
	}
	//���ó�ʼ���߳�����
	void setInitThreadSize(int size)
	{
		if (checkRunningState())
			return;
		initThreadSize_ = size;
	}
	//�����̳߳صĹ���ģʽ
	void setMode(PoolMode mode)
	{
		//�������������Ժ�������
		if (checkRunningState())
			return;
		poolMode_ = mode;
	}
	//����task�������������ֵ
	void setTaskQueMaxThreshHold(int threshhold)
	{
		if (checkRunningState())
			return;
		taskQusMaxThreshHold_ = threshhold;
	}
	//�����̳߳�cachedģʽ���߳���ֵ
	void setThreadThreshHold(int threshhold)
	{
		if (checkRunningState())
			return;
		if (PoolMode::MODE_CACHED == poolMode_)
		{
			threadSizeThreshHold_ = threshhold;
		}
	}
	//���̳߳��ύ���� csdn���ؿ��� ��ֵ���ú������۵�ԭ��
	template<typename Func,typename... Args>
	auto submitTask(Func&& func, Args&&... args)->std::future<decltype(func(args...))>
	{
		using RType = decltype(func(args...));
		auto task = std::make_shared<std::packaged_task<RType()>>(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
		std::future<RType> result = task->get_future();
		//��ȡ�� ���б��������̰߳�ȫ��
		std::unique_lock<std::mutex> lock(taskQueMtx_);
		//�û��ύ�����������������1s�������ж��ύ����ʧ�ܣ�����
		if (!notFull_.wait_for(lock, std::chrono::seconds(1),
			[&]()->bool {return taskQue_.size() < (size_t)taskQusMaxThreshHold_; }))
		{
			std::cerr << "task queue is full ,submit task fail." << std::endl;
			auto task = std::make_shared<std::packaged_task<RType()>>(
				[]()->RType {return RType(); });
			(*task)();
			//return task->getResult() ���ַ�������ȡ���߳�ִ����task��task����ͱ��������ˣ���ôresultҲ������
			return task->get_future();
		}
		//����п��࣬������������������
		//taskQue_.emplace(sp);
		//ִ������
		taskQue_.emplace([task]() {(*task)();});
		taskSize_++; 
		//��Ϊ�·�������������п϶������ˣ���notEmpty_�Ͻ���֪ͨ,�Ͽ�����߳�ִ������
		notEmpty_.notify_all();

		//cachedģʽ ������ȽϽ��� ������С���ҿ������ ��Ҫ��������������Ϳ����̵߳��������ж��Ƿ���Ҫ�����µ��̳߳���
		if (PoolMode::MODE_CACHED == poolMode_
			&& taskSize_ > idleThreadSize_
			&& curThreadSize_ < threadSizeThreshHold_)
		{
			//�������߳�
			auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
			int threadId = ptr->getId();
			threads_.emplace(threadId, std::move(ptr));
			//��Ҫ���������߳�
			threads_[threadId]->start();
			//��ر���
			curThreadSize_++;
			idleThreadSize_++;
			std::cout << ">>> create new thread...." << std::endl;
		}
		//���������result����
		return result;


	}
	
	
	//�����̳߳�::����CPU������
	void start(int initThreadSize= std::thread::hardware_concurrency())
	{
		//�����̳߳ص�����״̬
		isPoolRunning_ = true;
		//��¼��ʼ�̸߳���
		initThreadSize_ = initThreadSize;
		curThreadSize_ = initThreadSize;

		//�����̶߳���
		for (int i = 0; i < initThreadSize_; i++)
		{
			//����thread�̶߳����ʱ�򣬰��̺߳�������thread�̶߳���
			auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
			//threads_.emplace_back(std::move(ptr));
			int threadId = ptr->getId();
			threads_.emplace(threadId, std::move(ptr));
		}

		//���������߳�
		for (int i = 0; i < initThreadSize_; i++)
		{
			threads_[i]->start();
			//��¼��ʼ�����̵߳����� ��Ϊ�տ�ʼ����ȫ��Ӧ���ǿ��е�
			idleThreadSize_++;
		}
	}

	//��ֹ���п�������
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	//�����̺߳���
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
				//�Ȼ�ȡ��
				std::unique_lock<std::mutex> lock(taskQueMtx_);
				std::cout << "tid:" << std::this_thread::get_id()
					<< "���Ի�ȡ����..." << std::endl;
				//��cached��ģʽ�£��п����Ѿ������˺ܶ��̣߳����ǿ���ʱ�䳬��60s
				//Ӧ�Ѷ�����̻߳��յ�
				//ÿһ���ӷ���һ��
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
							//��ʱ����
							auto now = std::chrono::high_resolution_clock().now();
							auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
							if (dur.count() >= THREAD_MAX_IDLE_TIME
								&& curThreadSize_ > initThreadSize_)
							{
								//��ʼ���յ�ǰ�߳�
								//���̶߳�����߳��б�������ɾ��
								threads_.erase(threadId);
								//��¼�߳���������ر�����ֵ�޸�
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
						//�ȴ�notEmpty����
						notEmpty_.wait(lock);
					}
				}
					//�����������ȡһ���������
					task = taskQue_.front();
					taskQue_.pop();
					taskSize_--;
					idleThreadSize_--;
					std::cout << "tid:" << std::this_thread::get_id()
						<< "��ȡ����ɹ�..." << std::endl;
					// �����Ȼ��ʣ������
					if (taskQue_.size() > 0)
					{
						notEmpty_.notify_all();
					}
					//ȡ��һ���������֪ͨ ���Լ����ύ��������
					notFull_.notify_all();
			}//�������������Զ��ͷ�
				//��ǰ�̸߳���ִ���������
				if (task != nullptr) {
					//task->run();
					task();
					//ִ�����
				}
				//�����߳�ִ���������ʱ��
				lastTime = std::chrono::high_resolution_clock().now();
				idleThreadSize_++;
		}
	}
	//���pool������״̬
	bool checkRunningState()const
	{
		return isPoolRunning_;
	}
private:
	//�߳��б� ��Ϊvector�������̰߳�ȫ�����Բ��ʺ�ʹ��
	//vec���������size����ʾ�̳߳��̵߳�����
	//std::vector<std::unique_ptr<Thread>> threads_;
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;

	//��ʼ���߳�����
	int initThreadSize_;
	//��ǰ�̳߳�����̵߳�������
	std::atomic_int curThreadSize_;
	//�߳�����������ֵ
	int threadSizeThreshHold_;
	//��¼�����̵߳�����
	std::atomic_int idleThreadSize_;

	//�������
	using Task = std::function<void()>;
	std::queue<Task>  taskQue_;
	//���������
	std::atomic_int taskSize_;
	// �����������������ֵ
	int taskQusMaxThreshHold_;

	//��֤��������̰߳�ȫ
	std::mutex taskQueMtx_;
	//��ʾ������еĲ���
	std::condition_variable notFull_;
	//��ʾ������еĲ���
	std::condition_variable notEmpty_;
	//�ȴ��߳���Դȫ������
	std::condition_variable exitCond_;

	//��ǰ�̳߳صĹ���ģʽ
	PoolMode poolMode_;
	//��ǰ�̳߳ص�����״̬
	std::atomic_bool isPoolRunning_;

};




#endif // !THREADPOOL_H
