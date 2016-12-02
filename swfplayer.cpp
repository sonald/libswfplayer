#include <QtWebKit>
#include "swfplayer.h"

#include <vector>
#include <map>
#include <set>

#include <zlib.h>
//#include <libffmpegthumbnailer/videothumbnailer.h>
#include "videothumbnailer.h"

namespace ff = ffmpegthumbnailer;

static std::map<int, std::string> tag_videos = {
    {60, "VideoStream"},
    {61, "VideoFrame"},
};

static std::map<int, std::string> tag_images = {
    {6, "DefineBits"},
    {21, "DefineBitsJPEG2"},
    {35, "DefineBitsJPEG3"},
    {90, "DefineBitsJPEG4"},
    {20, "DefineBitsLossless"},
    {36, "DefineBitsLossless2"},
};

typedef struct SwfTag {
    unsigned type;
    unsigned int len;
} SwfTag;

struct SwfFileInfo {
    QString filename;

    char sig[4];
    int version;
    int length;
    bool compressed;

    int width;
    int height;

    int frameRate;
    int frameCount;

    bool valid;

    QImage thumb;

    QVector<SwfTag> tags;
                    
    // guessed from tags
    bool containsVideo;
    bool containsImage;

    static SwfFileInfo* parseSwfFile(const QString& file);
};

/**
 * mms.cfg: https://gist.github.com/orumin/6365218
 *
 * note: wmode: opaque make sure scaling correct (direct/window won't)
 * <param name="wmode" value="opaque" />
 */

static const char templ[] = R"(
<html lang="en">
    <head>
        <title>Player</title>
        <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
        <script type="text/javascript" src="%2"></script>
        <style>
            html, body {
               height: 100%;
               width: 100%;
               margin: 0;
               padding: 0;
            }

            #player {
                margin: 0;
            }

            body {
                background-color: lightblue;
            }

        </style>
    </head>
    <body>
        <object id="player" type="application/x-shockwave-flash" width="100%" height="100%">
            <param name="movie" value="%1" />
            <param name="quality" value="high" />
            <param name="loop" value="false" />
            <param name="play" value="false" />
            <param name="menu" value="false" />
            <param name="allowFullScreen" value="false" />
            <param name="wmode" value="opaque" />
        </object>


        <script type="text/javascript">
          function Play(){
              document.getElementById('player').Play();
          }

          function Pause(){
              document.getElementById('player').StopPlay();
          }

          function Stop(){
              //document.getElementById('player').GotoFrame(1);
              document.getElementById('player').Rewind();
          }

          function adjustSize() {
              var $obj = document.getElementById('player');
              $obj.style.position = "absolute";
              $obj.style.left = (document.documentElement.clientWidth - $obj.width)/2;
              $obj.style.top = (document.documentElement.clientHeight - $obj.height)/2;
          }
          //window.addEventListener('resize', adjustSize);
        </script>
    </body>
</html>
)";

// swf parsing helper
static int getInt(const unsigned char* buf, int bits, int off)
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

    return val;
}

static unsigned short getUI16(const unsigned char *buf)
{
    unsigned short v = *buf++;
    v |= *buf << 8;
    return v;
}

static unsigned int getUI32(const unsigned char *buf)
{
    unsigned int v = *buf++;
    v |= (*buf++ << 8);
    v |= (*buf++ << 16);
    v |= (*buf << 24);
    return v;
}

static QImage getThumbnailByGnash(const SwfFileInfo* sfi, const QString& file)
{
    if (!QFile::exists("/usr/bin/dump-gnash")) {
        return QImage();
    }

    QString tmpl("dump-gnash --screenshot last --screenshot-file %1 \"%2\""
            " --max-advances=20 --timeout=10 --width=%3 --height=%4 -r1");
    QImage img;

    char file_tmpl[] = ("/tmp/jinshan.XXXXXX");
    int tmpfd = mkstemp(file_tmpl);
    close(tmpfd);
    qDebug() << "getThumbnailByGnash " << file_tmpl;
    QString cmdline = tmpl.arg(file_tmpl).arg(file).arg(sfi->width).arg(sfi->height);
    qDebug() << cmdline;

    QProcess gnash;
    gnash.start(cmdline);
    if (!gnash.waitForStarted()) 
        return img;

    if (!gnash.waitForFinished(5000))
        return img;

    img = QImage(file_tmpl);
    //img.save("output.png");

    unlink(file_tmpl);
    return img;
}

static QImage getThumbnailFromSwf(const SwfFileInfo* sfi, const QString& file)
{
    int sz = sfi->width;

    if (!(sfi->containsImage || sfi->containsVideo)) {
        QImage img = getThumbnailByGnash(sfi, file);
        if (!img.isNull()) {
            return img;
        }
    }

    try {
        ff::VideoThumbnailer videoThumbnailer(sz, false, true, 8, false);
        videoThumbnailer.setLogCallback([] (ThumbnailerLogLevel lvl, const std::string& msg) {
                if (lvl == ThumbnailerLogLevelInfo)
                qDebug() << msg.c_str();
                else
                qDebug() << msg.c_str();
                });

        videoThumbnailer.setSeekPercentage(5);

        QImage img;
        QTemporaryFile tf;
        if (tf.open()) {
            qDebug() << "img " << tf.fileName();
            std::string infile = file.toStdString();
            std::string outfile = tf.fileName().toStdString();
            videoThumbnailer.generateThumbnail(infile, Png, outfile);
            //videoThumbnailer.generateThumbnail(file.toStdString(), Png, tf.fileName().toStdString());

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
            
static void parseTags(SwfFileInfo& sfi, const unsigned char *buf, const unsigned char* end)
{
    int count = 0;

    static std::set<int> forces = {
        21
    };

    while (end - buf > 0 && count++ < 100) {
        unsigned short tagAndLen = getUI16(buf);
        buf += 2;
        unsigned  id = tagAndLen >> 6;
        unsigned int len = tagAndLen & 0x3f;
        if (len == 63 || forces.find(id) != forces.end()) {
            len = getUI32(buf);
            buf += 4;
        }

        sfi.tags.push_back((SwfTag){id, len});
        //qDebug() << "Tag: type " << id << "len " << len;
        buf += len; // bypass data
    }
}

SwfFileInfo* SwfFileInfo::parseSwfFile(const QString& file)
{
    SwfFileInfo *sfi = new SwfFileInfo;
    sfi->valid = false;
    sfi->filename = file;

    QFile f(file);
    if (!f.open(QIODevice::ReadOnly))
        return sfi;

    /*
     * uint32_t signature: 24;     // (F|C|Z)WS
     * uint32_t version: 8;
     * uint32_t length; // file length
     * below maybe compressed
     * RECT frameSize; // Frame size in twips
     * uint16_t frameRate;
     * uint16_t frameCount;
     */
    int bufsz = 1048576;
    unsigned char buf[bufsz];
    f.read((char*)buf, sizeof buf);

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
                unsigned char out[sizeof buf - 8];
                uLongf outlen = sizeof out;
                uncompress ((Bytef*)out, &outlen, (Bytef*)(buf + 8), sizeof buf - 8);

                memcpy(buf + 8, out, sizeof buf - 8);
            }

            unsigned char *bufnext = buf + 8;

            int nbits = getInt(bufnext, 5, 0);
            int off = 5;

            int xmin = getInt(bufnext, nbits, off);
            int xmax = getInt(bufnext+(5+nbits)/8, nbits, (5+nbits)%8);
            int ymin = getInt(bufnext+(5+nbits*2)/8, nbits, (5+nbits*2)%8);
            int ymax = getInt(bufnext+(5+nbits*3)/8, nbits, (5+nbits*3)%8);
            
            bufnext += (5+nbits*3)/8 + 3;
            sfi->frameRate = getUI16(bufnext);
            sfi->frameRate >>= 8;
            sfi->frameCount = getUI16(bufnext+2);
            bufnext += 4;

            sfi->width = (xmax - xmin)/20;
            sfi->height = (ymax - ymin)/20;

            qDebug() << sfi->sig << sfi->version << sfi->length 
                << "nbits = " << (int)nbits 
                << xmin << xmax << ymin << ymax 
                << "w = " << sfi->width << "h = " << sfi->height
                << "rate = " << sfi->frameRate << "count = " << sfi->frameCount;

            const unsigned char *end = buf + bufsz;
            size_t offset = bufnext - buf;
            if (end - bufnext > sfi->length) {
                end = bufnext + sfi->length - offset;
            }
            parseTags(*sfi, bufnext, end);

            sfi->containsVideo = false;
            sfi->containsImage = false;

            Q_FOREACH(const SwfTag& t, sfi->tags) {
                //qDebug() << "check tag " << t.type;
                if (tag_videos.find(t.type) != tag_videos.end()) {
                    sfi->containsVideo = true;
                }

                if (tag_images.find(t.type) != tag_images.end()) {
                    sfi->containsImage = true;
                }
            }

            if (sfi->containsVideo) {
                qDebug() << "contains video stream";
            }

            if (sfi->containsImage) {
                qDebug() << "contains image";
            }
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
    _swfInfo(NULL),
    _enableDebug(false)
{
    if (settings()) {
        settings()->setAttribute(QWebSettings::PluginsEnabled, true);
        settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
        settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
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
    QObject::disconnect(this, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));

    page()->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);

    QWebFrame *frm = page()->mainFrame();
    if (frm) {
        frm->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
        frm->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
    }
    
    _loaded = true;
    if (_enableDebug) qDebug() << __func__;
}

void QSwfPlayer::loadSwf(QString& filename)
{
    if (_loaded) {
        setHtml("");
    }
    //reset states
    _loaded = false;
    _state = QSwfPlayer::Invalid;

    QFileInfo fi(filename);
    QString file = QString("file://") + fi.canonicalFilePath();

    _swfInfo = SwfFileInfo::parseSwfFile(filename);
    if (_swfInfo->valid) {
        _state = QSwfPlayer::Loaded;
        _preferedSize = QSize(_swfInfo->width, _swfInfo->height);
    }
}

void QSwfPlayer::showEvent(QShowEvent *event)
{
    qDebug() << __func__;
    if (!_loaded && _state == QSwfPlayer::Loaded) {
        QFileInfo fi(_swfInfo->filename);
        QString file = QString("file://") + fi.canonicalFilePath();

        QString buf(templ);
        buf = buf.arg(file);

        QObject::connect(this, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));
        setHtml(buf);
    }

    QWebView::showEvent(event);
}

void QSwfPlayer::hideEvent(QHideEvent *event)
{
    qDebug() << __func__;
    QWebView::hideEvent(event);
}

void QSwfPlayer::closeEvent(QCloseEvent *event)
{
    if (_loaded) {
        if (_state == QSwfPlayer::Playing)
            this->stop();
        this->settings()->clearMemoryCaches();
    }
    QWebView::closeEvent(event);
}

void QSwfPlayer::enableDebug(bool val)
{
    if (_enableDebug == val) {
        return;
    }

    _enableDebug = val;
    settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, _enableDebug);
}

void QSwfPlayer::contextMenuEvent(QContextMenuEvent * event)
{
    qDebug() << __func__;
    if (_enableDebug) {
        QWebView::contextMenuEvent(event);
    }
}

