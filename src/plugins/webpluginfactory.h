#ifndef WEB_PLUGIN_FACTORY_H
#define WEB_PLUGIN_FACTORY_H

#include <QWebPluginFactory>

class WebPage;

class WebPluginFactory : public QWebPluginFactory
{
public:
  WebPluginFactory(WebPage *page, QObject *parent);

  virtual QObject *create(const QString &mimeType, const QUrl &url,
                          const QStringList &argumentNames,
                          const QStringList &argumentValues) const;
  QList<QWebPluginFactory::Plugin> plugins() const;

private:
  WebPage *page_;
  QObject *parent_;
};
#endif // WEB_PLUGIN_FACTORY_H
