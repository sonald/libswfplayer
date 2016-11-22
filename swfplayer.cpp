#include <QtWebKitWidgets>
#include "swfplayer.h"

#include <zlib.h>

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
        <style>
            body {
                background-color: lightblue;
                margin: 0;
            }
        </style>
    </head>
    <body>
        <div id="player">
            <h1>Alternative content</h1>
        </div>
        <script type="text/javascript">
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

SwfFileInfo SwfFileInfo::parseSwfFile(const QString& file)
{
    SwfFileInfo sfi;
    sfi.valid = false;

    QFile f(file);
    if (!f.open(QIODevice::ReadOnly))
        return sfi;

    /*
     * uint32_t signature: 24;     // (F|C|Z)WS
     * uint32_t version: 8;
     * uint32_t length; // file length
     * RECT frameSize; // Frame size in twips
     * uint16_t frameRate;
     * uint16_t frameCount;
     */
    char buf[1024];
    f.read(buf, sizeof buf);

    switch (buf[0]) {
        case 'F':
        case 'C':
        case 'Z': {
            if (buf[1] != 'W' || buf[2] != 'S') {
                break;
            }
            memcpy(sfi.sig, buf, 3);
            sfi.sig[3] = 0;
            sfi.version = buf[3];
            sfi.length = *((int*)(buf+4));
            sfi.compressed = buf[0] == 'C' || buf[0] == 'Z';

            if (sfi.compressed) {
                //assume zlib decompress

                char out[sizeof buf - 8];
                z_stream stream;
                stream.zalloc = Z_NULL;
                stream.zfree = Z_NULL;
                stream.opaque = Z_NULL;

                stream.avail_in = 0;
                stream.next_in = Z_NULL;

                int ret = inflateInit(&stream);
                if (ret != Z_OK) {
                    qDebug() << "inflate failed";
                    return sfi;
                }

                stream.avail_in = sizeof buf - 8;
                stream.next_in = (Bytef*)(buf + 8);

                stream.next_out = (Bytef*)out;
                stream.avail_out = sizeof out;
                ret = inflate(&stream, Z_SYNC_FLUSH);
                if (ret != Z_OK)
                    qDebug() << "inflate error";

                memcpy(buf + 8, out, sizeof buf - 8);
                inflateEnd(&stream);
            }

            int64_t l = *(int64_t*)(buf+8);
            int nbits = (buf[8] & 0x1f);
            int off = 5;
            int xmin = (l >> off) & ((1<<nbits) - 1);

            l = *(int64_t*)&buf[8 + (5+nbits)/8];
            off = (5+nbits)%8;
            int xmax = (l >> off) & ((1<<nbits) - 1);

            l = *(int64_t*)&buf[8 + (5+nbits*2)/8];
            off = (5+nbits*2)%8;
            int ymin = (l >> off) & ((1<<nbits) - 1);

            l = *(int64_t*)&buf[8 + (5+nbits*3)/8];
            off = (5+nbits*3)%8;
            int ymax = (l >> off) & ((1<<nbits) - 1);

            sfi.width = xmax - xmin;
            sfi.height = ymax - ymin;

            qDebug() << sfi.sig << sfi.version << sfi.length 
                << "nbits = " << nbits 
                << xmin << xmax << ymin << ymax 
                << "w = " << sfi.width << "h = " << sfi.height;
            sfi.valid = true;
        }

        default:
            break;
    }

    f.close();
    return sfi;
}

QSwfPlayer::QSwfPlayer(QWidget* parent)
    : QWebView(parent),
    _loaded(false)
{
    if (settings()) {
        settings()->setAttribute(QWebSettings::PluginsEnabled, true);
        settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    }
}

QVariant QSwfPlayer::eval(const QString& script)
{
    if (page() && page()->mainFrame())
        return page()->mainFrame()->evaluateJavaScript(script);

    return QVariant();
}

void QSwfPlayer::play()
{
    eval("Play()");
}

void QSwfPlayer::stop()
{
    eval("Stop()");
}

void QSwfPlayer::pause()
{
    eval("Pause()");
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
    return QWebView::resizeEvent(event);
}

void QSwfPlayer::onLoadFinished(bool ok)
{
    _loaded = true;
    qDebug() << __func__;

    QObject::disconnect(this, &QWebView::loadFinished, this, &QSwfPlayer::onLoadFinished);

    page()->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);

    //auto *frm = page()->mainFrame();
    //frm->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    //frm->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
    
    eval("adjustSize()");
}

void QSwfPlayer::loadSwf(QString& filename)
{
    if (_loaded) {
        return;
    }

    QFileInfo fi(filename);
    QString file = QString("file://") + fi.canonicalFilePath();

    auto sfi = SwfFileInfo::parseSwfFile(filename);

    QString buf(templ);
    buf = buf.arg(this->width()).arg(this->height()).arg(file);

    QObject::connect(this, &QWebView::loadFinished, this, &QSwfPlayer::onLoadFinished);
    setHtml(buf);
}

