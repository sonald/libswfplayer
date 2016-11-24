#ifndef _SWF_PLAYER_H
#define _SWF_PLAYER_H 

#include <QtWebKitWidgets>

struct SwfFileInfo;

class QSwfPlayer: public QWebView {
    public:
        enum State {
            Invalid, // no swf loaded
            Loaded, // swf is loaded, and not playing
            Playing,
            Paused
        };
        QSwfPlayer(QWidget* parent = 0);

    public slots:
        //NOTE: not all swf can be stopped/paused
        void play();
        void stop();
        void pause();
        // grab a snapshot of current content
        void grab(QString filepath);

        // preview of swf if it contains video info, else return a blank image with size of swf 
        QImage thumbnail() const;
        // size of swf if it's loaded.
        QSize preferedSize() const { return _preferedSize; }
        State state() { return _state; }

        // load a swf file from filename (absolute or relative path) and then parse file 
        // to get thumbnail, size info
        void loadSwf(QString& filename);

    protected:
        void resizeEvent(QResizeEvent *event);

    private:
        bool _loaded;
        SwfFileInfo *_swfInfo;
        QSize _preferedSize;
        State _state;

        QVariant eval(const QString& script);

    private slots:
        void onLoadFinished(bool ok);
};

#endif /* ifndef _SWF_PLAYER_H */

