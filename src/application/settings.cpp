/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2017 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "settings.h"

#include <QCoreApplication>

QSettings *Settings::settings_ = 0;

Settings::Settings()
{
  if (!settings_->group().isEmpty())
    settings_->endGroup();
}

Settings::~Settings()
{
  if (!settings_->group().isEmpty())
    settings_->endGroup();
}

void Settings::createSettings(const QString &fileName)
{
  if (!fileName.isEmpty()) {
    settings_ = new QSettings(fileName, QSettings::IniFormat);
  } else {
    settings_ = new QSettings(QSettings::IniFormat,
                              QSettings::UserScope,
                              QCoreApplication::organizationName(),
                              QCoreApplication::applicationName());
  }
}

QSettings* Settings::getSettings()
{
    return settings_;
}

void Settings::syncSettings()
{
  settings_->sync();
}

QString Settings::fileName()
{
  return settings_->fileName();
}

void Settings::beginGroup(const QString &prefix)
{
  settings_->beginGroup(prefix);
}

void Settings::endGroup()
{
  settings_->endGroup();
}

void Settings::setValue(const QString &key, const QVariant &defaultValue)
{
  settings_->setValue(key, defaultValue);
}

QVariant Settings::value(const QString &key, const QVariant &defaultValue)
{
  return settings_->value(key, defaultValue);
}

bool Settings::contains(const QString &key)
{
  return settings_->contains(key);
}
