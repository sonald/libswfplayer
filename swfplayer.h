#ifndef _SWF_PLAYER_H
#define _SWF_PLAYER_H 

#include <QtCore>
#include <QImage>
#include <QWebView>

struct SwfFileInfo;

class QSwfPlayer: public QWebView {
    Q_OBJECT
    public:
        //check requirements for this player to work correctly.
        static bool checkPreRequirements();

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
        // grab a snapshot of current content when playing
        void grab(QString filepath);

        // preview of swf if it contains video info, 
        // else extract a frame from flash by gnash, if that fails,
        // return a blank image with size of swf 
        QImage thumbnail() const;
        // size of swf if it's loaded.
        QSize preferedSize() const { return _preferedSize; }
        State state() { return _state; }

        // load a swf file from filename (absolute or relative path) and then parse file 
        // to get thumbnail, size info
        void loadSwf(QString& filename);

        // turn on menu of inspection for debugging
        void enableDebug(bool on);

    protected:
        void resizeEvent(QResizeEvent *event);
        void showEvent(QShowEvent *event);
        void hideEvent(QHideEvent *event);
        void closeEvent(QCloseEvent *event);
        void contextMenuEvent(QContextMenuEvent * event);

    private:
        bool _loaded;
        SwfFileInfo *_swfInfo;
        QSize _preferedSize;
        State _state;
        bool _enableDebug;

        QVariant eval(const QString& script);

    private slots:
        void onLoadFinished(bool ok);
};



#endif /* ifndef _SWF_PLAYER_H */

