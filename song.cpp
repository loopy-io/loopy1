#include "song.h"
#include <QDebug>

Song::Song()
{

}

QDebug& operator<<(QDebug& debug, const Song& song)
{
    debug << song.m_url << ", "
          << song.m_name << ", "
          << song.m_artist << ", "
          << song.m_album << ", "
          << "歌词行数：" << song.m_lyrics.size();
    return debug;
}
