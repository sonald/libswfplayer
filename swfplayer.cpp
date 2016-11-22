#include <QtWebKitWidgets>
#include "swfplayer.h"

/**
 * mms.cfg: https://gist.github.com/orumin/6365218
 *
 */

const char templ[] = R"(
<html lang="en">
    <head>
        <title>Player</title>
        <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
        <script type="text/javascript" src="file:///home/sonald/Dropbox/stage/deepin/jinshan/swfobject.js"></script>
    </head>
    <body>
        <div id="player">
            <h1>Alternative content</h1>
        </div>
        <script type="text/javascript">
            // swfobject.embedSWF("file:///home/sonald/Dropbox/stage/deepin/jinshan/dragandpop.swf", "player", "%1", "%2", "9.0.0");
            function Play(){
                document.getElementById('player').Play();
            }

            function Pause(){
                document.getElementById('player').StopPlay();
            }

            function Stop(){
                document.getElementById('player').GotoFrame(1);
            }
            swfobject.embedSWF("%3", "player", "%1", "%2", "9.0.0");
        </script>
    </body>
</html>
)";

QSwfPlayer::QSwfPlayer(QWidget* parent)
    : QWebView(parent),
    _loaded(false)
{
    if (settings()) {
        settings()->setAttribute(QWebSettings::PluginsEnabled, true);
        settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    }
}

void QSwfPlayer::play()
{
    if (this->page())
        this->page()->mainFrame()->evaluateJavaScript("Play()");
}

void QSwfPlayer::stop()
{
    if (this->page())
        this->page()->mainFrame()->evaluateJavaScript("Stop()");
}

void QSwfPlayer::pause()
{
    if (this->page())
        this->page()->mainFrame()->evaluateJavaScript("Pause()");
}

void QSwfPlayer::grab(QString filepath)
{
    QPixmap pixmap(this->size());
    this->render(&pixmap);

    if (filepath.isEmpty())
        filepath = "swfsnapshot.png";
    pixmap.save(filepath);
}

void QSwfPlayer::resizeEvent(QResizeEvent *event)
{
}

void QSwfPlayer::loadSwf(QString& filename)
{
    if (_loaded) {
        return;
    }

    QFileInfo fi(filename);
    QString file = QString("file://") + fi.canonicalFilePath();

    QString buf(templ);
    buf = buf.arg(this->width()).arg(this->height()).arg(file);

    setHtml(buf);

    page()->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    _loaded = true;
}

