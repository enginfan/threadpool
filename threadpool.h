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
		void run(){//�̴߳���...}
}��

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

	//��ȡһ���ź�����Դ
	void wait()
	{
		std::unique_lock<std::mutex> lock(mtx_);
		cond_.wait(lock, [&]()->bool {return resLimit_ > 0; });
		resLimit_--;
	}
	//����һ���ź�����Դ
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


//Any���ͣ����Խ���������������(ģ��C++17���е�Any)
//ģ����ĺ���ֻ��д��ͷ�ļ�����
class Any
{
public:
	Any()= default;
	~Any() = default;
	//��Ϊ����unique_ptr��Ա�����䱾��Ҳ�����Խ��п�������͸�ֵ�����Խ����ƶ�����
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	//������캯��������Any�����������͵�����
	template<typename T>
	Any(T data) :base_(std::make_unique<Derive<T>>(data)) {};
	//��������ܰ�Any��������洢��data������ȡ����
	template<typename T>
	T cast_()
	{
		//������ô��base_�ҵ�����ָ���Derive���󣬴�������ȡ��data��Ա����
		//����ָ��-��������ָ�� RTTI��Run-Time Type Identification)��ͨ
		//������ʱ������Ϣ�����ܹ�ʹ��
		//�����ָ��������������Щָ���������ָ�Ķ����ʵ���������͡�
		//�˴�get�ǻ�ȡԭ��ָ�룬��Ϊ�������Ǹ���ָ��ָ�����������ת���ǰ�ȫ��
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
		virtual ~Base() = default;//���ӱ�������ָ���Ż�
	};
	//����������
	template<typename T>
	class Derive :public Base
	{
	public:
		Derive(T data):data_(data){}
		T data_;//�������������������
	};
private:
	//����һ������ָ��
	std::unique_ptr<Base> base_;//��������͸�ֵ����delete��ֻ����ֵ����
};


//Taskǰ������
class Task;

//ʵ�ֽ����ύ���̳߳ص�task����ִ����ɺ�ķ���ֵ����Result
class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;

	//��ȡ����ִ����ķ���ֵ
	void setVal(Any any);
	//��ȡtask�ķ���ֵ
	Any get();
private:
	//�洢����ķ���ֵ
	Any any_;
	//�߳�ͨ��
	Semaphore sem_;
	//ָ������ȡ����ֵ���������
	std::shared_ptr<Task> task_;
	//����ֵ�Ƿ���Ч
	std::atomic_bool isValid_;
};

//����������
class Task {
public:
	Task();
	~Task() = default;
	void exec();
	void setResult(Result* res);
	//�û������Զ����������񣬴�Task�̳У���дrun������ʵ���Զ���������
	virtual Any run() = 0;
private:
	//ע��Ҫʹ����ָ�룬�������������ָ��Ľ��������������������Ҳ��Զ�ò����ͷ�
	//Res�����������ǳ���task��
	Result* result_;
};

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
	Thread(ThreadFunc func);
	//�߳�����
	~Thread();
	//�����߳�
	void start();

	//��ȡ�߳�id д��CPP�б���Ϊ��̬��Ͳ��ɼ�
	int getId()const;
private:
	ThreadFunc func_;
	static int generatedId_;
	int threadId_;
};

//�̳߳�����
class ThreadPool {
public:
	ThreadPool();
	~ThreadPool();
	//���ó�ʼ���߳�����
	void setInitThreadSize(int size);
	//�����̳߳صĹ���ģʽ
	void setMode(PoolMode mode);
	//����task�������������ֵ
	void setTaskQueMaxThreshHold(int threshhold);
	//�����̳߳�cachedģʽ���߳���ֵ
	void setThreadThreshHold(int threshhold);
	//���̳߳��ύ����
	Result submitTask(std::shared_ptr<Task> sp);
	//�����̳߳�
	void start(int initThreadSize = 4);

	//��ֹ���п�������
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	//�����̺߳���
	void threadFunc(int threadId);
	//���pool������״̬
	bool checkRunningState() const;
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
	std::queue<std::shared_ptr<Task>>  taskQue_;
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

	//��ǰ�̳߳صĹ���ģʽ
	PoolMode poolMode_;
	//��ǰ�̳߳ص�����״̬
	std::atomic_bool isPoolRunning_;
	
};




#endif // !THREADPOOL_H

