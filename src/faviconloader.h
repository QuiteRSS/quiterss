#ifndef FAVICONLOADER_H
#define FAVICONLOADER_H

#include <QThread>
#include <QNetworkReply>
#include <QIcon>
#include <QUrl>

class FaviconLoader : public QThread
{
  Q_OBJECT
public:
  FaviconLoader( QObject *pParent = 0);
  ~FaviconLoader();

  void setTargetUrl(const QString& strUrl);

protected:
  virtual void run();

private slots:
  void slotRecived(QNetworkReply *pReplay);

private:
  QString strFeedUrl_;
  QString strTargetUrl_;

signals:
  void signalIconRecived(const QString& strUrl, const QByteArray& byteArray);
};

#endif // FAVICONLOADER_H
