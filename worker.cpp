#include "worker.h"
#include <QDebug>
#include <QFileInfo>

Worker::Worker(QObject *parent) : QObject(parent)
{

}

MessageQueue & MessageQueue::getInstance()
{
    static MessageQueue instance;
    return instance;
}

//入队接口
void MessageQueue::push(Song * song)
{
    if (!song) { return; }

    m_mutex.lock();             //加锁
    m_queue.push_back(song);    //存储
    m_mutex.unlock();           //解锁
}

/*
 * 出队一个歌曲指针接口，注意队列为空的情况
 *  第一次判断如果为空直接返回（避免没有数据时加解锁的开销）
 *  加锁
 *      第二次判断不为空再将队头元素取出（防止加锁过程中正好被别的线程取完数据成为空队列）
 *  解锁
 *  返回歌曲对象指针
 */
Song * MessageQueue::pop()
{
    qDebug() << m_queue.size();
    Song * song = nullptr;

    if (m_queue.isEmpty()) { return song; }

    m_mutex.lock();
    if (!m_queue.isEmpty())
    {
        song = m_queue.front(); //获取队头元素
        m_queue.pop_front();    //弹出队友元素
    }

    m_mutex.unlock();
    return song;
}

/*
 * 判断是否为空的接口
 * 1.手动加解锁
 * 2.使用RAII机制自动加解锁：使用QMutexLocker生成局部对象
 */
bool MessageQueue::empty()
{
    //1.手动加解锁
    //    m_mutex.lock();                 //手动加锁
    //    bool ret = m_queue.empty();
    //    m_mutex.unlock();               //手动解锁
    //    return ret;

    //2.使用RAII机制自动加解锁：使用QMutexLocker生成局部对象
    QMutexLocker locker(&m_mutex);
    return m_queue.empty();
}

//获取大小
int MessageQueue::size()
{
    QMutexLocker locker(&m_mutex);
    return m_queue.size();
}

void Worker::getASong(const QUrl &mp3Url)
{
    // 解析歌词
    qDebug() << "读取歌曲文件: " << mp3Url;

    QFileInfo info(mp3Url.path());

    if (!info.isFile())
    {
        qDebug() << "不可用的mp3文件路径：" << mp3Url;
        return;
    }

    qDebug() << "构造一个歌曲对象";
    Song * song = new Song(mp3Url, info.baseName(), "", "");

    qDebug() << "将路径后缀.mp3替换为.lrc, 然后判断是否存在歌词文件";
    QString lrcFile = mp3Url.path().replace(".mp3", ".lrc");

    // 歌词文件不存在，直接把歌曲对象放入消息队列，然后通知主线程ui（歌曲列表和歌词列表）里面可以显示
    bool ret = QFileInfo(lrcFile).isFile();
    if (!ret)
    {
        qDebug() << "歌词文件不存在: " << lrcFile;
        qDebug() << "存储到消息队列并发出信号，生成的歌曲：" << *song << endl;
        MessageQueue::getInstance().push(song);
        emit getASongFinished();
        return;
    }

    // 歌词文件存在但是没有权限打开该文件
    qDebug() << "打开歌词文件";
    QFile qfile(lrcFile);
    ret = qfile.open(QIODevice::ReadOnly | QIODevice::Text); //只读模式加文本模式打开文件

    if (!ret)
    {
        qDebug() << "打开文件失败: " << lrcFile;
        qDebug() << "存储到消息队列并发出信号，生成的歌曲：" << *song << endl;
        MessageQueue::getInstance().push(song);
        emit getASongFinished();
        return;
    }

    //  歌词文件存在可以打开该文件，继续往下解析
    qDebug() << "读取歌词文件，按行读取，解析并存储歌名、歌手名和专辑名";
    //使用QTextStream按行遍历文件，读取每行内容
    QTextStream stream(&qfile);
    QString line;   //存储一行数据
    QString text;

    while (!stream.atEnd()) //循环读取，直到读到末尾结束
    {
        //读取一行数据，如果中文乱码，考虑使用数据库课件案例中的utf8ToGbk/gbkToUtf8进行转换
        line = stream.readLine();
        //LOG << "line: " << line;
        if (line.isEmpty()) { continue; }

        //解析歌名、歌手、专辑
        //        if (line.startsWith("[ti:"))    //如果以[ti:开头说明后面是歌曲名
        //        {
        //            text = line.mid(4); //截取偏移量4开始的子串 "国际歌]"
        //            text.chop(1);       //去掉最后一个字符']' "国际歌"
        //            song->name(text);   //存储到歌曲对象中
        //            qDebug() << "读取并存储歌名：" << text;
        //        }
        if (line.startsWith("[ar:"))
        {
            text = line.mid(4);
            text.chop(1);
            song->artist(text);
            qDebug() << "读取并存储歌手：" << text;
        }
        else if (line.startsWith("[al:"))
        {
            text = line.mid(4);
            text.chop(1);
            song->album(text);
            qDebug() << "读取并存储专辑：" << text;
        }
    }

    qfile.close();


    // 歌词解析
    readLyrics(song);
    MessageQueue::getInstance().push(song);
    emit getASongFinished();

    return;
}


/*
 * 解析歌词函数，参数接收歌曲对象指针，
 *      根据歌曲url读取歌词文件，
 *      解析歌词信息存储到歌曲对象中

    歌词解析规则：
    按行读取，形如: [00:28.28]一生要走多远的路程
    1）根据']'第一次分割行内容，得：
        A [分钟数:秒数.毫秒数
        B 歌曲信息或歌词文本

    2）根据':'第二次分割，分割A得：
        A1 [分钟数
        A2 秒数.毫秒数

    3）A1去掉'['，转换成整型的分钟数
    4）A2转换成浮点数类型的秒数
    5）(A1 * 60 + A2) * 1000 = 这一行对应的播放时间进度（毫秒级）
    6）B文本可以直接存储和使用
*/
void Worker::readLyrics(Song * song)
{
    QString lrcFile = song->url().path().replace(".mp3", ".lrc");
    qDebug() << "打开歌词文件：" << lrcFile;
    QFile qfile(lrcFile);

    bool ret = qfile.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!ret)
    {
        qDebug() << "歌词文件打开失败: " << lrcFile;
        return;
    }

    //使用QTextStream按行遍历文件，读取每行内容
    QTextStream stream(&qfile);
    QString line;
    QString text;
    QStringList lineContents;   //歌词行分割后的内容数据:   时间戳    歌词文本
    QStringList timeContents;   //时间戳分割后的内容数组:   [分钟数   秒数.毫秒数
    QMap<qint64, QString> lyrics;   //存储歌词容器

    while (!stream.atEnd()) //循环读取，直到结束
    {
        line = stream.readLine();
        qDebug() << "line: " << line;
        if (line.isEmpty()) { continue; }

        //解析歌词信息
        lineContents = line.split(']');             //"时间戳"    "歌词文本"   "[00:00.00]..." ["[00:00.00", "国际歌 (《1921》电影主题曲) - 孙楠/周深"]
        timeContents = lineContents[0].split(':');  //"[分钟数"   "秒数.毫秒数" ["[00", "00.00"]

        //排除非歌词的信息行，将歌词行中的"[分钟数"转换成整型数值，如果转换失败说明不是歌词行
        bool ok = false;    //获取转换是否成功
        text = timeContents[0].mid(1);   // "02"
        int minutes = text.toInt(&ok);  //字符串转数值，传入参数接收是否转成功
        if (!ok) { continue; }

        double seconds = timeContents[1].toDouble();    //字符串转浮点数  68
        lyrics.insert((minutes * 60 + seconds) * 1000, lineContents[1]); // {"188000", "词 Lyricist：Eugène Edine Pottier"};
        qDebug() << "新增歌词：" << (minutes * 60 + seconds) * 1000 << " " << lineContents;
    }

    qfile.close();
    qDebug() << "解析歌词结束，存储到歌曲对象中，歌词行数：" << lyrics.size();
    song->lyrics(lyrics);
}
