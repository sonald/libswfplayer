#ifndef __SWF_PLAYER_H_
#define __SWF_PLAYER_H_

#include <QtCore>
#include <QImage>
#include <QWebView>

class KSwfFileInfo;
class KSwfPlayer: public QWebView
{
	Q_OBJECT
public:
	KSwfPlayer(QWidget* parent = 0);
	virtual ~KSwfPlayer();

	enum SwfPlayState
	{
		Invalid,		// no swf loaded
		Loaded,		// swf is loaded, and not playing
		Playing,
		Paused
	};

public slots:
	void Play();
	void Stop();
	void Pause();
	void Grab(QString strFilePath);
	void OnLoadFinished(bool bLoadFinished);

public:
	QImage ThumbNail() const;
	QSize SizePrefered() const { return m_SizePrefered; }
	SwfPlayState GetSwfPlayerState() { return m_eSwfPlayerState; }
	void LoadSwf(QString& strFileName);
	void EnableDebug(bool bEnableDebug);
	bool CheckPlugins();

protected:
	void resizeEvent(QResizeEvent *event);
	void showEvent(QShowEvent *event);
	void hideEvent(QHideEvent *event);
	void closeEvent(QCloseEvent *event);
	void contextMenuEvent(QContextMenuEvent * event);

private:
	QVariant Eval(const QString& script);

private:
	bool		m_bLoaded;
	KSwfFileInfo*		m_pSwfInfo;
	QSize		m_SizePrefered;
	SwfPlayState		m_eSwfPlayerState;
	bool m_bEnableDebug;
};

#endif /* ifndef __SWF_PLAYER_H_ */

