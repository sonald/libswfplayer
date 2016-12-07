#ifdef interface
#undef interface
#endif
#include <QtWebKit>
#include "swfplayer.h"
#include <vector>
#include <map>
#include <set>
#include <zlib.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifndef PROC_TIMEOUT
#define PROC_TIMEOUT 3000
#endif

static std::map<int, std::string> s_tag_videos =
{
	{60, "VideoStream"},
	{61, "VideoFrame"},
};

static std::map<int, std::string> s_tag_images =
{
	{6, "DefineBits"},
	{21, "DefineBitsJPEG2"},
	{35, "DefineBitsJPEG3"},
	{90, "DefineBitsJPEG4"},
	{20, "DefineBitsLossless"},
	{36, "DefineBitsLossless2"},
};

typedef struct SwfTag
{
	unsigned nType;
	unsigned int nLen;
} SwfTag;

class KSwfFileInfo : public QObject
{
public:
	QString strFileName;
	char m_szSig[4];
	int m_nVersion;
	int m_nLength;
	bool m_bComPressed;
	int m_nWidth;
	int m_nHeight;
	int m_nFrameRate;
	int m_nFrameCount;
	bool m_bValid;
	QImage m_ImgThumb;
	QVector<SwfTag> m_vSwfTags;
	bool m_bContainsVideo;
	bool m_bContainsImage;

	static KSwfFileInfo* ParseSwfFile(const QString& strfile);
};

/**
 * mms.cfg: https://gist.github.com/orumin/6365218
 *
 * note: wmode: opaque make sure scaling correct (direct/window won't)
 * <param name="wmode" value="opaque" />
 */

static const char s_szPlayerScript[] = R"(
<html lang="en">
	<head>
		<title>Player</title>
		<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
		<script type="text/javascript" src="%2"></script>
		<style>
			html, body
			{
				height: 100%;
				width: 100%;
				margin: 0;
				padding: 0;
			}

			#player
			{
				margin: 0;
			}

			body
			{
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
			function Play()
			{
				document.getElementById('player').Play();
			}

			function Pause()
			{
				document.getElementById('player').StopPlay();
			}

			function Stop()
			{
				//document.getElementById('player').GotoFrame(1);
				document.getElementById('player').Rewind();
			}

			function adjustSize()
			{
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

static int execWithTimeout(const QStringList& cmdList, int timeout)
{
	int pid = fork();
	if (pid < 0)
		return -1;
	else if (pid > 0)
	{
		//parent
		QTime time;
		time.start();
		while (time.elapsed() < timeout && waitpid(pid, 0, WNOHANG) >= 0)
			usleep(100000);

		if (!kill(pid, 0))
			kill(pid, SIGKILL);
	}
	else
	{
		char* args[cmdList.size()+1];
		for (int i = 0; i < cmdList.size(); i++)
		{
			args[i] = strdup(cmdList[i].toUtf8().constData());
		}

		args[cmdList.size()] = NULL;
		execvp(args[0], args);
	}

	return 0;
}

static QImage getThumbnailByGnash(KSwfFileInfo* pSwfFileInfo, const QString& strFile)
{
	if (!QFile::exists("/usr/bin/dump-gnash"))
		return QImage();

	QTemporaryFile tmpFile;
	tmpFile.open();
	tmpFile.close();
	tmpFile.setAutoRemove(false);
	QString strTmpFile = tmpFile.fileName();

	QStringList cmdList;
	cmdList << "dump-gnash";
	cmdList << "--screenshot";
	cmdList << "last";
	cmdList << "--screenshot-file";
	cmdList << strTmpFile;
	cmdList << strFile;
	cmdList << "--max-advances=20";
	cmdList << "--timeout=10";
	cmdList << QString("--width=%1").arg(pSwfFileInfo->m_nWidth);
	cmdList << QString("--height=%1").arg(pSwfFileInfo->m_nHeight);
	cmdList << "-r1";

	cmdList.join(" ");

	QTime time;
	time.start();
	execWithTimeout(cmdList, PROC_TIMEOUT);
#if _DEBUG
	qDebug() << "elapsed: " << time.elapsed();
#endif
	QImage img = QImage(strTmpFile);
	remove(strTmpFile.toLocal8Bit().data());

	return img;
}

static QImage getThumbnailByFfmpeg(KSwfFileInfo* pSwfFileInfo, const QString& strFile)
{
	if (!QFile::exists("/usr/bin/ffmpegthumbnailer"))
		return QImage();

	QTemporaryFile tmpFile;
	tmpFile.open();
	tmpFile.close();
	tmpFile.setAutoRemove(false);
	QString strTmpFile = tmpFile.fileName();

	QStringList cmdList;
	cmdList << "ffmpegthumbnailer";
	cmdList << "-i";
	cmdList << strFile;
	cmdList << "-o";
	cmdList << strTmpFile;
	cmdList << "-s";
	cmdList << QString("%1").arg(pSwfFileInfo->m_nWidth);

	cmdList.join(" ");

	QTime time;
	time.start();
	execWithTimeout(cmdList, PROC_TIMEOUT);
#if _DEBUG
	qDebug() << "elapsed: " << time.elapsed();
#endif
	QImage img = QImage(strTmpFile);
	remove(strTmpFile.toLocal8Bit().data());

	return img;
}

static QImage getThumbnailFromSwf(KSwfFileInfo* pSwfFileInfo, const QString& strFile)
{
	QImage Img;

	if (pSwfFileInfo->m_bContainsImage || pSwfFileInfo->m_bContainsVideo)
	{
		Img = getThumbnailByFfmpeg(pSwfFileInfo, strFile);
		if (!Img.isNull())
			return Img;
	}

	Img = getThumbnailByGnash(pSwfFileInfo, strFile);
	if (!Img.isNull())
		return Img;

	QImage ImgBlank(QSize(pSwfFileInfo->m_nWidth, pSwfFileInfo->m_nHeight), QImage::Format_RGB888);
	QPainter paint(&ImgBlank);
	QRect rect(QPoint(0, 0), QSize(pSwfFileInfo->m_nWidth, pSwfFileInfo->m_nHeight));
	paint.fillRect(rect, Qt::gray);
	paint.setPen(Qt::red);
	paint.drawText(rect.center(), "SWF");
	paint.end();
	return ImgBlank;
}

static void parseTags(KSwfFileInfo& swfFileInfo, const unsigned char *buf, const unsigned char* end)
{
	int count = 0;

	static std::set<int> forces =
	{
		21
	};

	while (end - buf > 0 && count++ < 100)
	{
		unsigned short tagAndLen = getUI16(buf);
		buf += 2;
		unsigned  id = tagAndLen >> 6;
		unsigned int len = tagAndLen & 0x3f;
		if (len == 63 || forces.find(id) != forces.end())
		{
			len = getUI32(buf);
			buf += 4;
		}

		swfFileInfo.m_vSwfTags.push_back((SwfTag){id, len});
		buf += len; // bypass data
	}
}

KSwfFileInfo* KSwfFileInfo::ParseSwfFile(const QString& strFile)
{
	KSwfFileInfo *pSwfFileInfo = new KSwfFileInfo;
	if (!pSwfFileInfo)
		return NULL;

	pSwfFileInfo->m_bValid = false;
	pSwfFileInfo->strFileName = strFile;

	QFile file(strFile);
	if (!file.open(QIODevice::ReadOnly))
	{
		delete pSwfFileInfo;
		pSwfFileInfo = NULL;
		return NULL;
	}

	/*
	 * uint32_t signature: 24;     // (F|C|Z)WS
	 * uint32_t m_nVersion: 8;
	 * uint32_t m_nLength; // file m_nLength
	 * below maybe m_bComPressed
	 * RECT frameSize; // Frame size in twips
	 * uint16_t m_nFrameRate;
	 * uint16_t m_nFrameCount;
	 */
	int bufsz = 1048576;
	unsigned char buf[bufsz];
	file.read((char*)buf, sizeof buf);

	switch (buf[0]) {
		case 'Z':
			break;
		case 'F':
		case 'C':
		{
			if (buf[1] != 'W' || buf[2] != 'S')
				break;

			memcpy(pSwfFileInfo->m_szSig, buf, 3);
			pSwfFileInfo->m_szSig[3] = 0;
			pSwfFileInfo->m_nVersion = buf[3];
			pSwfFileInfo->m_nLength = *((int*)(buf+4));
			pSwfFileInfo->m_bComPressed = buf[0] == 'C';

			if (pSwfFileInfo->m_bComPressed)
			{
				unsigned char out[sizeof(buf) - 8];
				uLongf outlen = sizeof(out);
#ifdef KINGSOFT_MEDIAFLASH
				uLong uErrlv = 0;
				uncompress ((Bytef*)out, &outlen, (Bytef*)(buf + 8), sizeof(buf) - 8, uErrlv);
#else
				uncompress ((Bytef*)out, &outlen, (Bytef*)(buf + 8), sizeof(buf) - 8);
#endif
				memcpy(buf + 8, out, sizeof(buf) - 8);
			}

			unsigned char *bufnext = buf + 8;

			int nbits = getInt(bufnext, 5, 0);
			int off = 5;

			int xmin = getInt(bufnext, nbits, off);
			int xmax = getInt(bufnext+(5+nbits)/8, nbits, (5+nbits)%8);
			int ymin = getInt(bufnext+(5+nbits*2)/8, nbits, (5+nbits*2)%8);
			int ymax = getInt(bufnext+(5+nbits*3)/8, nbits, (5+nbits*3)%8);

			bufnext += (5+nbits*3)/8 + 3;
			pSwfFileInfo->m_nFrameRate = getUI16(bufnext);
			pSwfFileInfo->m_nFrameRate >>= 8;
			pSwfFileInfo->m_nFrameCount = getUI16(bufnext+2);
			bufnext += 4;

			pSwfFileInfo->m_nWidth = (xmax - xmin)/20;
			pSwfFileInfo->m_nHeight = (ymax - ymin)/20;
#if _DEBUG
			qDebug() << pSwfFileInfo->m_szSig << pSwfFileInfo->m_nVersion << pSwfFileInfo->m_nLength
							<< "nbits = " << (int)nbits
							<< xmin << xmax << ymin << ymax
							<< "w = " << pSwfFileInfo->m_nWidth << "h = " << pSwfFileInfo->m_nHeight
							<< "rate = " << pSwfFileInfo->m_nFrameRate << "count = " << pSwfFileInfo->m_nFrameCount;
#endif
			const unsigned char *end = buf + bufsz;
			size_t offset = bufnext - buf;
			if (end - bufnext > pSwfFileInfo->m_nLength)
			{
				end = bufnext + pSwfFileInfo->m_nLength - offset;
			}

			parseTags(*pSwfFileInfo, bufnext, end);

			pSwfFileInfo->m_bContainsVideo = false;
			pSwfFileInfo->m_bContainsImage = false;

			Q_FOREACH(const SwfTag& t, pSwfFileInfo->m_vSwfTags)
			{
				if (s_tag_videos.find(t.nType) != s_tag_videos.end())
					pSwfFileInfo->m_bContainsVideo = true;

				if (s_tag_images.find(t.nType) != s_tag_images.end())
					pSwfFileInfo->m_bContainsImage = true;
			}

#if _DEBUG
			if (pSwfFileInfo->m_bContainsVideo)
				qDebug() << "contains video stream";

			if (pSwfFileInfo->m_bContainsImage)
				qDebug() << "contains image";
#endif
			pSwfFileInfo->m_bValid = true;
		}

		default:
			break;
	}

	file.close();

	pSwfFileInfo->m_ImgThumb = getThumbnailFromSwf(pSwfFileInfo, strFile);

	return pSwfFileInfo;
}

KSwfPlayer::KSwfPlayer(QWidget* parent)
	: QWebView(parent),
	m_bLoaded(false),
	m_eSwfPlayerState(KSwfPlayer::Invalid),
	m_pSwfInfo(NULL),
	m_bEnableDebug(false)
{
	if (settings())
	{
		settings()->setAttribute(QWebSettings::PluginsEnabled, true);
		settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
		settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
	}
}

KSwfPlayer::~KSwfPlayer()
{
	if (m_pSwfInfo)
	{
		delete m_pSwfInfo;
		m_pSwfInfo = NULL;
	}
}

QVariant KSwfPlayer::Eval(const QString& script)
{
	if (page() && page()->mainFrame())
		return page()->mainFrame()->evaluateJavaScript(script);

	return QVariant();
}

void KSwfPlayer::Play()
{
	if (m_eSwfPlayerState == KSwfPlayer::Invalid)
		return;

	m_eSwfPlayerState = KSwfPlayer::Playing;
	Eval("Play()");
}

void KSwfPlayer::Stop()
{
	if (m_eSwfPlayerState == KSwfPlayer::Invalid)
		return;

	m_eSwfPlayerState = KSwfPlayer::Loaded;
	Eval("Stop()");
}

void KSwfPlayer::Pause()
{
	if (m_eSwfPlayerState == KSwfPlayer::Invalid)
		return;
	m_eSwfPlayerState = KSwfPlayer::Paused;
	Eval("Pause()");
}

void KSwfPlayer::Grab(QString strFilePath)
{
	if (m_eSwfPlayerState == KSwfPlayer::Invalid)
		return;

	QPixmap pixmap(this->size());
	this->render(&pixmap);

	if (strFilePath.isEmpty())
		strFilePath = "swfsnapshot.png";
	pixmap.save(strFilePath);
}

void KSwfPlayer::resizeEvent(QResizeEvent *event)
{
	return QWebView::resizeEvent(event);
}

QImage KSwfPlayer::ThumbNail() const
{
	if (m_eSwfPlayerState != KSwfPlayer::Invalid)
		return m_pSwfInfo->m_ImgThumb;

	QImage ImgBlank(m_SizePrefered, QImage::Format_RGB888);
	QPainter paint(&ImgBlank);
	QRect rect(QPoint(0, 0), m_SizePrefered);
	paint.fillRect(rect, Qt::gray);
	paint.setPen(Qt::red);
	paint.drawText(rect.center(), "SWF");
	paint.end();

	return ImgBlank;
}

void KSwfPlayer::OnLoadFinished(bool OnLoadFinished)
{
	QObject::disconnect(this, SIGNAL(loadFinished(bool)), this, SLOT(OnLoadFinished(bool)));

	page()->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
	page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);

	QWebFrame *pWebFrame = page()->mainFrame();
	if (pWebFrame)
	{
		pWebFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
		pWebFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
	}

	m_bLoaded = true;
	if (m_bEnableDebug)
		qDebug() << __func__;
}

void KSwfPlayer::LoadSwf(QString& strFileName)
{
	if (m_bLoaded)
		setHtml("");

	m_bLoaded = false;
	m_eSwfPlayerState = KSwfPlayer::Invalid;

	QFileInfo fileInfo(strFileName);
	QString strFile = QString("file://") + fileInfo.canonicalFilePath();

	m_pSwfInfo = KSwfFileInfo::ParseSwfFile(strFileName);
	if (m_pSwfInfo->m_bValid)
	{
		m_eSwfPlayerState = KSwfPlayer::Loaded;
		m_SizePrefered = QSize(m_pSwfInfo->m_nWidth, m_pSwfInfo->m_nHeight);
	}
}

void KSwfPlayer::showEvent(QShowEvent *event)
{
	if (m_bEnableDebug)
		qDebug() << __func__;
	if (!m_bLoaded && m_eSwfPlayerState == KSwfPlayer::Loaded)
	{
		QFileInfo fileInfo(m_pSwfInfo->strFileName);
		QString strFile = QString("file://") + fileInfo.canonicalFilePath();

		QString strTmpBuf(s_szPlayerScript);
		strTmpBuf = strTmpBuf.arg(strFile);

		QObject::connect(this, SIGNAL(loadFinished(bool)), this, SLOT(OnLoadFinished(bool)));
		setHtml(strTmpBuf);
	}

	QWebView::showEvent(event);
}

void KSwfPlayer::hideEvent(QHideEvent *event)
{
	QWebView::hideEvent(event);
}

void KSwfPlayer::closeEvent(QCloseEvent *event)
{
	if (m_bLoaded)
	{
		if (m_eSwfPlayerState == KSwfPlayer::Playing)
			this->Stop();
		this->settings()->clearMemoryCaches();
	}

	QWebView::closeEvent(event);
}

void KSwfPlayer::EnableDebug(bool bEnableDebug)
{
	if (m_bEnableDebug == bEnableDebug)
		return;

	m_bEnableDebug = bEnableDebug;
	settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, m_bEnableDebug);
}

void KSwfPlayer::contextMenuEvent(QContextMenuEvent * event)
{
	if (m_bEnableDebug)
		QWebView::contextMenuEvent(event);
}

bool KSwfPlayer::CheckPlugins()
{
	QByteArray chklist[] =
	{
		"ffmpegthumbnailer",
		"dump-gnash"
	};

	bool bRes[2] =
	{
		false,
		false
	};

	QByteArray ba = qgetenv("PATH");
	if (ba.isEmpty())
	{
		ba = QByteArray("/bin:/usr/bin:/usr/local/bin");
	}

	QList<QByteArray> pathsList = ba.split(':');
	for (int i = 0; i < pathsList.size(); i++)
	{
		for (int j = 0; j < 2; j++)
		{
			QString strPath = QString("%1/%2").arg(pathsList[i].data()).arg(chklist[j].data());
			QFileInfo fileInfo(strPath);
			if (fileInfo.exists() && fileInfo.isExecutable())
				bRes[j] = true;
		}
	}

	if (!bRes[0] || !bRes[1])
	{
#if _DEBUG
		for (int j = 0; j < 2; j++)
		{
			if (!bRes[j])
				qDebug() << QString("%1 is missing").arg(chklist[j].data());
		}
#endif
		return false;
	}

	//find libflashplugin
	QString strSearchPaths[] =
	{
		".mozilla/plugins",
		".netscape/plugins",
		"//System locations",
		"/usr/lib/browser/plugins",
		"/usr/local/lib/mozilla/plugins",
		"/usr/lib/firefox/plugins",
		"/usr/lib64/browser-plugins",
		"/usr/lib/browser-plugins",
		"/usr/lib/mozilla/plugins",
		"/usr/local/netscape/plugins",
		"/opt/mozilla/plugins",
		"/opt/mozilla/lib/plugins",
		"/opt/netscape/plugins",
		"/opt/netscape/communicator/plugins",
		"/usr/lib/netscape/plugins",
		"/usr/lib/netscape/plugins-libc5",
		"/usr/lib/netscape/plugins-libc6",
		"/usr/lib64/netscape/plugins",
		"/usr/lib64/mozilla/plugins",
	};

	strSearchPaths[0] = QString("%1/%2").arg(QDir::homePath()).arg(strSearchPaths[0]);
	strSearchPaths[1] = QString("%1/%2").arg(QDir::homePath()).arg(strSearchPaths[1]);

	QString strFlashPlayerName[] =
	{
		"*flashplayer*.so",
		"*flashplugin*.so",
	};

	for (int i = 0; i < 19; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			QDir dir(strSearchPaths[i], strFlashPlayerName[j]);
			QFileInfoList infos = dir.entryInfoList();
			for (int k = 0; k < infos.size(); k++)
			{
				if (infos[k].exists())
					return true;
			}
		}
	}

#if _DEBUG
	qDebug() << QString("flash player is missing");
#endif

	return false;
}
