/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QWebView>

#define LEFT_BUTTON 0
#define MIDDLE_BUTTON 1
#define MIDDLE_BUTTON_MOD 2
#define LEFT_BUTTON_CTRL 3
#define LEFT_BUTTON_SHIFT 4
#define LEFT_BUTTON_ALT 5

class WebView : public QWebView
{
  Q_OBJECT
public:
  explicit WebView(QWidget *parent, QNetworkAccessManager *networkManager);

  int buttonClick_;
  bool rightButtonClick_;

protected:
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseReleaseEvent(QMouseEvent*);
  virtual void wheelEvent(QWheelEvent*);

private:
  int posX_;

};

#endif // WEBVIEW_H
