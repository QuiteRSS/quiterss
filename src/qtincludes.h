#ifndef QTINCLUDES_H
#define QTINCLUDES_H

/* Add C includes here */

#if defined __cplusplus
/* Add C++ includes here */

#ifdef QT_CORE_LIB
# include <QtCore>
#endif

#ifdef QT_GUI_LIB
# include <QtGui>
#endif

#ifdef QT_NETWORK_LIB
# include <QtNetwork>
#endif

#ifdef QT_XML_LIB
# include <QtXml>
#endif

#ifdef QT_SQL_LIB
# include <QtSql>
#endif

#ifdef QT_WEBKIT_LIB
# include <QtWebKit>
#endif

#endif

#endif // QTINCLUDES_H
