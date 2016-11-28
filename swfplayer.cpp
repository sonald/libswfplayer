#include <QtWebKit>
#include "swfplayer.h"

#include <vector>

#include <zlib.h>
#include <libffmpegthumbnailer/videothumbnailer.h>

namespace ff = ffmpegthumbnailer;

struct SwfFileInfo {
    char sig[4];
    char version;
    int length;
    bool compressed;

    int width;
    int height;

    int frameRate;
    int frameCount;

    bool valid;

    QImage thumb;

    static SwfFileInfo* parseSwfFile(const QString& file);
};

/**
 * mms.cfg: https://gist.github.com/orumin/6365218
 *
 */

static const char templ[] = R"(
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

static int getInt(const char* buf, int bits, int off)
{
    unsigned int val = 0;
    int pos = off;

    for (int i = 0; i < bits; i++) {
        val <<=1;
        int bit = (buf[(off + i)/8] >> (7-pos)) & 1;
        val |= bit;
        pos++;
        if (pos == 8) pos = 0;
    }

    //if (val&(1<<(bits-1))) val|=(0xffffffff<<bits);  
    return val;
}

static QImage getThumbnailByGnash(const SwfFileInfo* sfi, const QString& file)
{
    if (!QFile::exists("/usr/bin/dump-gnash")) {
        return QImage();
    }

    QString tmpl("dump-gnash --screenshot last --screenshot-file %1 \"%2\""
            " --max-advances=20 --timeout=100 --width=%3 --height=%4 -r1");
    QImage img;

    char file_tmpl[] = ("/tmp/jinshan.XXXXXX");
    char *output = mktemp(file_tmpl);
    qDebug() << "getThumbnailByGnash " << output;
    QString cmdline = tmpl.arg(output).arg(file).arg(sfi->width).arg(sfi->height);
    qDebug() << cmdline;

    QProcess gnash;
    gnash.start(cmdline);
    if (!gnash.waitForStarted()) 
        return img;

    if (!gnash.waitForFinished())
        return img;

    img = QImage(output);
    //img.save("output.png");

    unlink(output);
    return img;
}

static QImage getThumbnailFromSwf(const SwfFileInfo* sfi, const QString& file)
{
    int sz = sfi->width;

    try {
        ff::VideoThumbnailer videoThumbnailer(sz, false, true, 8, false);
        videoThumbnailer.setLogCallback([] (ThumbnailerLogLevel lvl, const std::string& msg) {
                if (lvl == ThumbnailerLogLevelInfo)
                qDebug() << msg.c_str();
                else
                qDebug() << msg.c_str();
                });

        videoThumbnailer.setSeekPercentage(10);

        QImage img;
        QTemporaryFile tf;
        if (tf.open()) {
            qDebug() << "img " << tf.fileName();
            videoThumbnailer.generateThumbnail(file.toStdString(), Png, tf.fileName().toStdString());

            img = QImage(tf.fileName());
        }
        //img.save("output.png");
        return img;
    } catch (std::exception& e) {
        qDebug() << "Error: " << e.what();

        {
            QImage img = getThumbnailByGnash(sfi, file);
            if (!img.isNull()) {
                return img;
            }
        }

        QImage blank(QSize(sfi->width, sfi->height), QImage::Format_RGB888);
        QPainter p(&blank);
        QRect r(QPoint(0, 0), QSize(sfi->width, sfi->height));
        p.fillRect(r, Qt::gray);
        p.setPen(Qt::red);
        p.drawText(r.center(), "SWF");
        p.end();
        return blank;
    }
}

SwfFileInfo* SwfFileInfo::parseSwfFile(const QString& file)
{
    SwfFileInfo *sfi = new SwfFileInfo;
    sfi->valid = false;

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
        case 'Z': 
            break;

        case 'F':
        case 'C': {
            if (buf[1] != 'W' || buf[2] != 'S') {
                break;
            }

            memcpy(sfi->sig, buf, 3);
            sfi->sig[3] = 0;
            sfi->version = buf[3];
            sfi->length = *((int*)(buf+4));
            sfi->compressed = buf[0] == 'C';

            if (sfi->compressed) {
                char out[sizeof buf - 8];
                uLongf outlen = sizeof out;
                uncompress ((Bytef*)out, &outlen, (Bytef*)(buf + 8), sizeof buf - 8);

                memcpy(buf + 8, out, sizeof buf - 8);
            }

            int nbits = getInt(buf+8, 5, 0);
            int off = 5;

            int xmin = getInt(buf+8, nbits, off);
            int xmax = getInt(buf+8+(5+nbits)/8, nbits, (5+nbits)%8);
            int ymin = getInt(buf+8+(5+nbits*2)/8, nbits, (5+nbits*2)%8);
            int ymax = getInt(buf+8+(5+nbits*3)/8, nbits, (5+nbits*3)%8);
            
            char *bufnext = buf + 8 + (5+nbits*3)/8 + 1;
            sfi->frameRate = (int)*bufnext;
            //sfi->frameCount = (int)*((short*)bufnext+1);

            sfi->width = (xmax - xmin)/20;
            sfi->height = (ymax - ymin)/20;

            qDebug() << sfi->sig << sfi->version << sfi->length 
                << "nbits = " << (int)nbits 
                << xmin << xmax << ymin << ymax 
                << "w = " << sfi->width << "h = " << sfi->height
                << "rate = " << sfi->frameRate << "count = " << sfi->frameCount;
            sfi->valid = true;
        }

        default:
            break;
    }

    f.close();

    sfi->thumb = getThumbnailFromSwf(sfi, file);

    return sfi;
}

QSwfPlayer::QSwfPlayer(QWidget* parent)
    : QWebView(parent),
    _loaded(false),
    _state(QSwfPlayer::Invalid),
    _swfInfo(NULL)
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
    if (_state == QSwfPlayer::Invalid) return;
    _state = QSwfPlayer::Playing;
    eval("Play()");
}

void QSwfPlayer::stop()
{
    if (_state == QSwfPlayer::Invalid) return;
    _state = QSwfPlayer::Loaded;
    eval("Stop()");
}

void QSwfPlayer::pause()
{
    if (_state == QSwfPlayer::Invalid) return;
    _state = QSwfPlayer::Paused;
    eval("Pause()");
}

void QSwfPlayer::grab(QString filepath)
{
    if (_state == QSwfPlayer::Invalid) return;

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

QImage QSwfPlayer::thumbnail() const
{
    if (_state != QSwfPlayer::Invalid) {
        return _swfInfo->thumb;
    }

    QImage blank(_preferedSize, QImage::Format_RGB888);
    QPainter p(&blank);
    QRect r(QPoint(0, 0), _preferedSize);
    p.fillRect(r, Qt::gray);
    p.setPen(Qt::red);
    p.drawText(r.center(), "SWF");
    p.end();

    return blank;
}

void QSwfPlayer::onLoadFinished(bool ok)
{
    _loaded = true;

    QObject::disconnect(this, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));

    page()->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);

    //auto *frm = page()->mainFrame();
    //frm->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    //frm->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
    
    eval("adjustSize()");
    qDebug() << __func__;
}

void QSwfPlayer::loadSwf(QString& filename)
{
    if (_loaded) {
        return;
    }

    QFileInfo fi(filename);
    QString file = QString("file://") + fi.canonicalFilePath();

    _swfInfo = SwfFileInfo::parseSwfFile(filename);
    if (_swfInfo->valid) {
        _state = QSwfPlayer::Loaded;
        _preferedSize = QSize(_swfInfo->width, _swfInfo->height);
        resize(_swfInfo->width, _swfInfo->height);
    }

    QString buf(templ);
    buf = buf.arg(this->width()).arg(this->height()).arg(file);

    QObject::connect(this, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));
    setHtml(buf);

    while (qApp->hasPendingEvents() && !_loaded) {
        qApp->processEvents();
    }
}

