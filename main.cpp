#include <QX11Info>
#include <QString>

#include "swfplayer.h"

#include <X11/Xlib.h>
static int width = 700, height = 500;
//default testing file
const char swffile[] = "file:///home/sonald/Dropbox/stage/deepin/jinshan/dragandpop.swf";

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto *w = new QSwfPlayer;
    w->setWindowFlags(Qt::BypassWindowManagerHint|Qt::CustomizeWindowHint);

    w->resize(width, height);
    w->setAttribute(Qt::WA_QuitOnClose, true);
    QString file = QString::fromUtf8(swffile);
    if (argc == 2) {
        file = (QString::fromUtf8(argv[1]));
    }
    w->loadSwf(file);
    w->show();

    auto* w2 = new QTabWidget;
    w2->setAttribute(Qt::WA_QuitOnClose, true);

    auto *tab1 = new QWidget;
    {
        auto *layout = new QVBoxLayout(tab1);
        layout->addStretch();

        auto *hbox = new QHBoxLayout;
        layout->addLayout(hbox);

        auto *pbPlay = new QPushButton(tab1);
        pbPlay->setText("Play");
        hbox->addWidget(pbPlay);
        QObject::connect(pbPlay, &QPushButton::pressed, [w]() {
            w->page()->mainFrame()->evaluateJavaScript("Play()");
        });

        auto *pbStop = new QPushButton(tab1);
        pbStop->setText("Stop");
        hbox->addWidget(pbStop);
        QObject::connect(pbStop, &QPushButton::pressed, [w]() {
            w->page()->mainFrame()->evaluateJavaScript("Stop()");
        });

        auto *pbPause = new QPushButton(tab1);
        pbPause->setText("Pause");
        hbox->addWidget(pbPause);
        QObject::connect(pbPause, &QPushButton::pressed, [w]() {
            w->page()->mainFrame()->evaluateJavaScript("Pause()");
        });

        auto *pbGrab = new QPushButton(tab1);
        pbGrab->setText("Grab");
        hbox->addWidget(pbGrab);
        QObject::connect(pbGrab, &QPushButton::pressed, [w]() {
            w->grab("snapshot.png");
        });

        tab1->setLayout(layout);
    }

    w2->addTab(tab1, "Page1");

    auto *tab2 = new QWidget;
    auto *pb = new QPushButton(tab2);
    pb->setText("quit");

    auto *layout = new QVBoxLayout(tab2);
    //layout->addStretch();
    layout->addWidget(pb);
    tab2->setLayout(layout);

    w2->addTab(tab2, "Page2");

    QObject::connect(w2, &QTabWidget::currentChanged, [=](int index) {
        qDebug() << "currentChanged: " << index;
        if (index == 0) {
            w->show();
        } else {
            w->hide();
        }
    });
    w2->resize(width + 80, height + 100);
    w2->show();

    XReparentWindow(QX11Info::display(), w->winId(), w2->winId(), 20, 40);
    app.exec();
    return 0;
}
