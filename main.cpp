#include <QString>
#include <QtGui>

#include "swfplayer.h"

static int width = 700, height = 500;
//default testing file
const char swffile[] = "file:///home/sonald/Dropbox/stage/deepin/jinshan/dragandpop.swf";

static KSwfPlayer *w = NULL;
static QString file;

using NewPlayer = KSwfPlayer* (*)(QWidget*);
using FnCheckPlugins = bool (*)(KSwfPlayer*);
using FnLoadSwf = void (*)(KSwfPlayer*, QString*);
using FnSizePrefered = void (*) (KSwfPlayer*, QSize*);
using FnEnableDebug = void (*) (KSwfPlayer*, bool bEnableDebug);
using FnAction = void (*)(KSwfPlayer*);
using FnGetSwfPlayerState = KSwfPlayer::SwfPlayState (*) (KSwfPlayer* obj);
//void Grab(void*, QString strFilePath);
using FnThumbNail = void (*) (KSwfPlayer* obj, QImage* pImg);

class MainWindow: public QTabWidget {
    Q_OBJECT
    public:
        virtual ~MainWindow() {
            qDebug() << __func__;
            delete w;
            if (!_lib->unload()) {
                qDebug() << _lib->errorString();
            }
            delete _lib;
        }

        MainWindow(): QTabWidget(0) {
            {
                _lib = new QLibrary("libswfplayer.so");
                if (_lib->load()) {
                    NewPlayer fn = (NewPlayer)_lib->resolve("CreateKSwfPlayer");
                    FnCheckPlugins fn2 = (FnCheckPlugins)_lib->resolve("CheckPlugins");
                    FnLoadSwf fn3 = (FnLoadSwf)_lib->resolve("LoadSwf");

                    if (fn) {
                        w = fn(this);
                        if (fn2(w)) {
                            fn3(w, &file);
                        }
                    }
                }

                delete w;
                _lib->unload();
                delete _lib;
            }

            _lib = new QLibrary("libswfplayer.so");
            if (_lib->load()) {
                NewPlayer fn = (NewPlayer)_lib->resolve("CreateKSwfPlayer");
                FnCheckPlugins fn2 = (FnCheckPlugins)_lib->resolve("CheckPlugins");
                FnLoadSwf fn3 = (FnLoadSwf)_lib->resolve("LoadSwf");

                if (fn) {
                    w = fn(this);
                    if (fn2(w)) {
                        fn3(w, &file);
                    }
                } else {
                    qDebug() << _lib->errorString();
                }
            } else {
                qDebug() << _lib->errorString();
            }

            {
                _bar = new QWidget(NULL);
                _bar->setWindowFlags(Qt::X11BypassWindowManagerHint|Qt::WindowStaysOnTopHint|Qt::CustomizeWindowHint);
                _bar->resize(600, 40);
                auto *layout = new QHBoxLayout(_bar);

                const char* names[] = {"play", "pause", "stop"};
                for (int i = 0; i < 3; i++) {
                    auto *btn = new QPushButton(names[i]);
                    layout->addWidget(btn);
                }

                _bar->setLayout(layout);
                _bar->setVisible(false);
            }

            auto *tab1 = new QWidget;
            {
                auto *layout = new QVBoxLayout(tab1);
                //layout->addStretch();

                layout->addWidget(w);

                auto *hbox = new QHBoxLayout;
                layout->addLayout(hbox);

                auto *pbPlay = new QPushButton(tab1);
                QString txt = QString::fromStdString("Play");
                pbPlay->setText("Play");
                hbox->addWidget(pbPlay);
                QObject::connect(pbPlay, SIGNAL(pressed()), this, SLOT(Play()));

                auto *pbStop = new QPushButton(tab1);
                pbStop->setText("Stop");
                hbox->addWidget(pbStop);
                QObject::connect(pbStop, SIGNAL(pressed()), this, SLOT(Stop()));

                auto *pbPause = new QPushButton(tab1);
                pbPause->setText("Pause");
                hbox->addWidget(pbPause);
                QObject::connect(pbPause, SIGNAL(pressed()), this, SLOT(Pause()));


                auto *pbDebug = new QPushButton(tab1);
                pbDebug->setText("Debug");
                hbox->addWidget(pbDebug);
                QObject::connect(pbDebug, SIGNAL(pressed()), this, SLOT(toggleDebug()));

                tab1->setLayout(layout);
            }

            this->addTab(tab1, "Page1");

            auto *tab2 = new QWidget;
            auto *pb = new QPushButton(tab2);
            pb->setText("quit");

            auto *layout = new QVBoxLayout(tab2);
            _lb = new QLabel;
            //_lb->setPixmap(QPixmap::fromImage(w->ThumbNail()));
            layout->addWidget(_lb);
            layout->addWidget(pb);
            tab2->setLayout(layout);

            this->addTab(tab2, "Page2");
        }

        QSize SizePrefered() {
            QSize sz;
            FnSizePrefered fn = (FnSizePrefered)_lib->resolve("SizePrefered");
            fn(w, &sz);
            return sz;
        }

        public slots:
            void toggleDebug() {
                static bool on = false;
                
                on = !on;
                FnEnableDebug fn = (FnEnableDebug)_lib->resolve("EnableDebug");
                fn(w, on);
            }

            void OnCurrenChanged(int index) {
                qDebug() << "currentChanged: " << index;
                if (index == 0) {
                    w->show();
                } else {
                    auto fn = (FnGetSwfPlayerState)_lib->resolve("GetSwfPlayerState");
                    KSwfPlayer::SwfPlayState state = fn(w);
                    if (state == KSwfPlayer::Loaded) {
                        QImage thumb;
                        auto fn = (FnThumbNail)_lib->resolve("ThumbNail");
                        fn(w, &thumb);
                        _lb->setPixmap(QPixmap::fromImage(thumb));
                    }
                    w->hide();
                }
            }

            void Play() {
                FnAction fn = (FnAction)_lib->resolve("Play");
                fn(w);
            }

            void Stop() {
                FnAction fn = (FnAction)_lib->resolve("Stop");
                fn(w);
            }

            void Pause() {
                FnAction fn = (FnAction)_lib->resolve("Pause");
                fn(w);
            }


    protected:
        void closeEvent(QCloseEvent *ce) {
            ce->accept();
            qApp->quit();
        }

        void keyPressEvent(QKeyEvent *event)
        {
            qDebug() << "main " << __func__;
            if (event->key() == Qt::Key_F5) {
                if (isFullScreen()) 
                    showNormal();
                else {
                    showFullScreen();
                }
                _bar->setVisible(isFullScreen());
                _bar->move(400, 800);

            } else if (event->key() == Qt::Key_F6) {
                if (isFullScreen()) {
                    _bar->setVisible(!_bar->isVisible());
                }
            }

            QTabWidget::keyPressEvent(event);
        }

        void mousePressEvent(QMouseEvent *event)
        {
            qDebug() << "main " << __func__;
            QTabWidget::mousePressEvent(event);
        }

    private:
        QLabel *_lb;
        QLibrary *_lib;
        QWidget *_bar; // control bar
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    file = QString::fromUtf8(swffile);
    if (argc == 2) {
        file = (QString::fromUtf8(argv[1]));
    }

    //Window which KSwfPlayer window embeds into
    MainWindow* w2 = new MainWindow;
    w2->setAttribute(Qt::WA_QuitOnClose, true);
    QObject::connect(w2, SIGNAL(currentChanged(int)), w2, SLOT(OnCurrenChanged(int)));

    width = w2->SizePrefered().width(), height = w2->SizePrefered().height();
    w2->resize(width + 80, height + 100);
    w2->show();

    app.exec();

    qDebug() << "cleanup";
    delete w2;
    return 0;
}

#include "moc_main.cxx"
