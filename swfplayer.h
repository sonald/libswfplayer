#ifndef _SWF_PLAYER_H
#define _SWF_PLAYER_H 

#include <QtWebKitWidgets>

struct SwfFileInfo {
    char sig[4];
    char version;
    int length;
    bool compressed;

    int width;
    int height;

    bool valid;
    static SwfFileInfo parseSwfFile(const QString& file);
};


class QSwfPlayer: public QWebView {
    public:
        QSwfPlayer(QWidget* parent = 0);

    public slots:
        void play();
        void stop();
        void pause();
        // grab a snapshot of current content
        void grab(QString filepath);
        void loadSwf(QString& filename);

    protected:
        void resizeEvent(QResizeEvent *event);

    private:
        bool _loaded;
        SwfFileInfo _swfInfo;

        QVariant eval(const QString& script);

    private slots:
        void onLoadFinished(bool ok);
};

#endif /* ifndef _SWF_PLAYER_H */

