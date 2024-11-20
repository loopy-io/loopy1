#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include "song.h"

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);

signals:
    void getASongFinished();    //解析结束信号

public:
    void readLyrics(Song *);

public slots:
    void getASong(const QUrl & mp3Url); //解析歌曲，参数接收歌曲路径
};


/*消息队列单例类，用于存储数据（歌曲对象指针），共享给多个线程访问读写
 * todo: 抽象一个消息类Message，内部封装消息类型、具体的消息体等属性
 *  内部一个队列和互斥量
 *  入队一个歌曲指针的接口
 *  出队一个歌曲指针的接口
 *  判断是否为空的接口
 *      取代手动调用加锁lock和解锁unlock，使用自动加解锁的类对象QMutexLocker
 *      传入一个QMutex指针构造一个局部QMutexLocker对象，构造时自动加锁，析构（函数返回）时自动解锁
 *      这种以局部对象自动管理资源的方式称为RAII(Resource Acquisition Is Initialization) 资源获取即初始化
 *  查询数据个数接口size
*/
class MessageQueue
{
private:
    MessageQueue() {}
    MessageQueue(const MessageQueue & other) {}
    ~MessageQueue() { /*todo: 清空消息队列中的数据*/ }

public:
    static MessageQueue & getInstance();

private:
    QQueue<Song*> m_queue;  //存储数据的队列
    QMutex m_mutex;         //互斥量，保护外部多线程访问共享数据m_queue
    //互斥量本身由系统进行访问保护（同一时刻只会有一个线程获取到互斥量（加锁成功））

public:
    void push(Song * song); //入队接口
    Song * pop();           //出队接口
    bool empty();           //判断是否为空
    int size();             //获取大小
};

#endif // WORKER_H
