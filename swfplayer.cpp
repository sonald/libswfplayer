#include <QtWebKit>
#include "swfplayer.h"

#include <vector>
#include <map>
#include <set>

#include <zlib.h>
#include <libffmpegthumbnailer/videothumbnailer.h>

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
 */

static const char templ[] = R"(
<html lang="en">
    <head>
        <title>Player</title>
        <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
        <script type="text/javascript" src="%4"></script>
        <style>
            html, body {
               height: 100%;
               width: 100%;
               margin: 0;
               padding: 0;
            }

            #player {
                margin: 0;
                position: absolute;
            }

            body {
                background-color: lightblue;
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
              //document.getElementById('player').GotoFrame(1);
              document.getElementById('player').Rewind();
          }

          function adjustSize() {
              var $obj = document.getElementById('player');
              // $obj.width = document.documentElement.clientWidth;
              // $obj.height = document.documentElement.clientHeight;
              $obj.style.position = "absolute";
              $obj.style.left = (document.documentElement.clientWidth - $obj.width)/2;
              $obj.style.top = (document.documentElement.clientHeight - $obj.height)/2;
          }
          window.addEventListener('resize', adjustSize);

          swfobject.embedSWF("%3", "player", "%1", "%2", "9.0.0");
        </script>
    </body>
</html>
)";

static const char swfobject[] = R"(
var swfobject=function(){var D="undefined",r="object",T="Shockwave Flash",Z="ShockwaveFlash.ShockwaveFlash",q="application/x-shockwave-flash",S="SWFObjectExprInst",x="onreadystatechange",Q=window,h=document,t=navigator,V=false,X=[],o=[],P=[],K=[],I,p,E,B,L=false,a=false,m,G,j=true,l=false,O=function(){var ad=typeof h.getElementById!=D&&typeof h.getElementsByTagName!=D&&typeof h.createElement!=D,ak=t.userAgent.toLowerCase(),ab=t.platform.toLowerCase(),ah=ab?/win/.test(ab):/win/.test(ak),af=ab?/mac/.test(ab):/mac/.test(ak),ai=/webkit/.test(ak)?parseFloat(ak.replace(/^.*webkit\/(\d+(\.\d+)?).*$/,"$1")):false,aa=t.appName==="Microsoft Internet Explorer",aj=[0,0,0],ae=null;if(typeof t.plugins!=D&&typeof t.plugins[T]==r){ae=t.plugins[T].description;if(ae&&(typeof t.mimeTypes!=D&&t.mimeTypes[q]&&t.mimeTypes[q].enabledPlugin)){V=true;aa=false;ae=ae.replace(/^.*\s+(\S+\s+\S+$)/,"$1");aj[0]=n(ae.replace(/^(.*)\..*$/,"$1"));aj[1]=n(ae.replace(/^.*\.(.*)\s.*$/,"$1"));aj[2]=/[a-zA-Z]/.test(ae)?n(ae.replace(/^.*[a-zA-Z]+(.*)$/,"$1")):0}}else{if(typeof Q.ActiveXObject!=D){try{var ag=new ActiveXObject(Z);if(ag){ae=ag.GetVariable("$version");if(ae){aa=true;ae=ae.split(" ")[1].split(",");aj=[n(ae[0]),n(ae[1]),n(ae[2])]}}}catch(ac){}}}return{w3:ad,pv:aj,wk:ai,ie:aa,win:ah,mac:af}}(),i=function(){if(!O.w3){return}if((typeof h.readyState!=D&&(h.readyState==="complete"||h.readyState==="interactive"))||(typeof h.readyState==D&&(h.getElementsByTagName("body")[0]||h.body))){f()}if(!L){if(typeof h.addEventListener!=D){h.addEventListener("DOMContentLoaded",f,false)}if(O.ie){h.attachEvent(x,function aa(){if(h.readyState=="complete"){h.detachEvent(x,aa);f()}});if(Q==top){(function ac(){if(L){return}try{h.documentElement.doScroll("left")}catch(ad){setTimeout(ac,0);return}f()}())}}if(O.wk){(function ab(){if(L){return}if(!/loaded|complete/.test(h.readyState)){setTimeout(ab,0);return}f()}())}}}();function f(){if(L||!document.getElementsByTagName("body")[0]){return}try{var ac,ad=C("span");ad.style.display="none";ac=h.getElementsByTagName("body")[0].appendChild(ad);ac.parentNode.removeChild(ac);ac=null;ad=null}catch(ae){return}L=true;var aa=X.length;for(var ab=0;ab<aa;ab++){X[ab]()}}function M(aa){if(L){aa()}else{X[X.length]=aa}}function s(ab){if(typeof Q.addEventListener!=D){Q.addEventListener("load",ab,false)}else{if(typeof h.addEventListener!=D){h.addEventListener("load",ab,false)}else{if(typeof Q.attachEvent!=D){g(Q,"onload",ab)}else{if(typeof Q.onload=="function"){var aa=Q.onload;Q.onload=function(){aa();ab()}}else{Q.onload=ab}}}}}function Y(){var aa=h.getElementsByTagName("body")[0];var ae=C(r);ae.setAttribute("style","visibility: hidden;");ae.setAttribute("type",q);var ad=aa.appendChild(ae);if(ad){var ac=0;(function ab(){if(typeof ad.GetVariable!=D){try{var ag=ad.GetVariable("$version");if(ag){ag=ag.split(" ")[1].split(",");O.pv=[n(ag[0]),n(ag[1]),n(ag[2])]}}catch(af){O.pv=[8,0,0]}}else{if(ac<10){ac++;setTimeout(ab,10);return}}aa.removeChild(ae);ad=null;H()}())}else{H()}}function H(){var aj=o.length;if(aj>0){for(var ai=0;ai<aj;ai++){var ab=o[ai].id;var ae=o[ai].callbackFn;var ad={success:false,id:ab};if(O.pv[0]>0){var ah=c(ab);if(ah){if(F(o[ai].swfVersion)&&!(O.wk&&O.wk<312)){w(ab,true);if(ae){ad.success=true;ad.ref=z(ab);ad.id=ab;ae(ad)}}else{if(o[ai].expressInstall&&A()){var al={};al.data=o[ai].expressInstall;al.width=ah.getAttribute("width")||"0";al.height=ah.getAttribute("height")||"0";if(ah.getAttribute("class")){al.styleclass=ah.getAttribute("class")}if(ah.getAttribute("align")){al.align=ah.getAttribute("align")}var ak={};var aa=ah.getElementsByTagName("param");var af=aa.length;for(var ag=0;ag<af;ag++){if(aa[ag].getAttribute("name").toLowerCase()!="movie"){ak[aa[ag].getAttribute("name")]=aa[ag].getAttribute("value")}}R(al,ak,ab,ae)}else{b(ah);if(ae){ae(ad)}}}}}else{w(ab,true);if(ae){var ac=z(ab);if(ac&&typeof ac.SetVariable!=D){ad.success=true;ad.ref=ac;ad.id=ac.id}ae(ad)}}}}}X[0]=function(){if(V){Y()}else{H()}};function z(ac){var aa=null,ab=c(ac);if(ab&&ab.nodeName.toUpperCase()==="OBJECT"){if(typeof ab.SetVariable!==D){aa=ab}else{aa=ab.getElementsByTagName(r)[0]||ab}}return aa}function A(){return !a&&F("6.0.65")&&(O.win||O.mac)&&!(O.wk&&O.wk<312)}function R(ad,ae,aa,ac){var ah=c(aa);aa=W(aa);a=true;E=ac||null;B={success:false,id:aa};if(ah){if(ah.nodeName.toUpperCase()=="OBJECT"){I=J(ah);p=null}else{I=ah;p=aa}ad.id=S;if(typeof ad.width==D||(!/%$/.test(ad.width)&&n(ad.width)<310)){ad.width="310"}if(typeof ad.height==D||(!/%$/.test(ad.height)&&n(ad.height)<137)){ad.height="137"}var ag=O.ie?"ActiveX":"PlugIn",af="MMredirectURL="+encodeURIComponent(Q.location.toString().replace(/&/g,"%26"))+"&MMplayerType="+ag+"&MMdoctitle="+encodeURIComponent(h.title.slice(0,47)+" - Flash Player Installation");if(typeof ae.flashvars!=D){ae.flashvars+="&"+af}else{ae.flashvars=af}if(O.ie&&ah.readyState!=4){var ab=C("div");
aa+="SWFObjectNew";ab.setAttribute("id",aa);ah.parentNode.insertBefore(ab,ah);ah.style.display="none";y(ah)}u(ad,ae,aa)}}function b(ab){if(O.ie&&ab.readyState!=4){ab.style.display="none";var aa=C("div");ab.parentNode.insertBefore(aa,ab);aa.parentNode.replaceChild(J(ab),aa);y(ab)}else{ab.parentNode.replaceChild(J(ab),ab)}}function J(af){var ae=C("div");if(O.win&&O.ie){ae.innerHTML=af.innerHTML}else{var ab=af.getElementsByTagName(r)[0];if(ab){var ag=ab.childNodes;if(ag){var aa=ag.length;for(var ad=0;ad<aa;ad++){if(!(ag[ad].nodeType==1&&ag[ad].nodeName=="PARAM")&&!(ag[ad].nodeType==8)){ae.appendChild(ag[ad].cloneNode(true))}}}}}return ae}function k(aa,ab){var ac=C("div");ac.innerHTML="<object classid='clsid:D27CDB6E-AE6D-11cf-96B8-444553540000'><param name='movie' value='"+aa+"'>"+ab+"</object>";return ac.firstChild}function u(ai,ag,ab){var aa,ad=c(ab);ab=W(ab);if(O.wk&&O.wk<312){return aa}if(ad){var ac=(O.ie)?C("div"):C(r),af,ah,ae;if(typeof ai.id==D){ai.id=ab}for(ae in ag){if(ag.hasOwnProperty(ae)&&ae.toLowerCase()!=="movie"){e(ac,ae,ag[ae])}}if(O.ie){ac=k(ai.data,ac.innerHTML)}for(af in ai){if(ai.hasOwnProperty(af)){ah=af.toLowerCase();if(ah==="styleclass"){ac.setAttribute("class",ai[af])}else{if(ah!=="classid"&&ah!=="data"){ac.setAttribute(af,ai[af])}}}}if(O.ie){P[P.length]=ai.id}else{ac.setAttribute("type",q);ac.setAttribute("data",ai.data)}ad.parentNode.replaceChild(ac,ad);aa=ac}return aa}function e(ac,aa,ab){var ad=C("param");ad.setAttribute("name",aa);ad.setAttribute("value",ab);ac.appendChild(ad)}function y(ac){var ab=c(ac);if(ab&&ab.nodeName.toUpperCase()=="OBJECT"){if(O.ie){ab.style.display="none";(function aa(){if(ab.readyState==4){for(var ad in ab){if(typeof ab[ad]=="function"){ab[ad]=null}}ab.parentNode.removeChild(ab)}else{setTimeout(aa,10)}}())}else{ab.parentNode.removeChild(ab)}}}function U(aa){return(aa&&aa.nodeType&&aa.nodeType===1)}function W(aa){return(U(aa))?aa.id:aa}function c(ac){if(U(ac)){return ac}var aa=null;try{aa=h.getElementById(ac)}catch(ab){}return aa}function C(aa){return h.createElement(aa)}function n(aa){return parseInt(aa,10)}function g(ac,aa,ab){ac.attachEvent(aa,ab);K[K.length]=[ac,aa,ab]}function F(ac){ac+="";var ab=O.pv,aa=ac.split(".");aa[0]=n(aa[0]);aa[1]=n(aa[1])||0;aa[2]=n(aa[2])||0;return(ab[0]>aa[0]||(ab[0]==aa[0]&&ab[1]>aa[1])||(ab[0]==aa[0]&&ab[1]==aa[1]&&ab[2]>=aa[2]))?true:false}function v(af,ab,ag,ae){var ad=h.getElementsByTagName("head")[0];if(!ad){return}var aa=(typeof ag=="string")?ag:"screen";if(ae){m=null;G=null}if(!m||G!=aa){var ac=C("style");ac.setAttribute("type","text/css");ac.setAttribute("media",aa);m=ad.appendChild(ac);if(O.ie&&typeof h.styleSheets!=D&&h.styleSheets.length>0){m=h.styleSheets[h.styleSheets.length-1]}G=aa}if(m){if(typeof m.addRule!=D){m.addRule(af,ab)}else{if(typeof h.createTextNode!=D){m.appendChild(h.createTextNode(af+" {"+ab+"}"))}}}}function w(ad,aa){if(!j){return}var ab=aa?"visible":"hidden",ac=c(ad);if(L&&ac){ac.style.visibility=ab}else{if(typeof ad==="string"){v("#"+ad,"visibility:"+ab)}}}function N(ab){var ac=/[\\\"<>\.;]/;var aa=ac.exec(ab)!=null;return aa&&typeof encodeURIComponent!=D?encodeURIComponent(ab):ab}var d=function(){if(O.ie){window.attachEvent("onunload",function(){var af=K.length;for(var ae=0;ae<af;ae++){K[ae][0].detachEvent(K[ae][1],K[ae][2])}var ac=P.length;for(var ad=0;ad<ac;ad++){y(P[ad])}for(var ab in O){O[ab]=null}O=null;for(var aa in swfobject){swfobject[aa]=null}swfobject=null})}}();return{registerObject:function(ae,aa,ad,ac){if(O.w3&&ae&&aa){var ab={};ab.id=ae;ab.swfVersion=aa;ab.expressInstall=ad;ab.callbackFn=ac;o[o.length]=ab;w(ae,false)}else{if(ac){ac({success:false,id:ae})}}},getObjectById:function(aa){if(O.w3){return z(aa)}},embedSWF:function(af,al,ai,ak,ab,ae,ad,ah,aj,ag){var ac=W(al),aa={success:false,id:ac};if(O.w3&&!(O.wk&&O.wk<312)&&af&&al&&ai&&ak&&ab){w(ac,false);M(function(){ai+="";ak+="";var an={};if(aj&&typeof aj===r){for(var aq in aj){an[aq]=aj[aq]}}an.data=af;an.width=ai;an.height=ak;var ar={};if(ah&&typeof ah===r){for(var ao in ah){ar[ao]=ah[ao]}}if(ad&&typeof ad===r){for(var am in ad){if(ad.hasOwnProperty(am)){var ap=(l)?encodeURIComponent(am):am,at=(l)?encodeURIComponent(ad[am]):ad[am];if(typeof ar.flashvars!=D){ar.flashvars+="&"+ap+"="+at}else{ar.flashvars=ap+"="+at}}}}if(F(ab)){var au=u(an,ar,al);if(an.id==ac){w(ac,true)}aa.success=true;aa.ref=au;aa.id=au.id}else{if(ae&&A()){an.data=ae;R(an,ar,al,ag);return}else{w(ac,true)}}if(ag){ag(aa)}})}else{if(ag){ag(aa)}}},switchOffAutoHideShow:function(){j=false},enableUriEncoding:function(aa){l=(typeof aa===D)?true:aa},ua:O,getFlashPlayerVersion:function(){return{major:O.pv[0],minor:O.pv[1],release:O.pv[2]}},hasFlashPlayerVersion:F,createSWF:function(ac,ab,aa){if(O.w3){return u(ac,ab,aa)}else{return undefined}},showExpressInstall:function(ac,ad,aa,ab){if(O.w3&&A()){R(ac,ad,aa,ab)}},removeSWF:function(aa){if(O.w3){y(aa)}},createCSS:function(ad,ac,ab,aa){if(O.w3){v(ad,ac,ab,aa)}},addDomLoadEvent:M,addLoadEvent:s,getQueryParamValue:function(ad){var ac=h.location.search||h.location.hash;
if(ac){if(/\?/.test(ac)){ac=ac.split("?")[1]}if(ad==null){return N(ac)}var ab=ac.split("&");for(var aa=0;aa<ab.length;aa++){if(ab[aa].substring(0,ab[aa].indexOf("="))==ad){return N(ab[aa].substring((ab[aa].indexOf("=")+1)))}}}return""},expressInstallCallback:function(){if(a){var aa=c(S);if(aa&&I){aa.parentNode.replaceChild(I,aa);if(p){w(p,true);if(O.ie){I.style.display="block"}}if(E){E(B)}}a=false}},version:"2.3"}}();
)";

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

    //if (val&(1<<(bits-1))) val|=(0xffffffff<<bits);  
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
    char *output = mktemp(file_tmpl);
    qDebug() << "getThumbnailByGnash " << output;
    QString cmdline = tmpl.arg(output).arg(file).arg(sfi->width).arg(sfi->height);
    qDebug() << cmdline;

    QProcess gnash;
    gnash.start(cmdline);
    if (!gnash.waitForStarted()) 
        return img;

    if (!gnash.waitForFinished(5000))
        return img;

    img = QImage(output);
    //img.save("output.png");

    unlink(output);
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
    _swfInfo(NULL)
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
    eval("adjustSize()");
    //qDebug() << __func__;
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

        QString jspath = "swfobject.js";
        QFileInfo jsinfo(jspath);
        jspath = QString("file://") + jsinfo.canonicalFilePath();

        if (!jsinfo.exists()) {
            _jstf.setAutoRemove(true);
            if (_jstf.open()) {
                _jstf.write(swfobject, strlen(swfobject));
                _jstf.close();
                jspath = QString("file://") + _jstf.fileName();
                qDebug() << "create temp js file " << jspath;
            }
        }

        QString buf(templ);
        buf = buf.arg(preferedSize().width()).arg(preferedSize().height()).arg(file).arg(jspath);

        QObject::connect(this, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));
        setHtml(buf);
    }
}

void QSwfPlayer::hideEvent(QHideEvent *event)
{
    qDebug() << __func__;
    QWidget::hideEvent(event);
}

void QSwfPlayer::closeEvent(QCloseEvent *event)
{
    if (_loaded) {
        this->settings()->clearMemoryCaches();
        setHtml("");
    }
    QWidget::closeEvent(event);
}
