#include "widget.h"
#include "ui_widget.h"
#include <QFileDialog>
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "song.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , m_pthread(new QThread)

{
    ui->setupUi(this);

    init_media();                            //media引擎初始化

    init_window();                          //界面布局

    init_worker();
    
    connect(ui->pushButton_add,&QPushButton::clicked,this,&Widget::pushButton_add_clicked);                  //添加音乐按钮

    connect(ui->pushButton_play,&QPushButton::clicked,this,&Widget::pushButton_play_clicked);                //添加播放按键

    connect(m_pmediaplayer,&QMediaPlayer::stateChanged,this,&Widget::handle_mediaPlayer_stateChanged);       //播放状态显示到pushbutton

    connect(m_pmediaplayer,&QMediaPlayer::positionChanged,this,&Widget::handle_mediaPlayer_positionChanged); //播放进度显示到进度条

    connect(ui->horizontalSlider_time,&QSlider::sliderReleased,this,&Widget::horizontalSlider_position_sliderReleased); //拖动进度条改编歌曲进度

    connect(ui->pushButton_previous,&QPushButton::clicked,this,&Widget::pushButton_previous_clicked);        //上一首

    connect(ui->pushButton_next,&QPushButton::clicked,this,&Widget::pushButton_next_clicked);                //下一首

    connect(m_pmediaplayer,&QMediaPlayer::currentMediaChanged,this,&Widget::handle_mediaPlaylist_currentMediaChanged); //切歌时发生的一系列变化

    connect(ui->pushButton_playbackmodel,&QPushButton::clicked,this,&Widget::pushButton_playbackmodel_clicked);        //点击播放模式变化

    connect(m_pmediaplayerlist,&QMediaPlaylist::playbackModeChanged,this,&Widget::handleMediaPlaylistPlaybackModeChanged); //播放模式变化体现在按钮文本

    connect(ui->listWidget_music, &QListWidget::itemDoubleClicked, this, &Widget::listWidget_playlist_itemDoubleClicked); //双击列表中音乐项目
}


Widget::~Widget()
{
    delete ui;

}

void Widget::init_media()                       //初始化多媒体
{
    m_pmediaplayer = new QMediaPlayer(this);
    m_pmediaplayerlist = new QMediaPlaylist(this);

    m_pmediaplayer->setPlaylist(m_pmediaplayerlist);
    m_pmediaplayerlist->setPlaybackMode(QMediaPlaylist::Loop);



    return;
}

void Widget::init_worker()
{
    m_pworker = new Worker;
    m_pworker->moveToThread(m_pthread);
    connect(this, &Widget::addSong, m_pworker, &Worker::getASong);
    connect(m_pworker, &Worker::getASongFinished, this, &Widget::handle_worker_getASongFinished);

    m_pthread->start();

    return;
}

void Widget::handle_worker_getASongFinished()
{
    // 从消息队列取出一个音乐对象
    Song* psong = MessageQueue::getInstance().pop();

    // 把该音乐对象加到音乐管理对象里面
    SongManager::getInstance().addSong(psong);

    // 取出该音乐对象里面的歌曲路径加入到音乐播放器列表
    m_pmediaplayerlist->addMedia(psong->url());

    // 取出该音乐对象里面的歌曲路径加入到ui音乐列表
    ui->listWidget_music->addItem( new QListWidgetItem(psong->name()) );

}
        
void Widget::init_window()                     //界面布局
{
    this->setWindowTitle("音乐播放器");
    this->setWindowIcon(QIcon(":/icons/music.png"));


    ui->pushButton_add->setIcon(QIcon(":/icons/add.png"));           //图标
    ui->pushButton_previous->setIcon(QIcon(":/icons/previous.png"));
    ui->pushButton_play->setIcon(QIcon(":/icons/play.png"));
    ui->pushButton_next->setIcon(QIcon(":/icons/next.png"));
    ui->pushButton_playbackmodel->setIcon(QIcon(":/icons/loop.png"));


    ui->listWidget_music->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget_music->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget_music->setStyleSheet("background-color:transparent");
    ui->listWidget_lyrics->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget_lyrics->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget_lyrics->setStyleSheet("background-color:transparent");

    QVBoxLayout * V1 = new QVBoxLayout();
    V1->addWidget(ui->pushButton_add);
    V1->addWidget(ui->pushButton_previous);
    V1->addWidget(ui->pushButton_play);
    V1->addWidget(ui->pushButton_next);
    V1->addWidget(ui->pushButton_playbackmodel);


    QVBoxLayout * V2 = new QVBoxLayout();
    V2->addWidget(ui->listWidget_music);

    QVBoxLayout * V3 = new QVBoxLayout();
    V3->addWidget(ui->listWidget_lyrics);

    QHBoxLayout * H1 = new QHBoxLayout();
    H1->addLayout(V1,1);
    H1->addLayout(V2,3);
    H1->addLayout(V3,6);

    QHBoxLayout * H2 = new QHBoxLayout();
    H2->addWidget(ui->label_song,Qt::AlignRight);


    QHBoxLayout * H3 = new QHBoxLayout();
    H3->addWidget(ui->horizontalSlider_time,9);
    H3->addWidget(ui->label_time,1);

    QVBoxLayout * V = new QVBoxLayout();
    V->addLayout(H1);
    V->addLayout(H2);
    V->addLayout(H3);

    setLayout(V);

    return;
}


void Widget::pushButton_add_clicked()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, "添加音乐", QDir::currentPath(),"(*.mp3)");
    if (fileNames.isEmpty())
    {
        // 对选择的多个文件进行操作
        qDebug() << "选择文件为空";
        return;
    }

    for (auto file : fileNames) //范围for循环，从files中遍历获取每一个元素赋值给file
    {
        if (SongManager::getInstance().contains(QUrl(file)))
        {
            qDebug() << "歌曲管理员已存储，不重复添加：" << file;
            continue;
        }

        emit addSong(file);
        /*
        //设置到媒体播放列表
        m_pmediaplayerlist->addMedia(QMediaContent(QUrl(file)));
        ui->listWidget_music->addItem(new QListWidgetItem(QFileInfo(file).baseName()));*/
    }


    return;
}

void Widget::pushButton_play_clicked() //播放/暂停功能
{


        if(QMediaPlayer::PlayingState == m_pmediaplayer->state())
        {
            m_pmediaplayer->pause();
        }
        else
        {
            m_pmediaplayer->play();
        }

        return;

}

void Widget::handle_mediaPlayer_stateChanged(QMediaPlayer::State newState)//暂停/播放显示切换
{
    if(QMediaPlayer::PlayingState == newState)
    {
        ui->pushButton_play->setText("暂停");
    }
    else
    {
        ui->pushButton_play->setText("播放");
    }

    return;
}

void Widget::handle_mediaPlayer_positionChanged(qint64 position)//进度条同步歌曲显示
{

    // 当前播放到的位置（毫秒）转变成秒
    qint64 seconds = position / 1000;

    //歌曲总时长（毫秒）转变成秒
    qint64 durationSeconds = m_pmediaplayer->duration() / 1000;

    // 滚动条的总长度
    int slider_max = ui->horizontalSlider_time->maximum();

    // 前端进度条的值 = 后台当前播放进度 / 后台歌曲时长 * 前端进度条最大值
    ui->horizontalSlider_time->setValue((double)seconds / durationSeconds * slider_max);

    QString timetext = QString("%1:%2/%3:%4")
            .arg(seconds / 60, 2, 10,  QChar('0'))
            .arg(seconds % 60, 2, 10,  QChar('0'))
            .arg(durationSeconds / 60, 2, 10,  QChar('0'))
            .arg(durationSeconds % 60, 2, 10,  QChar('0'));

    ui->label_time->setText(timetext);

    updateCurrentLyric();

    return;
}

void Widget::horizontalSlider_position_sliderReleased() //松开进度条，歌曲定位同步
{


        // 前端进度条的值
        int slider_value = ui->horizontalSlider_time->value();

        //歌曲总时长毫秒
        qint64 durationSeconds = m_pmediaplayer->duration();

        // 滚动条的总长度
        int slider_max = ui->horizontalSlider_time->maximum();

        // 后台播放进度(毫秒） = 前端进度条 / 进度条最大值 * 后台歌曲时长
        m_pmediaplayer->setPosition((double) durationSeconds / slider_max *slider_value);

        return;

}

void Widget::pushButton_previous_clicked()
{
    qDebug()<<"切换上一首";
    m_pmediaplayerlist->previous();
    return;
}


void Widget::pushButton_next_clicked()
{
    qDebug()<<"切换下一首";
    m_pmediaplayerlist->next();
    return;
}

void Widget::handle_mediaPlaylist_currentMediaChanged(const QMediaContent &media)
{
        qDebug() << "切歌";

        if (media.isNull())
        {
            ui->label_song->clear();
            ui->listWidget_music->setCurrentRow(m_pmediaplayerlist->currentIndex());

            return;
        }
        // 当切歌的时候，重新设置歌曲名字
        QString fileName = media.canonicalUrl().fileName();

        ui->label_song->setText(fileName);
        ui->listWidget_music->setCurrentRow(m_pmediaplayerlist->currentIndex());


        Song & songRef = SongManager::getInstance().song(media.canonicalUrl());

        updateAllLyrics(songRef.lyrics());

        return;
}



void Widget::pushButton_playbackmodel_clicked()    //播放模式切换
{

    QMediaPlaylist::PlaybackMode mode = m_pmediaplayerlist->playbackMode();

    int next_mode = (mode + 1) % 5 ;

    m_pmediaplayerlist->setPlaybackMode(QMediaPlaylist::PlaybackMode(next_mode));

    return;

}
void Widget::handleMediaPlaylistPlaybackModeChanged(QMediaPlaylist::PlaybackMode model)  //播放模式切换后显示在按键上
{

    QStringList models = {"单曲播放","单曲循环","顺序播放","循环播放","随机播放"};

    ui->pushButton_playbackmodel->setText(models[model]);

    return;
}

void Widget::listWidget_playlist_itemDoubleClicked(QListWidgetItem *item)//前端歌曲列表双击某一首歌切歌并播放：前端歌曲列表双击当前行（双击信号） -> 设置媒体播放列表当前索引, 并调用媒体播放器的播放函数
{

    m_pmediaplayerlist->setCurrentIndex(ui->listWidget_music->currentRow());
    m_pmediaplayer->play();
    return;
}

void Widget::updateCurrentLyric()
{
    if (m_pmediaplayer->state() != QMediaPlayer::PlayingState)
    {
        qDebug() << "当前不是播放状态, 不用同步";
        return;
    }

    QUrl url = m_pmediaplayer->currentMedia().canonicalUrl();
    const QMap<qint64, QString> & lyrics = SongManager::getInstance().lyrics(url);
    if (lyrics.isEmpty())
    {
        qDebug() << "当前歌曲没有歌词，不用同步";
        return;
    }

    qDebug() << "实时刷新当前歌词，播放器当前进度：" << m_pmediaplayer->position();
    qint64 position = m_pmediaplayer->position();
    int index = 0;  //存储当前进度对应的歌词文本的索引

    //循环遍历歌词容器，从头往后找
    for (auto it = lyrics.begin(); it != lyrics.end(); it++)
    {
        //end前面的最后一个元素
        if (lyrics.end() == (it + 1)) { break; }

        //比较播放进度和时间戳，播放进度大于等于某一行（的起始事件），并且小于下一行
        if (position >= it.key() && position < (it + 1).key())
        {
            qDebug() << "当前歌词时间范围和播放器进度及文本：["
                     << it.key() << ", " << (it + 1).key() << ") "
                     << position << " "
                     << it.value();
            break;
        }

        index++;    //如果不是这一行，就继续判断下一行
    }

    ui->listWidget_lyrics->setCurrentRow(index);    //界面歌词控件设置当前行，高亮显示
    QListWidgetItem * item = ui->listWidget_lyrics->item(index);    //获取当前行元素
    ui->listWidget_lyrics->scrollToItem(item, QAbstractItemView::PositionAtCenter); //列表滚动到该行，并垂直居中
}


void Widget::updateAllLyrics(const QMap<qint64, QString>& lyrics)
{
    // 清空上一首歌的歌词
    ui->listWidget_lyrics->clear();

    // 该歌曲没有歌词
    if (lyrics.isEmpty())
    {
        QListWidgetItem * item = new QListWidgetItem("无歌词");
        item->setTextAlignment(Qt::AlignCenter);    //设置该行文本居中显示
        ui->listWidget_lyrics->addItem(item);
        return;
    }

    // 该歌曲有歌词
    qDebug() << "更新所有歌词，第一行文本：" << lyrics.first();
    for (auto text : lyrics) //获取lyrcis中的每行歌词的文本到text
    {
        //LOG << "text: " << text;
        QListWidgetItem * item = new QListWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);    //设置该行文本居中显示
        ui->listWidget_lyrics->addItem(item);
    }

    return;
}








