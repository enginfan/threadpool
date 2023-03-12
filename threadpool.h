#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
//����������

class Task {
public:
	//�û������Զ����������񣬴�Task�̳У���дrun������ʵ���Զ���������
	virtual void run() = 0;
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
	using ThreadFunc = std::function<void>;
	//�̹߳���
	//�߳�����
	//�����߳�
	void start();
private:
	ThreadFunc func_;
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
	//���̳߳��ύ����
	void submitTask(std::shared_ptr<Task> sp);
	//�����̳߳�
	void start(int initThreadSize=4);
	
	//��ֹ���п�������
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	//�����̺߳���
	void threadFunc();
private:
	//�߳��б�
	std::vector<Thread*> threads_;
	//��ʼ���߳�����
	size_t initThreadSize_;
	//�������
	std::queue<std::shared_ptr<Task>>  taskQue_;
	//���������
	std::atomic_uint taskSize_;
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
};




#endif // !THREADPOOL_H

