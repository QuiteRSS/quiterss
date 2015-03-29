#!/bin/sh
set -x

if [ "${QT}" = '4' ]
then
	_qt_deps='libqt4-dev qt4-dev-tools libqtcore4 libqtgui4 libqt4-sql-sqlite libqtwebkit-dev libphonon-dev'
else
	_qt_deps='qtbase5-dev qtbase5-dev-tools qttools5-dev-tools libqt5webkit5-dev qtmultimedia5-dev'
fi

# Remove not used sources:
sudo ls -la /etc/apt/sources.list.d/
sudo rm -f /etc/apt/sources.list.d/couchdb-ppa-source.list
sudo rm -f /etc/apt/sources.list.d/mongodb.list
sudo rm -f /etc/apt/sources.list.d/pgdg-source.list
sudo rm -f /etc/apt/sources.list.d/rabbitmq-source.list
sudo rm -f /etc/apt/sources.list.d/rwky-redis-source.list
sudo rm -f /etc/apt/sources.list.d/travis_ci_zeromq3-source.list
sudo rm -f /etc/apt/sources.list.d/ubuntugis-stable-source.list
sudo rm -f /etc/apt/sources.list.d/webupd8team-java-ppa-source.list
sudo ls -la /etc/apt/sources.list.d/

# This is an ugly hack for partial updating of build environment from
# Ubuntu 12.04 (Precise Pangolin) to Ubuntu 14.04 (Trusty Tahr):
sudo sed -i 's/precise/trusty/g' /etc/apt/sources.list &&
sudo apt-get update -qq &&
sudo apt-get install -qq gdb libsqlite3-dev ${_qt_deps} &&
echo 'Updated'
