#include <QString>
#include <QtGui>

#include "swfplayer.h"

static int width = 700, height = 500;
//default testing file
const char swffile[] = "file:///home/sonald/Dropbox/stage/deepin/jinshan/dragandpop.swf";

static KSwfPlayer *w = NULL;
static QString file;

using NewPlayer = KSwfPlayer* (*)();

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
#if DEBUG_TEST
            _lib = new QLibrary("libswfplayer.so");
            if (_lib->load()) {
                NewPlayer fn = (NewPlayer)_lib->resolve("new_player");
                w = fn();
                if (w->CheckPlugins()) {
                    w->LoadSwf(file);
                }
                delete w;

                _lib->unload();
                delete _lib;
            }
#endif

            _lib = new QLibrary("libswfplayer.so");
            if (_lib->load()) {
                NewPlayer fn = (NewPlayer)_lib->resolve("new_player");
                if (fn) {
                    w = fn();
                    w->setParent(this);
                    if (w->CheckPlugins()) {
                        w->LoadSwf(file);
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
                QObject::connect(pbPlay, SIGNAL(pressed()), w, SLOT(Play()));

                auto *pbStop = new QPushButton(tab1);
                pbStop->setText("Stop");
                hbox->addWidget(pbStop);
                QObject::connect(pbStop, SIGNAL(pressed()), w, SLOT(Stop()));

                auto *pbPause = new QPushButton(tab1);
                pbPause->setText("Pause");
                hbox->addWidget(pbPause);
                QObject::connect(pbPause, SIGNAL(pressed()), w, SLOT(Pause()));


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
            _lb->setPixmap(QPixmap::fromImage(w->ThumbNail()));
            layout->addWidget(_lb);
            layout->addWidget(pb);
            tab2->setLayout(layout);

            this->addTab(tab2, "Page2");
        }

        public slots:
            void toggleDebug() {
                static bool on = false;
                
                on = !on;
                w->EnableDebug(on);
            }

            void OnCurrenChanged(int index) {
                qDebug() << "currentChanged: " << index;
                if (index == 0) {
                    w->show();
                } else {
                    //if (w->state() == KSwfPlayer::Loaded)
                        //_lb->setPixmap(QPixmap::fromImage(w->ThumbNail()));
                    w->hide();
                }
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

    width = w->SizePrefered().width(), height = w->SizePrefered().height();
    w2->resize(width + 80, height + 100);
    w2->show();

    app.exec();

    qDebug() << "cleanup";
    delete w2;
    return 0;
}

#include "main.moc"
