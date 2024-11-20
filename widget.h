#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QMediaContent>
#include <QListWidgetItem>
#include <QThread>
#include "worker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT
signals:
    void addSong(const QUrl& mp3Url); //添加歌曲信号

public slots:
    void handle_worker_getASongFinished(); //处理工作对象解析结束信号
public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
public:
    void init_media();
    void init_window();
    void init_worker();
    void updateAllLyrics(const QMap<qint64, QString>&);
    void updateCurrentLyric();

public slots:
    void pushButton_play_clicked();
    void pushButton_add_clicked();
    void horizontalSlider_position_sliderReleased();
    void pushButton_previous_clicked();
    void pushButton_next_clicked();
    void pushButton_playbackmodel_clicked();
    void listWidget_playlist_itemDoubleClicked(QListWidgetItem *);


public slots:
    void handle_mediaPlayer_stateChanged(QMediaPlayer::State);
    void handle_mediaPlayer_positionChanged(qint64 position);
    void handle_mediaPlaylist_currentMediaChanged(const QMediaContent&);
    void handleMediaPlaylistPlaybackModeChanged(QMediaPlaylist::PlaybackMode);



private:
    Ui::Widget *ui;
    QMediaPlayer *m_pmediaplayer;
    QMediaPlaylist *m_pmediaplayerlist;
    QThread* m_pthread;
    Worker* m_pworker;

};
#endif // WIDGET_H
