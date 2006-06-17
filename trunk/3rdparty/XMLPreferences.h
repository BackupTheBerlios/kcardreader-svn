/****************************************************************************
** Filename: XMLPreferences.h
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** Class for reading and writing of XML configuration files.
**
** Copyright (C) 2005 Angius Fabrizio. All rights reserved.
**
** This file is part of the OSDaB project (http://osdab.sourceforge.net/).
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See the file LICENSE.GPL that came with this software distribution or
** visit http://www.gnu.org/copyleft/gpl.html for GPL licensing information.
**
**********************************************************************/

#ifndef OSDAB_XMLPREFERENCES_H
#define OSDAB_XMLPREFERENCES_H

#include <QtGlobal>
#include <QString>

class QFile;
class QByteArray;
class QBitArray;
class QStringList;
class QRect;
class QPoint;
class QSize;

#ifndef QT_CORE_ONLY
class QColor;
#endif

class XMLPreferencesPrivate;

class XMLPreferences
{
    public:
        enum BinEncodingType { CommaSeparatedValues, Base64 };

        XMLPreferences( const QString& name, const QString& company = QString() );
        ~XMLPreferences();

        bool isNull();

        bool load( const QString& file );
        bool load( QFile* file );
        bool save( const QString& file );
        bool save( QFile* file );

        QString getString( const QString& section, const QString& name );
        QString getString( const QString& name );

        QStringList getStringList( const QString& section, const QString& name );
        QStringList getStringList( const QString& name );

        bool getBool( const QString& section, const QString& name, bool* ok = 0 );
        bool getBool( const QString& name, bool* ok = 0 );

        int getInt( const QString& section, const QString& name, bool* ok = 0 );
        int getInt( const QString& name, bool* ok = 0 );

        qint64 getInt64( const QString& section, const QString& name, bool* ok = 0 );
        qint64 getInt64( const QString& name, bool* ok = 0 );

        QBitArray getBitArray( const QString& section, const QString& name );
        QBitArray getBitArray( const QString& name );

        QByteArray getByteArray( const QString& section, const QString& name );
        QByteArray getByteArray( const QString& name );

        QRect getRect( const QString& section, const QString& name );
        QRect getRect( const QString& name );

        QPoint getPoint( const QString& section, const QString& name );
        QPoint getPoint( const QString& name );

        QSize getSize( const QString& section, const QString& name );
        QSize getSize( const QString& name );

#ifndef QT_CORE_ONLY

        QColor getColor( const QString& section, const QString& name );
        QColor getColor( const QString& name );
#endif


        bool setString( const QString& section, const QString& name, const QString& value );
        bool setString( const QString& name, const QString& value );

        bool setStringList( const QString& section, const QString& name, const QStringList& value );
        bool setStringList( const QString& name, const QStringList& value );

        bool setBool( const QString& section, const QString& name, bool value );
        bool setBool( const QString& name, bool value );

        bool setInt( const QString& section, const QString& name, int value );
        bool setInt( const QString& name, int value );

        bool setInt64( const QString& section, const QString& name, qint64 value );
        bool setInt64( const QString& name, qint64 value );

        bool setBitArray( const QString& section, const QString& name, const QBitArray& value, BinEncodingType encoding = Base64 );
        bool setBitArray( const QString& name, const QBitArray& value, BinEncodingType encoding = Base64 );

        bool setByteArray( const QString& section, const QString& name, const QByteArray& value, BinEncodingType encoding = Base64 );
        bool setByteArray( const QString& name, const QByteArray& value, BinEncodingType encoding = Base64 );

        bool setRect( const QString& section, const QString& name, const QRect& value );
        bool setRect( const QString& name, const QRect& value );

        bool setSize( const QString& section, const QString& name, const QSize& value );
        bool setSize( const QString& name, const QSize& value );

        bool setPoint( const QString& section, const QString& name, const QPoint& value );
        bool setPoint( const QString& name, const QPoint& value );

#ifndef QT_CORE_ONLY

        bool setColor( const QString& section, const QString& name, const QColor& value );
        bool setColor( const QString& name, const QColor& value );
#endif

        void clear();

        void setCompany( const QString& name );
        void setVersion( const QString& version );
        void setVersion( int major, int minor = 0 );

        void setDefaultSection( const QString& name );

        void addSection( const QString& section );
        bool deleteSection( const QString& section = QString() );

        bool autoAddSections() const;
        void setAutoAddSections( bool enable = true );

    private:
        XMLPreferencesPrivate* d;
};

#endif // OSDAB_XMLPREFERENCES_H
