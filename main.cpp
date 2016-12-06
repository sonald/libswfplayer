#include <QString>
#include <QtGui>

#include "swfplayer.h"

static int width = 700, height = 500;
//default testing file
const char swffile[] = "file:///home/sonald/Dropbox/stage/deepin/jinshan/dragandpop.swf";

static KSwfPlayer *w;
static QString file;

class MainWindow: public QTabWidget {
    Q_OBJECT
    public:
        MainWindow(): QTabWidget(0) {
            auto *tab1 = new QWidget;
            {
                auto *layout = new QVBoxLayout(tab1);
                //layout->addStretch();

                w = new KSwfPlayer();
                w->LoadSwf(file);
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

    private:
        QLabel *_lb;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    file = QString::fromUtf8(swffile);
    if (argc == 2) {
        file = (QString::fromUtf8(argv[1]));
    }

    if (!KSwfPlayer::checkPreRequirements()) {
        return 0;
    }

    //Window which KSwfPlayer window embeds into
    MainWindow* w2 = new MainWindow;
    w2->setAttribute(Qt::WA_QuitOnClose, true);
    QObject::connect(w2, SIGNAL(currentChanged(int)), w2, SLOT(OnCurrenChanged(int)));

    width = w->SizePrefered().width(), height = w->SizePrefered().height();
    w2->resize(width + 80, height + 100);
    w2->show();

    app.exec();
    return 0;
}

#include "main.moc"
