/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2019 QuiteRSS Team <quiterssteam@gmail.com>
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

#include "globals.h"

Settings::Settings()
{
  if (!globals.settings->group().isEmpty())
    globals.settings->endGroup();
}

Settings::~Settings()
{
  if (!globals.settings->group().isEmpty())
    globals.settings->endGroup();
}

void Settings::createSettings(const QString &fileName)
{
  if (!fileName.isEmpty()) {
    globals.settings = new QSettings(fileName, QSettings::IniFormat);
  } else {
    globals.settings = new QSettings(QSettings::IniFormat,
                              QSettings::UserScope,
                              QCoreApplication::organizationName(),
                              QCoreApplication::applicationName());
  }
}

QSettings* Settings::getSettings()
{
    return globals.settings;
}

void Settings::syncSettings()
{
  globals.settings->sync();
}

QString Settings::fileName()
{
  return globals.settings->fileName();
}

void Settings::beginGroup(const QString &prefix)
{
  globals.settings->beginGroup(prefix);
}

void Settings::endGroup()
{
  globals.settings->endGroup();
}

void Settings::setValue(const QString &key, const QVariant &defaultValue)
{
  globals.settings->setValue(key, defaultValue);
}

QVariant Settings::value(const QString &key, const QVariant &defaultValue)
{
  return globals.settings->value(key, defaultValue);
}

bool Settings::contains(const QString &key)
{
  return globals.settings->contains(key);
}
