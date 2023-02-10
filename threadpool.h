#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
//����������

class Task {
public:
	//�û������Զ����������񣬴�Task�̳У���дrun������ʵ���Զ���������
	virtual void run() = 0;
};

//�̳߳�֧�ֵ�ģʽ
enum PoolMode {
	MODE_FIXED,//�̶��������߳�
	MODE_CACHED,//�߳������ɶ�̬����
};

//�߳�����
class Thread {
public:
private:
};

//�̳߳�����
class TreadPool {
public:
private:
	std::vector<Thread*> threads_;//�߳��б�
	size_t initThreadSize_;//��ʼ���߳�����

	std::queue<std::shared_ptr<Task>>  taskQue_;//�������
	std::atomic_uint taskSize_;//���������
	int taskQusMaxThreshHold_; // �����������������ֵ
	

};




#endif // !THREADPOOL_H

