#!/bin/sh
set -x

# Defaults from Debian 8 with 'export DEB_BUILD_MAINT_OPTIONS="hardening=+all"':
#export CFLAGS="-g -O2 -fPIE -fstack-protector-strong -Wformat -Werror=format-security"
#export CXXFLAGS="-g -O2 -fPIE -fstack-protector-strong -Wformat -Werror=format-security"
#export CPPFLAGS="-D_FORTIFY_SOURCE=2"
#export LDFLAGS="-fPIE -pie -Wl,-z,relro -Wl,-z,now"

# Debian adds CPPFLAGS to both QMAKE_CFLAGS and QMAKE_CXXFLAGS

# -fstack-protector-strong needs gcc-4.9
export CFLAGS='-pipe -g -O2 -fPIE -fstack-protector -Wformat -Werror=format-security'
export CXXFLAGS="${CFLAGS}"
export CPPFLAGS='-D_FORTIFY_SOURCE=2'
export LDFLAGS='-fPIE -pie -Wl,-z,relro -Wl,-z,now -Wl,--hash-style=gnu -Wl,--as-needed'

mkdir _build &&
cd _build &&
/usr/lib/x86_64-linux-gnu/qt"${QT}"/bin/qmake \
	QMAKE_CC=$CC \
	QMAKE_CXX=$CXX \
	QMAKE_CFLAGS="${CFLAGS} ${CPPFLAGS}" \
	QMAKE_CFLAGS_RELEASE= \
	QMAKE_CFLAGS_DEBUG= \
	QMAKE_CXXFLAGS="${CXXFLAGS} ${CPPFLAGS}" \
	QMAKE_CXXFLAGS_RELEASE= \
	QMAKE_CXXFLAGS_DEBUG= \
	QMAKE_LFLAGS="${LDFLAGS}" \
	QMAKE_LFLAGS_RELEASE= \
	QMAKE_LFLAGS_DEBUG= \
	CONFIG+=$CONFIG \
	PREFIX=/usr \
	../QuiteRSS.pro &&
make -j2 &&
sudo make install &&
objdump -p /usr/bin/quiterss|grep NEEDED|sort &&
du -sc /usr/bin/quiterss /usr/share/quiterss/*
