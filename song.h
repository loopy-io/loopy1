#ifndef SONG_H
#define SONG_H


/*歌曲类，
 * 封装媒体文件路径url、歌曲名name、歌手artist、专辑名album等歌曲信息，
 * 以及一个歌词容器lyrics作为数据成员
 */
#include <QUrl>
#include <QString>
#include <QMap>
class Song
{
private:
    QUrl m_url;
    QString m_name;
    QString m_artist;
    QString m_album;
    QMap<qint64, QString> m_lyrics; //键值对容器，存储歌词时间戳（毫秒级播放进度），和歌词文本
public:
    Song();
    Song(const QUrl & url,
         const QString & name,
         const QString & artist,
         const QString & album)
        : m_url(url), m_name(name), m_artist(artist), m_album(album)
    {}

    //get系列方法，可以根据常量方法的常量修饰符进行重载
    //  针对读取方法，定义成常量方法，可供非常量对象和常量对象一起使用
    const QUrl & url() const { return m_url; }
    QUrl url() { return m_url; }
    //set方法
    void url(const QUrl & url) { m_url = url; }

    const QString & name() const { return m_name; }
    const QString & artist() const { return m_artist; }
    const QString & album() const { return m_album; }
    void name(const QString & name) { m_name = name; }
    void artist(const QString & artist) { m_artist = artist; }
    void album(const QString & album) { m_album = album; }

    const QMap<qint64, QString> & lyrics() const        { return m_lyrics; }
    void lyrics(const QMap<qint64, QString> & lyrics)   { m_lyrics = lyrics; }

    //重载输出Song类对象的输出运算符函数，输出流类型使用QDebug&
    //注意：头文件声明友元，源文件里定义函数
    friend QDebug& operator<<(QDebug & debug, const Song & song);
};

/*歌曲对象管理类，管理运行时添加的所有歌曲构造的歌曲对象
 *  实现为单例类
 *  包含一个歌曲对象列表，存储<路径，动态分配的歌曲对象指针>键值对容器，注意释放问题
 *  返回歌曲对象列表的接口
 *  查询歌曲对象数量size
 *  清空歌曲接口
 *  是否包含某首歌接口
 *  根据歌曲url查找并返回某首歌（的引用）的接口
 *  根据歌曲url，返回某首歌的歌词
 *  添加歌曲接口，接收歌曲对象指针
 *  todo: 删除歌曲接口，参数接收歌曲路径
 */
class SongManager
{
private:
    //歌曲对象列表，存储<路径，歌曲对象指针>键值对容器，注意释放问题
    QMap<QUrl, Song*> m_songs;

private:
    SongManager() {}
    SongManager(const SongManager& other) {}
    ~SongManager() { clear(); }

public:
    static SongManager& getInstance()
    {
        static SongManager instance;
        return instance;
    }

    //返回歌曲对象列表的接口
    QMap<QUrl, Song*>& songs() { return m_songs; }

    //查询歌曲对象数量size
    int size() const { return m_songs.size(); }

    //清空歌曲接口clear
    void clear()
    {
        for(auto song : m_songs)
        {
            if (song) { delete song; }
        }
        m_songs.clear();
    }

    //是否包含某首歌接口
    bool contains(const QUrl& mp3Url) { return m_songs.contains(mp3Url); }

    //根据歌曲url，查找并返回某首歌（的引用）的接口
    //  用户应该先判断是否有这首歌，再来获取引用
    Song& song( const QUrl& url) { return *m_songs[url]; }

    //根据歌曲url，返回某首歌的歌词
    //  假设歌曲列表中包含这首歌
    const QMap<qint64, QString>& lyrics(const QUrl& url) { return m_songs[url]->lyrics(); }

    //添加歌曲接口，接收歌曲对象指针
    void addSong(Song* song)
    {
        if (song)
        {
            m_songs.insert(song->url(), song);
        }
    }
};

#endif // SONG_H
