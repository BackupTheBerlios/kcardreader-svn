#/*
#*   Copyright (C) 2006 by Alexander Saal <alex.saal@gmx.de>
#*
#*   This program is free software; you can redistribute it and/or modify
#*   it under the terms of the GNU General Public License as published by
#*   the Free Software Foundation; either version 2 of the License, or (at
#*   your option) any later version.
#*
#*   This program is distributed in the hope that it will be useful, but
#*   WITHOUT ANY WARRANTY; without even the implied warranty of
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#*   GNU General Public License for more details.
#*
#*   You should have received a copy of the GNU General Public License
#*   along with this program; if not, write to the Free Software Foundation,
#*   Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#*/

# Qt 4 Settings
TEMPLATE = app
DESTDIR = bin
DEPENDPATH += include src ui 3rdparty
INCLUDEPATH += include lib 3rdparty
RESOURCES += res/qcr.qrc
CONFIG += debug thread warn_on qt
QT += xml

TARGET = qcardreader
#QMAKE_POST_LINK = strip -s bin/qcardreader
RCC_DIR = .unix/rcc
MOC_DIR += .unix/moc
OBJECTS_DIR += .unix/obj
UI_DIR += .unix/ui
LIBS += -L/usr/lib -lchipcard2c
LIBS += -L/usr/lib -lgwenhywfar

# Project Ui files
FORMS += ui/qcardreader.ui
FORMS += ui/login.ui

# Project Translation
#TRANSLATIONS += lng/qcr_de.ts

# Project Misc files
DISTFILES += ChangeLog
DISTFILES += LICENSE
DISTFILES += README
DISTFILES += INSTALL

# Project Header
HEADERS += include/qcr.h
HEADERS += lib/qcr_chipcard.h
HEADERS += lib/qcr_login.h

# Project Source
SOURCES += src/main.cpp
SOURCES += src/qcr.cpp
SOURCES += lib/qcr_chipcard.cpp
SOURCES += lib/qcr_login.cpp

# 3rdparty Header
HEADERS += 3rdparty/Base64.h
HEADERS += 3rdparty/XMLWriter.h
HEADERS += 3rdparty/XMLPreferences.h 

# 3rdparty Source
SOURCES += 3rdparty/Base64.cpp
SOURCES += 3rdparty/XMLPreferences.cpp
SOURCES += 3rdparty/XMLWriter.cpp 
