#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
//����������

class Task {
public:
	//�û������Զ����������񣬴�Task�̳У���дrun������ʵ���Զ���������
	virtual void run() = 0;
};

//�̳߳�֧�ֵ�ģʽ
enum class PoolMode {
	MODE_FIXED,//�̶��������߳�
	MODE_CACHED,//�߳������ɶ�̬����
};

//�߳�����
class Thread {
public:
private:
};

//�̳߳�����
class ThreadPool {
public:
	ThreadPool();
	~ThreadPool();

	void setMode(PoolMode mode);

	void start();//�����̳߳�
private:
	std::vector<Thread*> threads_;//�߳��б�
	size_t initThreadSize_;//��ʼ���߳�����

	std::queue<std::shared_ptr<Task>>  taskQue_;//�������
	std::atomic_uint taskSize_;//���������
	int taskQusMaxThreshHold_; // �����������������ֵ
	
	std::mutex taskQueMtx_;//��֤��������̰߳�ȫ
	std::condition_variable notFull_;//��ʾ������еĲ���
	std::condition_variable notEmpty_;//��ʾ������еĲ���

	PoolMode poolMode_;//��ǰ�̳߳صĹ���ģʽ
};




#endif // !THREADPOOL_H

