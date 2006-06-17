/****************************************************************************
** Filename: xmlpreferences.cpp
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

#include <QFile>
#include <QDir>
#include <QCoreApplication>

#include <QByteArray>
#include <QBitArray>
#include <QVariant>
#include <QHash>
#include <QStringList>
#include <QRect>

#ifndef QT_CORE_ONLY
#include <QColor>
#endif

#include <QDomDocument>
#include <QDomNode>
#include <QDomElement>

#include "XMLPreferences.h"
#include "Base64.h"
#include "XMLWriter.h"

/*!
	\class XMLPreferences xmlpreferences.h
 
	\brief The XMLPreferences class allows to easily store and retrieve
	application preferences from XML files.
 
	Adding support for new data types involves the following steps:
	<ul>
	<li>add the get and set methods to the header file interface
	<li>implement a getTYPE method; this usually involves
		<ul>
		<li>changing the type returned in case of error
		<li>changing QVariant::TYPE to your type
		<li>changing return itr.value().data.toTYPE() to your type
		</ul>
	 <li>implement a setTYPE method (usually tou only need to change the method parameters)
		<ul>
		<li>add a case entry to the writeSection() method to serialize the data type to a string
		<li>add an else if entry to the parseSection() method to deserialize the data type from a string
		</ul>
	 </ul>
*/

/*!
	\enum XMLPreferences::BinEncodingType Identifies how binary data should be encoded.
 
	\value CommaSeparatedValues Separates values using a simple comma
	\value Base64 Encodes binary data using base 64 encoding
*/


/************************************************************************
Private interface
*************************************************************************/

//! \internal
class XMLPreferencesPrivate
{
    public:
        XMLPreferencesPrivate( const QString& name, const QString& company );
        ~XMLPreferencesPrivate();

        typedef struct EncVariant
        {
            QVariant data;
            XMLPreferences::BinEncodingType encoding;
        };

        // document root contains all the sections except
        // the default section
        // <sectionName, <key,value>>
        QHash<QString, QHash<QString, EncVariant> >* documentRoot;

        // default section
        QHash<QString, EncVariant>* defaultSection;

        QString* appName;
        QString* appCompany;

        QString* defaultSectionName;

        int versionMajor;
        int versionMinor;

        bool ignoreMissingVersion;
        bool autoAddSections;

        void init();
        void writeSection( XMLWriter& out, const QString& name, const QHash<QString, EncVariant>& map );
        void parseSection( const QDomElement& el, QHash<QString, EncVariant>& map );
        bool checkVersion( const QString& version );
};

//! \internal
XMLPreferencesPrivate::XMLPreferencesPrivate( const QString& name, const QString& company )
{
    defaultSectionName = new QString( "default" );
    autoAddSections = true;

    if ( name.isEmpty() ) {
        QString myself = QCoreApplication::applicationFilePath();
        myself = myself.section( "/", -1, -1 );
        if ( myself.toLower().endsWith( ".exe" ) )
            myself.truncate( myself.length() - 4 );

        if ( myself.isEmpty() )
            appName = new QString( name );
        else
            appName = new QString( myself );
    } else
        appName = new QString( name );

    if ( company.isEmpty() )
        appCompany = 0;
    else
        appCompany = new QString( company );

    ignoreMissingVersion = true;
    versionMajor = -1;
    versionMinor = -1;

    documentRoot = 0;
    defaultSection = 0;
}

//! \internal
XMLPreferencesPrivate::~XMLPreferencesPrivate()
{
    delete documentRoot;
    delete defaultSection;

    delete appName;
    delete appCompany;

    delete defaultSectionName;
}

//! \internal
void XMLPreferencesPrivate::init()
{
    delete documentRoot;
    documentRoot = new QHash<QString, QHash<QString, EncVariant> >();
    delete defaultSection;
    defaultSection = new QHash<QString, EncVariant>();
}

//! \internal
void XMLPreferencesPrivate::writeSection( XMLWriter& out, const QString& name, const QHash<QString, EncVariant>& map )
{
    QHash<QString, EncVariant>::ConstIterator itr = map.constBegin();
    QHash<QString, QString>* attrs = new QHash<QString, QString>();

    attrs->insert( "name", name );
    out.writeOpenTag( "section", attrs );
    attrs->clear();

    while ( itr != map.constEnd() ) {
        if ( itr.value().data.isNull() ) {
            ++itr;
            continue;
        }

        attrs->insert( "name", itr.key() );

        // Supported types: QString, bool, int, QByteArray, QBitArray, QColor
        switch ( itr.value().data.type() ) {
            case QVariant::String :
                attrs->insert( "type", "qstring" );
                out.writeTaggedString( "setting", itr.value().data.toString(), attrs );
                break;
            case QVariant::StringList : {
                    attrs->insert( "type", "qstringlist" );
                    out.writeOpenTag( "setting", attrs );
                    attrs->clear();
                    QStringList list = itr.value().data.toStringList();
                    QStringList::Iterator listItr = list.begin();
                    while ( listItr != list.end() ) {
                        out.writeTaggedString( "value", *listItr, attrs );
                        ++listItr;
                    }
                    out.writeCloseTag( "setting" );
                }
                break;
            case QVariant::Bool :
                attrs->insert( "type", "bool" );
                out.writeTaggedString( "setting", itr.value().data.toBool() ? "true" : "false", attrs );
                break;
            case QVariant::Int :
                attrs->insert( "type", "int" );
                out.writeTaggedString( "setting", QString::number( itr.value().data.toInt() ), attrs );
                break;
            case QVariant::LongLong :
                attrs->insert( "type", "int64" );
                out.writeTaggedString( "setting", QString::number( itr.value().data.toLongLong() ), attrs );
                break;
            case QVariant::Rect : {
                    attrs->insert( "type", "qrect" );
                    QRect rect = itr.value().data.toRect();
                    QString s = QString( "%1;%2;%3;%4" ).arg( rect.x() ).arg( rect.y() ).arg( rect.width() ).arg( rect.height() );
                    out.writeTaggedString( "setting", s, attrs );
                }
                break;
            case QVariant::Point : {
                    attrs->insert( "type", "qpoint" );
                    QPoint point = itr.value().data.toPoint();
                    QString s = QString( "%1;%2" ).arg( point.x() ).arg( point.y() );
                    out.writeTaggedString( "setting", s, attrs );
                }
                break;
            case QVariant::Size : {
                    attrs->insert( "type", "qsize" );
                    QSize size = itr.value().data.toSize();
                    QString s = QString( "%1;%2" ).arg( size.width() ).arg( size.height() );
                    out.writeTaggedString( "setting", s, attrs );
                }
                break;
            case QVariant::ByteArray : {
                    attrs->insert( "type", "qbytearray" );
                    const QByteArray ba = itr.value().data.toByteArray();
                    switch ( itr.value().encoding ) {
                        case XMLPreferences::Base64:
                            attrs->insert( "encoding", "base64" );
                            out.writeTaggedString( "setting", Base64::encode( ba ), attrs );
                            break;
                        default:
                            attrs->insert( "encoding", "csv" );
                            QString s;
                            for ( uint i = 0; i < ( uint ) ba.size(); ++i )
                                ( i != 0 ) ? s += "," + QString::number( ( uint ) ba.at( i ), 16 ) : s += QString::number( ( uint ) ba.at( i ), 16 );
                            out.writeTaggedString( "setting", s, attrs );
                    }
                    attrs->clear();
                }
                break;
            case QVariant::BitArray : {
                    attrs->insert( "type", "qbitarray" );
                    const QBitArray ba = itr.value().data.toBitArray();
                    attrs->insert( "size", QString::number( ba.size() ) );
                    switch ( itr.value().encoding ) {
                        case XMLPreferences::Base64:
                            attrs->insert( "encoding", "base64" );
                            out.writeTaggedString( "setting", Base64::encode( ba ), attrs );
                            break;
                        default:
                            attrs->insert( "encoding", "csv" );
                            QString s;
                            for ( uint i = 0; i < ( uint ) ba.size(); ++i )
                            ( i != 0 ) ? s += ba.testBit( i ) ? ",1" : ",0" : s += ba.testBit( i ) ? "1" : "0";
                            out.writeTaggedString( "setting", s, attrs );
                    }
                    attrs->clear();
                }
                break;
#ifndef QT_CORE_ONLY

            case QVariant::Color :
                attrs->insert( "type", "qcolor" );
                out.writeTaggedString( "setting", itr.value().data.value<QColor>().name(), attrs );
                break;
#endif // QT_CORE_ONLY

            default:
                ;
        }

        ++itr;
    }

    out.writeCloseTag( "section" );
    delete attrs;
}

//! \internal
bool XMLPreferencesPrivate::checkVersion( const QString& version )
{
    if ( version.isEmpty() )
        return ignoreMissingVersion;

    QString sub = version.section( '.', 0, 0 );
    if ( sub.isEmpty() )
        return false;

    bool ok;
    int tmp = sub.toInt( &ok );
    if ( !ok || ( tmp < 0 ) || ( tmp > 1 ) )
        return false;

    versionMajor = tmp;

    sub = version.section( '.', 1, 1 );
    if ( sub.isEmpty() )
        return false;

    tmp = sub.toInt( &ok );
    if ( !ok || ( tmp < 0 ) )
        return false;

    versionMinor = tmp;

    return true;
}

//! \internal
void XMLPreferencesPrivate::parseSection( const QDomElement& rootEl, QHash<QString, EncVariant>& map )
{
    QDomNode node = rootEl.firstChild();
    QDomElement el;

    while ( !node.isNull() ) {
        el = node.toElement();
        if ( el.tagName() != "setting" ) {
            node = node.nextSibling();
            continue;
        }

        QString name = el.attribute( "name" );
        QString type = el.attribute( "type" );
        if ( name.isEmpty() || type.isEmpty() ) {
            node = node.nextSibling();
            continue;
        }

        // Supported types: QString, bool, int, QByteArray, QBitArray, QColor
        if ( type == "qstring" ) {
            EncVariant v;
            v.data = QVariant( el.text() );
            map.insert( name, v );
        } else if ( type == "qstringlist" ) {
            QDomNode listNode = el.firstChild();
            QString listText;
            QStringList list;
            while ( !listNode.isNull() ) {
                listText = listNode.toElement().text();
                if ( !listText.isEmpty() )
                    list.append( listText );
                listNode = listNode.nextSibling();
            }
            EncVariant v;
            v.data = QVariant( list );
            map.insert( name, v );
        } else if ( type == "bool" ) {
            EncVariant v;
            v.data = QVariant( el.text() == "true" );
            map.insert( name, v );
        } else if ( type == "int" ) {
            bool ok;
            int i = el.text().toInt( &ok );
            if ( ok ) {
                EncVariant v;
                v.data = QVariant( i );
                map.insert( name, v );
            }
        } else if ( type == "int64" ) {
            bool ok;
            qlonglong i = el.text().toLongLong( &ok );
            if ( ok ) {
                EncVariant v;
                v.data = QVariant( i );
                map.insert( name, v );
            }
        } else if ( type == "qrect" ) {
            QStringList lst = el.text().split( ';', QString::KeepEmptyParts );

            // a qrect has 4 values: xpos, ypos, width and height
            if ( lst.size() == 4 ) {
                int x, y, w, h;
                int count = 0;
                bool ok;

                x = lst[ 0 ].toInt( &ok );
                if ( ok )
                    ++count;
                y = lst[ 1 ].toInt( &ok );
                if ( ok )
                    ++count;
                w = lst[ 2 ].toInt( &ok );
                if ( ok )
                    ++count;
                h = lst[ 3 ].toInt( &ok );
                if ( ok )
                    ++count;

                if ( count == 4 ) {
                    EncVariant v;
                    v.data = QVariant( QRect( x, y, w, h ) );
                    map.insert( name, v );
                }
            }
        } else if ( type == "qpoint" ) {
            QStringList lst = el.text().split( ';', QString::KeepEmptyParts );

            // a qrect has 2 values, the x and y coordinates
            if ( lst.size() == 2 ) {
                int x, y;
                int count = 0;
                bool ok;

                x = lst[ 0 ].toInt( &ok );
                if ( ok )
                    ++count;
                y = lst[ 1 ].toInt( &ok );
                if ( ok )
                    ++count;

                if ( count == 2 ) {
                    EncVariant v;
                    v.data = QVariant( QPoint( x, y ) );
                    map.insert( name, v );
                }
            }
        } else if ( type == "qsize" ) {
            QStringList lst = el.text().split( ';', QString::KeepEmptyParts );

            // a qrect has 2 values: width and height
            if ( lst.size() == 2 ) {
                int w, h;
                int count = 0;
                bool ok;

                w = lst[ 0 ].toInt( &ok );
                if ( ok )
                    ++count;
                h = lst[ 1 ].toInt( &ok );
                if ( ok )
                    ++count;

                if ( count == 2 ) {
                    EncVariant v;
                    v.data = QVariant( QSize( w, h ) );
                    map.insert( name, v );
                }
            }
        } else if ( type == "qbytearray" ) {
            QString enc = el.attribute( "encoding" );
            if ( enc == "base64" ) {
                QByteArray ba = Base64::decode( el.text() );
                EncVariant v;
                v.data = QVariant( ba );
                v.encoding = XMLPreferences::Base64;
                map.insert( name, v );
            } else if ( enc == "csv" ) {
                QStringList lst = el.text().split( ',', QString::SkipEmptyParts );
                uint sz = lst.size();
                QByteArray ba( sz, '\0' );
                bool ok;
                uint count = 0;
                for ( QStringList::ConstIterator itr = lst.constBegin(); itr != lst.constEnd(); ++itr ) {
                    uint n = ( *itr ).toUInt( &ok, 16 );
                    if ( ok )
                        ba.data() [ count++ ] = ( uchar ) n;
                }
                if ( count < sz )
                    ba.resize( count );
                EncVariant v;
                v.data = QVariant( ba );
                v.encoding = XMLPreferences::CommaSeparatedValues;
                map.insert( name, v );
            }
        } else if ( type == "qbitarray" ) {
            bool ok;
            uint arr_sz = el.attribute( "size" ).toUInt( &ok );
            if ( !ok ) {
                node = node.nextSibling();
                continue;
            }

            QString enc = el.attribute( "encoding" );
            if ( enc == "base64" ) {
                QBitArray ba = Base64::decode( el.text(), arr_sz );
                EncVariant v;
                v.data = QVariant( ba );
                v.encoding = XMLPreferences::Base64;
                map.insert( name, v );
            } else if ( enc == "csv" ) {
                QStringList lst = el.text().split( ',', QString::SkipEmptyParts );
                uint sz = lst.size();
                if ( arr_sz < sz )
                    sz = arr_sz;
                QBitArray ba( sz );
                uint count = 0;
                for ( QStringList::ConstIterator itr = lst.constBegin(); itr != lst.constEnd(); ++itr ) {
                    if ( *itr == "1" )
                        ba.setBit( count++, true );
                    else if ( *itr == "0" )
                        ba.setBit( count++, false );
                }
                if ( count < sz )
                    ba.resize( count );
                EncVariant v;
                v.data = QVariant( ba );
                v.encoding = XMLPreferences::CommaSeparatedValues;
                map.insert( name, v );
            }
        }
#ifndef QT_CORE_ONLY
        else if ( type == "qcolor" ) {
            QColor col( el.text() );
            if ( col.isValid() ) {
                EncVariant v;
                v.data = QVariant( col );
                map.insert( name, v );
            }
        }
#endif // QT_CORE_ONLY

        node = node.nextSibling();

    } // while
}


/************************************************************************
Public interface
*************************************************************************/

/*!
	Creates a new XML preferences manager.
	\a name and \a company are used in the XML file header.
	Application file name is used if \a name is empty.
*/
XMLPreferences::XMLPreferences( const QString& name, const QString& company )
{
    d = new XMLPreferencesPrivate( name, company );
}

/*!
	Frees any allocated resources.
*/
XMLPreferences::~XMLPreferences()
{
    delete d;
}

/*!
	Returns true if there are no preferences set.
*/
bool XMLPreferences::isNull()
{
    return d->documentRoot == 0;
}

/*!
	Attempts to load settings from a file.
	Returns false on error.
*/
bool XMLPreferences::load( const QString& filename )
{
    QFile file( filename );

    if ( !file.open( QIODevice::ReadOnly ) )
        return false;

    bool res = load( &file );
    file.close();
    return res;
}

/*!
	Attempts to load settings from a file.
	Returns false on error.
*/
bool XMLPreferences::load( QFile* file )
{
    if ( file == 0 )
        return false;

    QDomDocument doc;
    if ( !doc.setContent( file, false ) )
        return false;

    QDomElement docroot = doc.documentElement();
    if ( docroot.isNull() )
        return false;

    if ( docroot.tagName() != "XMLPreferences" )
        return false;

    QString attribute;

    // get file version
    attribute = docroot.attribute( "update" );
    if ( !d->checkVersion( attribute ) )
        return false;

    if ( d->documentRoot == 0 )
        d->init();

    QDomNode node = docroot.firstChild();
    QDomElement el;

    while ( !node.isNull() ) {
        el = node.toElement();

        if ( ( el.tagName() == "section" ) && el.hasChildNodes() ) {
            QString sName = el.attribute( "name" );
            if ( sName.isEmpty() ) {
                node = node.nextSibling();
                continue;
            }

            if ( sName == ( *d->defaultSectionName ) )
                d->parseSection( el, *d->defaultSection );
            else {
                // find or create the right section
                QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr =
                    d->documentRoot->find( sName );
                if ( itr == d->documentRoot->end() ) {
                    // create a new section
                    QHash<QString, XMLPreferencesPrivate::EncVariant> section;
                    d->parseSection( el, section );
                    d->documentRoot->insert( sName, section );
                } else
                    d->documentRoot->insert( sName, itr.value() );
            }
        }

        node = node.nextSibling();

    } // while

    return true;
}

/*!
	Attempts to write the settings to a file.
	Returns false on error.
*/
bool XMLPreferences::save( const QString& filename )
{
    QFile file( filename );
    if ( !file.open( QIODevice::WriteOnly ) )
        return false;

    bool res = save( &file );
    file.close();
    return res;
}

/*!
	Attempts to write the settings to a file.
	Returns false on error.
*/
bool XMLPreferences::save( QFile* file )
{
    if ( file == 0 )
        return false;

    if ( d->documentRoot == 0 )
        d->init();

    XMLWriter out( file );
    QHash<QString, QString>* attrs = new QHash<QString, QString>();

    attrs->insert( "version", "1.0" );
    out.writeOpenTag( "XMLPreferences", attrs );
    attrs->clear();

    if ( d->appCompany != 0 )
        attrs->insert( "company", *d->appCompany );
    if ( d->versionMajor > 0 )
        attrs->insert( "version", QString( "%1.%2" ).arg( d->versionMajor ).arg( d->versionMinor ) );
    attrs->insert( "product", *d->appName );
    out.writeAtomTag( "software_info", attrs );

    delete attrs;

    // DEFAULT SECTION
    if ( d->defaultSection != 0 )
        if ( !d->defaultSection->isEmpty() )
            d->writeSection( out, *d->defaultSectionName, *d->defaultSection );

    QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr
    = d->documentRoot->begin();

    while ( itr != d->documentRoot->end() ) {
        d->writeSection( out, itr.key(), itr.value() );
        ++itr;
    }

    out.writeCloseTag( "XMLPreferences" );

    return true;
}

/*!
	Returns the string value associated to \a name in the default section.
	Returns QString() if no such value could be found.
*/
QString XMLPreferences::getString( const QString& name )
{
    return getString( QString(), name );
}

/*!
	Returns the string value associated to \a name in \a section or in the
	default section if the \a section name is empty.
	Returns QString() if no such value could be found.
*/
QString XMLPreferences::getString( const QString& section, const QString& name )
{
    if ( d->documentRoot == 0 )
        return QString();

    if ( section.isEmpty() ) {
        QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr =
            d->defaultSection->find( name );
        if ( itr == d->defaultSection->end() ) {
            // no such value
            return QString();
        } else {
            if ( itr.value().data.type() != QVariant::String )
                return QString();
            else
                return itr.value().data.toString();
        }
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator sec =
            d->documentRoot->find( section );
        if ( sec == d->documentRoot->end() ) {
            // no such section
            return QString();
        } else {
            QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr = sec.value().find( name );
            if ( itr == sec.value().end() ) {
                // no such value
                return QString();
            } else {
                if ( itr.value().data.type() != QVariant::String )
                    return QString();
                else
                    return itr.value().data.toString();
            }
        }
    }

    return QString();
}


/*!
	Returns the string list associated to \a name in the default section.
	Returns an empty list if no such value could be found.
*/
QStringList XMLPreferences::getStringList( const QString& name )
{
    return getStringList( QString(), name );
}

/*!
	Returns the string list associated to \a name in \a section or in the
	default section if the \a section name is empty.
	Returns an empty list if no such value could be found.
*/
QStringList XMLPreferences::getStringList( const QString& section, const QString& name )
{
    if ( d->documentRoot == 0 )
        return QStringList();

    if ( section.isEmpty() ) {
        QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr =
            d->defaultSection->find( name );
        if ( itr == d->defaultSection->end() ) {
            // no such value
            return QStringList();
        } else {
            if ( itr.value().data.type() != QVariant::StringList )
                return QStringList();
            else
                return itr.value().data.toStringList();
        }
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator sec =
            d->documentRoot->find( section );
        if ( sec == d->documentRoot->end() ) {
            // no such section
            return QStringList();
        } else {
            QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr = sec.value().find( name );
            if ( itr == sec.value().end() ) {
                // no such value
                return QStringList();
            } else {
                if ( itr.value().data.type() != QVariant::StringList )
                    return QStringList();
                else
                    return itr.value().data.toStringList();
            }
        }
    }

    return QStringList();
}

/*!
	Returns the boolean value associated to \a name in the default section.
	Sets \a ok to false if no such value could be found and \a ok is not null.
*/
bool XMLPreferences::getBool( const QString& name, bool* ok )
{
    return getBool( QString(), name, ok );
}

/*!
	Returns the boolean value associated to \a name in \a section or in the
	default section if the \a section name is empty.
	Sets \a ok to false if no such value could be found and \a ok is not null.
*/
bool XMLPreferences::getBool( const QString& section, const QString& name, bool* ok )
{
    if ( d->documentRoot == 0 ) {
        if ( ok != 0 )
            * ok = false;
        return false;
    }

    if ( ok != 0 )
        * ok = true;

    if ( section.isEmpty() ) {
        QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr = d->defaultSection->find( name );
        if ( itr == d->defaultSection->end() ) {
            // no such value
            if ( ok != 0 )
                * ok = false;
            return false;
        } else {
            if ( itr.value().data.type() != QVariant::Bool ) {
                if ( ok != 0 )
                    * ok = false;
                return false;
            } else
                return itr.value().data.toBool();
        }
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator sec =
            d->documentRoot->find( section );
        if ( sec == d->documentRoot->end() ) {
            // no such section
            if ( ok != 0 )
                * ok = false;
            return false;
        } else {
            QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr = sec.value().find( name );
            if ( itr == sec.value().end() ) {
                // no such value
                if ( ok != 0 )
                    * ok = false;
                return false;
            } else {
                if ( itr.value().data.type() != QVariant::Bool ) {
                    if ( ok != 0 )
                        * ok = false;
                    return false;
                } else
                    return itr.value().data.toBool();
            }
        }
    }

    if ( ok != 0 )
        * ok = false;
    return false;
}

/*!
	Returns the integer value associated to \a name in the default section.
	Sets \a ok to false if no such value could be found and \a ok is not null.
*/
int XMLPreferences::getInt( const QString& name, bool* ok )
{
    return getInt( QString(), name, ok );
}

/*!
	Returns the integer value associated to \a name in \a section or in the
	default section if the \a section name is empty.
	Sets \a ok to false if no such value could be found and \a ok is not null.
*/
int XMLPreferences::getInt( const QString& section, const QString& name, bool* ok )
{
    if ( d->documentRoot == 0 ) {
        if ( ok != 0 )
            * ok = false;
        return 0;
    }

    if ( ok != 0 )
        * ok = true;

    if ( section.isEmpty() ) {
        QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr =
            d->defaultSection->find( name );
        if ( itr == d->defaultSection->end() ) {
            // no such value
            if ( ok != 0 )
                * ok = false;
            return 0;
        } else {
            if ( itr.value().data.type() != QVariant::Int ) {
                if ( ok != 0 )
                    * ok = false;
                return 0;
            } else
                return itr.value().data.toInt();
        }
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator sec =
            d->documentRoot->find( section );
        if ( sec == d->documentRoot->end() ) {
            // no such section
            if ( ok != 0 )
                * ok = false;
            return 0;
        } else {
            QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr = sec.value().find( name );
            if ( itr == sec.value().end() ) {
                // no such value
                if ( ok != 0 )
                    * ok = false;
                return 0;
            } else {
                if ( itr.value().data.type() != QVariant::Int ) {
                    if ( ok != 0 )
                        * ok = false;
                    return 0;
                } else
                    return itr.value().data.toInt();
            }
        }
    }

    if ( ok != 0 )
        * ok = false;
    return 0;
}

/*!
	Returns the 64bit integer value associated to \a name in the default section.
	Sets \a ok to false if no such value could be found and \a ok is not null.
*/
qint64 XMLPreferences::getInt64( const QString& name, bool* ok )
{
    return getInt64( QString(), name, ok );
}

/*!
	Returns the 64bit integer value associated to \a name in \a section or in the
	default section if the \a section name is empty.
	Sets \a ok to false if no such value could be found and \a ok is not null.
*/
qint64 XMLPreferences::getInt64( const QString& section, const QString& name, bool* ok )
{
    if ( d->documentRoot == 0 ) {
        if ( ok != 0 )
            * ok = false;
        return Q_INT64_C( 0 );
    }

    if ( ok != 0 )
        * ok = true;

    if ( section.isEmpty() ) {
        QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr =
            d->defaultSection->find( name );
        if ( itr == d->defaultSection->end() ) {
            // no such value
            if ( ok != 0 )
                * ok = false;
            return Q_INT64_C( 0 );
        } else {
            if ( itr.value().data.type() != QVariant::LongLong ) {
                if ( ok != 0 )
                    * ok = false;
                return Q_INT64_C( 0 );
            } else
                return ( qint64 ) itr.value().data.toLongLong();
        }
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator sec =
            d->documentRoot->find( section );
        if ( sec == d->documentRoot->end() ) {
            // no such section
            if ( ok != 0 )
                * ok = false;
            return Q_INT64_C( 0 );
        } else {
            QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr = sec.value().find( name );
            if ( itr == sec.value().end() ) {
                // no such value
                if ( ok != 0 )
                    * ok = false;
                return Q_INT64_C( 0 );
            } else {
                if ( itr.value().data.type() != QVariant::LongLong ) {
                    if ( ok != 0 )
                        * ok = false;
                    return Q_INT64_C( 0 );
                } else
                    return ( qint64 ) itr.value().data.toLongLong();
            }
        }
    }

    if ( ok != 0 )
        * ok = false;
    return Q_INT64_C( 0 );
}

/*!
	Returns the bit array associated to \a name in the default section.
	Returns an empty array if no such value could be found
*/
QBitArray XMLPreferences::getBitArray( const QString& name )
{
    return getBitArray( QString(), name );
}

/*!
	Returns the bit array associated to \a name in \a section or in the
	default section if the \a section name is empty.
	Returns an empty array if no such value could be found
*/
QBitArray XMLPreferences::getBitArray( const QString& section, const QString& name )
{
    if ( d->documentRoot == 0 )
        return QBitArray();

    if ( section.isEmpty() ) {
        QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr =
            d->defaultSection->find( name );
        if ( itr == d->defaultSection->end() ) {
            // no such value
            return QBitArray();
        } else {
            if ( itr.value().data.type() != QVariant::BitArray )
                return QBitArray();
            else
                return itr.value().data.toBitArray();
        }
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator sec =
            d->documentRoot->find( section );
        if ( sec == d->documentRoot->end() ) {
            // no such section
            return QBitArray();
        } else {
            QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr = sec.value().find( name );
            if ( itr == sec.value().end() ) {
                // no such value
                return QBitArray();
            } else {
                if ( itr.value().data.type() != QVariant::BitArray )
                    return QBitArray();
                else
                    return itr.value().data.toBitArray();
            }
        }
    }

    return QBitArray();
}

/*!
	Returns the byte array associated to \a name in the default section.
	Returns an empty array if no such value could be found
*/
QByteArray XMLPreferences::getByteArray( const QString& name )
{
    return getByteArray( QString(), name );
}

/*!
	Returns the byte array associated to \a name in \a section or in the
	default section if the \a section name is empty.
	Returns an empty array if no such value could be found
*/
QByteArray XMLPreferences::getByteArray( const QString& section, const QString& name )
{
    if ( d->documentRoot == 0 )
        return QByteArray();

    if ( section.isEmpty() ) {
        QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr =
            d->defaultSection->find( name );
        if ( itr == d->defaultSection->end() ) {
            // no such value
            return QByteArray();
        } else {
            if ( itr.value().data.type() != QVariant::ByteArray )
                return QByteArray();
            else
                return itr.value().data.toByteArray();
        }
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator sec =
            d->documentRoot->find( section );
        if ( sec == d->documentRoot->end() ) {
            // no such section
            return QByteArray();
        } else {
            QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr = sec.value().find( name );
            if ( itr == sec.value().end() ) {
                // no such value
                return QByteArray();
            } else {
                if ( itr.value().data.type() != QVariant::ByteArray )
                    return QByteArray();
                else
                    return itr.value().data.toByteArray();
            }
        }
    }

    return QByteArray();
}

/*!
	Returns the rectangle associated to \a name in the default section.
	Returns an empty rectangle if no such value could be found
*/
QRect XMLPreferences::getRect( const QString& name )
{
    return getRect( QString(), name );
}

/*!
	Returns the rectangle associated to \a name in \a section or in the
	default section if the \a section name is empty.
	Returns an empty rectangle if no such value could be found
*/
QRect XMLPreferences::getRect( const QString& section, const QString& name )
{
    if ( d->documentRoot == 0 ) {
        return QRect();
    }

    if ( section.isEmpty() ) {
        QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr =
            d->defaultSection->find( name );
        if ( itr == d->defaultSection->end() ) {
            // no such value
            return QRect();
        } else {
            if ( itr.value().data.type() != QVariant::Rect ) {
                return QRect();
            } else
                return itr.value().data.toRect();
        }
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator sec =
            d->documentRoot->find( section );
        if ( sec == d->documentRoot->end() ) {
            // no such section
            return QRect();
        } else {
            QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr = sec.value().find( name );
            if ( itr == sec.value().end() ) {
                // no such value
                return QRect();
            } else {
                if ( itr.value().data.type() != QVariant::Rect ) {
                    return QRect();
                } else
                    return itr.value().data.toRect();
            }
        }
    }

    return QRect();
}

/*!
	Returns the point associated to \a name in the default section.
	Returns an invalid point if no such value could be found
*/
QPoint XMLPreferences::getPoint( const QString& name )
{
    return getPoint( QString(), name );
}

/*!
	Returns the point associated to \a name in \a section or in the
	default section if the \a section name is empty.
	Returns an invalid point if no such value could be found
*/
QPoint XMLPreferences::getPoint( const QString& section, const QString& name )
{
    if ( d->documentRoot == 0 ) {
        return QPoint();
    }

    if ( section.isEmpty() ) {
        QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr =
            d->defaultSection->find( name );
        if ( itr == d->defaultSection->end() ) {
            // no such value
            return QPoint();
        } else {
            if ( itr.value().data.type() != QVariant::Point ) {
                return QPoint();
            } else
                return itr.value().data.toPoint();
        }
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator sec =
            d->documentRoot->find( section );
        if ( sec == d->documentRoot->end() ) {
            // no such section
            return QPoint();
        } else {
            QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr = sec.value().find( name );
            if ( itr == sec.value().end() ) {
                // no such value
                return QPoint();
            } else {
                if ( itr.value().data.type() != QVariant::Point ) {
                    return QPoint();
                } else
                    return itr.value().data.toPoint();
            }
        }
    }

    return QPoint();
}

/*!
	Returns the size associated to \a name in the default section.
	Returns an invalid size if no such value could be found
*/
QSize XMLPreferences::getSize( const QString& name )
{
    return getSize( QString(), name );
}

/*!
	Returns the size associated to \a name in \a section or in the
	default section if the \a section name is empty.
	Returns an invalid size if no such value could be found
*/
QSize XMLPreferences::getSize( const QString& section, const QString& name )
{
    if ( d->documentRoot == 0 ) {
        return QSize();
    }

    if ( section.isEmpty() ) {
        QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr =
            d->defaultSection->find( name );
        if ( itr == d->defaultSection->end() ) {
            // no such value
            return QSize();
        } else {
            if ( itr.value().data.type() != QVariant::Size ) {
                return QSize();
            } else
                return itr.value().data.toSize();
        }
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator sec =
            d->documentRoot->find( section );
        if ( sec == d->documentRoot->end() ) {
            // no such section
            return QSize();
        } else {
            QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr = sec.value().find( name );
            if ( itr == sec.value().end() ) {
                // no such value
                return QSize();
            } else {
                if ( itr.value().data.type() != QVariant::Size ) {
                    return QSize();
                } else
                    return itr.value().data.toSize();
            }
        }
    }

    return QSize();
}

#ifndef QT_CORE_ONLY

/*!
	Returns the color associated to \a name in the default section.
	Returns a null color if no such value could be found
*/
QColor XMLPreferences::getColor( const QString& name )
{
    return getColor( QString(), name );
}

/*!
	Returns the color associated to \a name in \a section or in the
	default section if the \a section name is empty.
	Returns a null color if no such value could be found
*/
QColor XMLPreferences::getColor( const QString& section, const QString& name )
{
    if ( d->documentRoot == 0 )
        return QColor();

    if ( section.isEmpty() ) {
        QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr =
            d->defaultSection->find( name );
        if ( itr == d->defaultSection->end() ) {
            // no such value
            return QColor();
        } else {
            if ( itr.value().data.type() != QVariant::Color )
                return QColor();
            else
                return itr.value().data.value<QColor>();
        }
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator sec =
            d->documentRoot->find( section );
        if ( sec == d->documentRoot->end() ) {
            // no such section
            return QColor();
        } else {
            QHash<QString, XMLPreferencesPrivate::EncVariant>::Iterator itr = sec.value().find( name );
            if ( itr == sec.value().end() ) {
                // no such value
                return QColor();
            } else {
                if ( itr.value().data.type() != QVariant::Color )
                    return QColor();
                else
                    return itr.value().data.value<QColor>();
            }
        }
    }

    return QColor();
}

#endif // QT_CORE_ONLY

/*!
	Adds a new section to the settings.
*/
void XMLPreferences::addSection( const QString& section )
{
    if ( section.isEmpty() || ( section == *d->defaultSectionName ) )
        return ;

    if ( d->documentRoot == 0 )
        d->init();

    if ( d->documentRoot->contains( section ) )
        return ;

    QHash<QString, XMLPreferencesPrivate::EncVariant> map;
    d->documentRoot->insert( section, map );
}


/*!
	Removes a section and all its data.
	Returns false if no such section exists.
 */
bool XMLPreferences::deleteSection( const QString& section )
{
    if ( d->documentRoot == 0 )
        return false;

    if ( section.isEmpty() || ( section == *d->defaultSectionName ) ) {
        delete d->defaultSection;
        d->defaultSection = new QHash<QString, XMLPreferencesPrivate::EncVariant>();
        return true;
    }

    QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr = d->documentRoot->find( section );
    if ( itr == d->documentRoot->end() )
        return false;

    d->documentRoot->erase( itr );
    return true;
}

/*!
	Adds a string type value to the default section and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setString( const QString& name, const QString& value )
{
    return setString( QString(), name, value );
}

/*!
	Adds a string type value to \a section or to the default section if \a section is empty
	and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
 */
bool XMLPreferences::setString( const QString& section, const QString& name, const QString& value )
{
    if ( name.isEmpty() )
        return false;

    if ( d->documentRoot == 0 )
        d->init();

    if ( section.isEmpty() || ( section == *d->defaultSectionName ) ) {
        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( value );
        d->defaultSection->insert( name, v );
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr = d->documentRoot->find( section );
        if ( itr == d->documentRoot->end() ) {
            if ( !d->autoAddSections )
                return false;
            QHash<QString, XMLPreferencesPrivate::EncVariant> map;
            itr = d->documentRoot->insert( section, map );
        }

        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( value );
        itr.value().insert( name, v );
    }

    return true;
}

/*!
	Adds a string list type value to the default section and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setStringList( const QString& name, const QStringList& value )
{
    return setStringList( QString(), name, value );
}

/*!
	Adds a string list type value to \a section or to the default section if \a section is empty
	and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setStringList( const QString& section, const QString& name, const QStringList& value )
{
    if ( name.isEmpty() )
        return false;

    if ( d->documentRoot == 0 )
        d->init();

    if ( section.isEmpty() || ( section == *d->defaultSectionName ) ) {
        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( value );
        d->defaultSection->insert( name, v );
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr =
            d->documentRoot->find( section );
        if ( itr == d->documentRoot->end() ) {
            if ( !d->autoAddSections )
                return false;
            QHash<QString, XMLPreferencesPrivate::EncVariant> map;
            itr = d->documentRoot->insert( section, map );
        }

        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( value );
        itr.value().insert( name, v );
    }

    return true;
}

/*!
	Adds a boolean type value to the default section and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setBool( const QString& name, bool value )
{
    return setBool( QString(), name, value );
}

/*!
	Adds a boolean type value to \a section or to the default section if \a section is empty
	and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setBool( const QString& section, const QString& name, bool value )
{
    if ( name.isEmpty() )
        return false;

    if ( d->documentRoot == 0 )
        d->init();

    if ( section.isEmpty() || ( section == *d->defaultSectionName ) ) {
        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( value );
        d->defaultSection->insert( name, v );
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr =
            d->documentRoot->find( section );
        if ( itr == d->documentRoot->end() ) {
            if ( !d->autoAddSections )
                return false;
            QHash<QString, XMLPreferencesPrivate::EncVariant> map;
            itr = d->documentRoot->insert( section, map );
        }

        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( value );
        itr.value().insert( name, v );
    }

    return true;
}

/*!
	Adds an integer type value to the default section and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setInt( const QString& name, int value )
{
    return setInt( QString(), name, value );
}

/*!
	Adds an integer type value to \a section or to the default section if \a section is empty
	and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setInt( const QString& section, const QString& name, int value )
{
    if ( name.isEmpty() )
        return false;

    if ( d->documentRoot == 0 )
        d->init();

    if ( section.isEmpty() || ( section == *d->defaultSectionName ) ) {
        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( value );
        d->defaultSection->insert( name, v );
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr =
            d->documentRoot->find( section );
        if ( itr == d->documentRoot->end() ) {
            if ( !d->autoAddSections )
                return false;
            QHash<QString, XMLPreferencesPrivate::EncVariant> map;
            itr = d->documentRoot->insert( section, map );
        }

        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( value );
        itr.value().insert( name, v );
    }

    return true;
}

/*!
	Adds a 64bit integer type value to the default section and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setInt64( const QString& name, qint64 value )
{
    return setInt64( QString(), name, value );
}

/*!
	Adds a 64bit integer type value to \a section or to the default section if \a section is empty
	and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setInt64( const QString& section, const QString& name, qint64 value )
{
    if ( name.isEmpty() )
        return false;

    if ( d->documentRoot == 0 )
        d->init();

    if ( section.isEmpty() || ( section == *d->defaultSectionName ) ) {
        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( ( qlonglong ) value );
        d->defaultSection->insert( name, v );
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr =
            d->documentRoot->find( section );
        if ( itr == d->documentRoot->end() ) {
            if ( !d->autoAddSections )
                return false;
            QHash<QString, XMLPreferencesPrivate::EncVariant> map;
            itr = d->documentRoot->insert( section, map );
        }

        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( value );
        itr.value().insert( name, v );
    }

    return true;
}

/*!
	Adds a bit array type value to the default section and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setBitArray( const QString& name, const QBitArray& value, BinEncodingType encoding )
{
    return setBitArray( QString(), name, value, encoding );
}

/*!
	Adds a bit array integer type value to \a section or to the default section if \a section is empty
	and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setBitArray( const QString& section, const QString& name, const QBitArray& value, BinEncodingType encoding )
{
    if ( name.isEmpty() )
        return false;

    if ( d->documentRoot == 0 )
        d->init();

    if ( section.isEmpty() || ( section == *d->defaultSectionName ) ) {
        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( value );
        v.encoding = encoding;
        d->defaultSection->insert( name, v );
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr =
            d->documentRoot->find( section );
        if ( itr == d->documentRoot->end() ) {
            if ( !d->autoAddSections )
                return false;
            QHash<QString, XMLPreferencesPrivate::EncVariant> map;
            itr = d->documentRoot->insert( section, map );
        }

        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( value );
        v.encoding = encoding;
        itr.value().insert( name, v );
    }

    return true;
}

/*!
	Adds a byte array type value to the default section and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setByteArray( const QString& name, const QByteArray& value, BinEncodingType encoding )
{
    return setByteArray( QString(), name, value, encoding );
}

/*!
	Adds a byte array integer type value to \a section or to the default section if \a section is empty
	and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setByteArray( const QString& section, const QString& name, const QByteArray& value, BinEncodingType encoding )
{
    if ( name.isEmpty() )
        return false;

    if ( d->documentRoot == 0 )
        d->init();

    if ( section.isEmpty() || ( section == *d->defaultSectionName ) ) {
        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( value );
        v.encoding = encoding;
        d->defaultSection->insert( name, v );
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr =
            d->documentRoot->find( section );
        if ( itr == d->documentRoot->end() ) {
            if ( !d->autoAddSections )
                return false;
            QHash<QString, XMLPreferencesPrivate::EncVariant> map;
            itr = d->documentRoot->insert( section, map );
        }

        XMLPreferencesPrivate::EncVariant v;
        v.data = QVariant( value );
        v.encoding = encoding;
        itr.value().insert( name, v );
    }

    return true;
}

/*!
	Adds a rectangle type value to the default section and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setRect( const QString& name, const QRect& value )
{
    return setRect( QString(), name, value );
}

/*!
	Adds a rectangle type value to \a section or to the default section if \a section is empty
	and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setRect( const QString& section, const QString& name, const QRect& value )
{
    if ( name.isEmpty() )
        return false;

    if ( d->documentRoot == 0 )
        d->init();

    if ( section.isEmpty() || ( section == *d->defaultSectionName ) ) {
        XMLPreferencesPrivate::EncVariant v;
        v.data = value;
        d->defaultSection->insert( name, v );
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr =
            d->documentRoot->find( section );
        if ( itr == d->documentRoot->end() ) {
            if ( !d->autoAddSections )
                return false;
            QHash<QString, XMLPreferencesPrivate::EncVariant> map;
            itr = d->documentRoot->insert( section, map );
        }

        XMLPreferencesPrivate::EncVariant v;
        v.data = value;
        itr.value().insert( name, v );
    }

    return true;
}

/*!
	Adds a size type value to the default section and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setSize( const QString& name, const QSize& value )
{
    return setSize( QString(), name, value );
}

/*!
	Adds a size type value to \a section or to the default section if \a section is empty
	and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setSize( const QString& section, const QString& name, const QSize& value )
{
    if ( name.isEmpty() )
        return false;

    if ( d->documentRoot == 0 )
        d->init();

    if ( section.isEmpty() || ( section == *d->defaultSectionName ) ) {
        XMLPreferencesPrivate::EncVariant v;
        v.data = value;
        d->defaultSection->insert( name, v );
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr =
            d->documentRoot->find( section );
        if ( itr == d->documentRoot->end() ) {
            if ( !d->autoAddSections )
                return false;
            QHash<QString, XMLPreferencesPrivate::EncVariant> map;
            itr = d->documentRoot->insert( section, map );
        }

        XMLPreferencesPrivate::EncVariant v;
        v.data = value;
        itr.value().insert( name, v );
    }

    return true;
}

/*!
	Adds a point type value to the default section and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setPoint( const QString& name, const QPoint& value )
{
    return setPoint( QString(), name, value );
}

/*!
	Adds a point type value to \a section or to the default section if \a section is empty
	and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setPoint( const QString& section, const QString& name, const QPoint& value )
{
    if ( name.isEmpty() )
        return false;

    if ( d->documentRoot == 0 )
        d->init();

    if ( section.isEmpty() || ( section == *d->defaultSectionName ) ) {
        XMLPreferencesPrivate::EncVariant v;
        v.data = value;
        d->defaultSection->insert( name, v );
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr =
            d->documentRoot->find( section );
        if ( itr == d->documentRoot->end() ) {
            if ( !d->autoAddSections )
                return false;
            QHash<QString, XMLPreferencesPrivate::EncVariant> map;
            itr = d->documentRoot->insert( section, map );
        }

        XMLPreferencesPrivate::EncVariant v;
        v.data = value;
        itr.value().insert( name, v );
    }

    return true;
}

#ifndef QT_CORE_ONLY

/*!
	Adds a color type value to the default section and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setColor( const QString& name, const QColor& value )
{
    return setColor( QString(), name, value );
}

/*!
	Adds a color type value to \a section or to the default section if \a section is empty
	and binds it to \a name.
	Returns false if the value could not be added (invalid value or section or name).
*/
bool XMLPreferences::setColor( const QString& section, const QString& name, const QColor& value )
{
    if ( name.isEmpty() )
        return false;

    if ( d->documentRoot == 0 )
        d->init();

    if ( section.isEmpty() || ( section == *d->defaultSectionName ) ) {
        XMLPreferencesPrivate::EncVariant v;
        v.data = value;
        d->defaultSection->insert( name, v );
    } else {
        QHash<QString, QHash<QString, XMLPreferencesPrivate::EncVariant> >::Iterator itr =
            d->documentRoot->find( section );
        if ( itr == d->documentRoot->end() ) {
            if ( !d->autoAddSections )
                return false;
            QHash<QString, XMLPreferencesPrivate::EncVariant> map;
            itr = d->documentRoot->insert( section, map );
        }

        XMLPreferencesPrivate::EncVariant v;
        v.data = value;
        itr.value().insert( name, v );
    }

    return true;
}

#endif // QT_CORE_ONLY

/*!
	Clears all the data.
 */
void XMLPreferences::clear()
{
    delete d->documentRoot;
    d->documentRoot = 0;
    delete d->defaultSection;
    d->defaultSection = 0;
}

/*!
	Sets the company name if \a name is not empty.
 */
void XMLPreferences::setCompany( const QString& name )
{
    if ( !name.isEmpty() ) {
        delete d->appCompany;
        d->appCompany = new QString( name );
    }
}

/*!
	Sets the product version.
 */
void XMLPreferences::setVersion( const QString& version )
{
    d->checkVersion( version );
}

/*!
	Sets the product version.
 */
void XMLPreferences::setVersion( int major, int minor )
{
    d->versionMajor = ( major < 0 ) ? 0 : major;
    d->versionMinor = ( minor < 0 ) ? 0 : minor;
}


/*!
	Sets the name of the default section if \a name is not empty.
 */
void XMLPreferences::setDefaultSection( const QString& name )
{
    if ( !name.isEmpty() ) {
        delete d->defaultSectionName;
        d->defaultSectionName = new QString( name );
    }
}

/*!
	Returns true if nonexistent sections are automatically created when items are going
	to be added into them
 */
bool XMLPreferences::autoAddSections() const
{
    return d->autoAddSections;
}

/*!
	If enabled, automatically creates nonexistent sections when items are going
	to be added into them
*/
void XMLPreferences::setAutoAddSections( bool enable )
{
    d->autoAddSections = enable;
}
