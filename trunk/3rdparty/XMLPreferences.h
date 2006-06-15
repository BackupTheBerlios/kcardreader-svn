/****************************************************************************
** Filename: XMLPreferences.h
** Last updated [dd/mm/yyyy]: 08/01/2006
**
** Class for reading and writing of XML configuration files.
** [Requires Qt4 - QtCore, QtGui and QtXml modules]
** Use of QtGui module may be avoided by #defining QT_CORE_ONLY.
** You will not be able to load/save QColor objects without the QtGui module!
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

#ifndef XMLPREFERENCES_H
#define XMLPREFERENCES_H

#include <QtCore/QByteArray>
#include <QtCore/QBitArray>
#include <QtCore/QVariant>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QRect>

#ifndef QT_CORE_ONLY
#include <QtGui/QColor>
#endif

class QDataStream;
class QFile;
class QDomElement;

class XMLWriter;

class XMLPreferences {
    public:
        enum BinEncoding { BE_CSV, BE_BASE64 };

        XMLPreferences( const QString& name, const QString& company = QString() );
        ~XMLPreferences();

        bool isNull();

        bool load( const QString& file );
        bool load( QFile* file );
        bool save( const QString& file );
        bool save( QFile* file );

        QString getString( const QString& section, const QString& name );
        QString getString( const QString& name ) {
            return getString( QString(), name );
        }

        QStringList getStringList( const QString& section, const QString& name );
        QStringList getStringList( const QString& name ) {
            return getStringList( QString(), name );
        }

        bool getBool( const QString& section, const QString& name, bool* ok = 0 );
        bool getBool( const QString& name, bool* ok = 0 ) {
            return getBool( QString(), name, ok );
        }

        int getInt( const QString& section, const QString& name, bool* ok = 0 );
        int getInt( const QString& name, bool* ok = 0 ) {
            return getInt( QString(), name, ok );
        }

        qint64 getInt64( const QString& section, const QString& name, bool* ok = 0 );
        qint64 getInt64( const QString& name, bool* ok = 0 ) {
            return getInt64( QString(), name, ok );
        }

        QBitArray getBitArray( const QString& section, const QString& name );
        QBitArray getBitArray( const QString& name ) {
            return getBitArray( QString(), name );
        }

        QByteArray getByteArray( const QString& section, const QString& name );
        QByteArray getByteArray( const QString& name ) {
            return getByteArray( QString(), name );
        }

        QRect getRect( const QString& section, const QString& name );
        QRect getRect( const QString& name ) {
            return getRect( QString(), name );
        }

        QPoint getPoint( const QString& section, const QString& name );
        QPoint getPoint( const QString& name ) {
            return getPoint( QString(), name );
        }

        QSize getSize( const QString& section, const QString& name );
        QSize getSize( const QString& name ) {
            return getSize( QString(), name );
        }

#ifndef QT_CORE_ONLY
        QColor getColor( const QString& section, const QString& name );
        QColor getColor( const QString& name ) {
            return getColor( QString(), name );
        }
#endif

        void addSection( const QString& section );
        bool deleteSection( const QString& section = QString() );

        bool setString( const QString& section, const QString& name, const QString& value );
        bool setString( const QString& name, const QString& value ) {
            return setString( QString(), name, value );
        }

        bool setStringList( const QString& section, const QString& name, const QStringList& value );
        bool setStringList( const QString& name, const QStringList& value ) {
            return setStringList( QString(), name, value );
        }

        bool setBool( const QString& section, const QString& name, bool value );
        bool setBool( const QString& name, bool value ) {
            return setBool( QString(), name, value );
        }

        bool setInt( const QString& section, const QString& name, int value );
        bool setInt( const QString& name, int value ) {
            return setInt( QString(), name, value );
        }

        bool setInt64( const QString& section, const QString& name, qint64 value );
        bool setInt64( const QString& name, qint64 value ) {
            return setInt64( QString(), name, value );
        }

        bool setBitArray( const QString& section, const QString& name, const QBitArray& value, BinEncoding encoding = BE_BASE64 );
        bool setBitArray( const QString& name, const QBitArray& value, BinEncoding encoding = BE_BASE64 ) {
            return setBitArray( QString(), name, value, encoding );
        }

        bool setByteArray( const QString& section, const QString& name, const QByteArray& value, BinEncoding encoding = BE_BASE64 );
        bool setByteArray( const QString& name, const QByteArray& value, BinEncoding encoding = BE_BASE64 ) {
            return setByteArray( QString(), name, value, encoding );
        }

        bool setRect( const QString& section, const QString& name, const QRect& value );
        bool setRect( const QString& name, const QRect& value ) {
            return setRect( QString(), name, value );
        }

        bool setSize( const QString& section, const QString& name, const QSize& value );
        bool setSize( const QString& name, const QSize& value ) {
            return setSize( QString(), name, value );
        }

        bool setPoint( const QString& section, const QString& name, const QPoint& value );
        bool setPoint( const QString& name, const QPoint& value ) {
            return setPoint( QString(), name, value );
        }

#ifndef QT_CORE_ONLY
        bool setColor( const QString& section, const QString& name, const QColor& value );
        bool setColor( const QString& name, const QColor& value ) {
            return setColor( QString(), name, value );
        }
#endif

        void clear();

        void setCompany( const QString& name );
        void setVersion( const QString& version );
        void setVersion( int major, int minor = 0 );

        void setDefaultSection( const QString& name );

        bool autoAddSections() const;
        void setAutoAddSections( bool enable = true );

    private:
        struct EncVariant {
            QVariant data;
            BinEncoding encoding;
        };

        QMap<QString, QMap<QString, EncVariant> >* mRoot;
        QMap<QString, EncVariant>* mDefaultMap;

        QString* mName;
        QString* mCompany;

        QString* mDefaultSection;

        int mVersionMajor;
        int mVersionMinor;

        bool mIgnoreMissingVersion;
        bool mAutoAddSections;

        void init();
        void writeSection( XMLWriter& out, const QString& name, const QMap<QString, EncVariant>& map );
        void parseSection( const QDomElement& el, QMap<QString, EncVariant>& map );
        bool checkVersion( const QString& version );
};

#endif // XMLPREFERENCES_H
