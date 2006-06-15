/****************************************************************************
** Filename: XMLPreferences.cpp
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

#include <XMLPreferences.h>
#include <Base64.h>
#include <XMLWriter.h>

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtCore/QBitArray>
#include <QtCore/QCoreApplication>

#ifndef QT_CORE_ONLY
#include <QtGui/QColor>
#endif

#include <QtXml/QDomDocument>
#include <QtXml/QDomNode>
#include <QtXml/QDomElement>


/*
 * Name and company are used in the xml file header.
 * Application file name is used if name is empty.
 *
 * Adding support for new data types involves the following steps:
 *  - add the get and set methods to the header file interface
 *  - implement a getTYPE method; this usually involves
 *     - changing the type returned in case of error
 *     - changing QVariant::TYPE to your type
 *     - changing return itr.value().data.toTYPE() to your type
 *  - implement a setTYPE method (usually tou only need to change the method parameters)
 *  - add a case entry to the writeSection() method to serialize the data type to a string
 *  - add an else if entry to the parseSection() method to deserialize the data type from a string
 */
XMLPreferences::XMLPreferences( const QString& name, const QString& company ) {
    mRoot = 0;
    mDefaultMap = 0;
    mDefaultSection = new QString( "default" );
    mAutoAddSections = true;

    if ( name.isEmpty() ) {
        QString myself = QCoreApplication::applicationFilePath();
        myself = myself.section( "/", -1, -1 );
        if ( myself.toLower().endsWith( ".exe" ) )
            myself.truncate( myself.length() - 4 );

        if ( myself.isEmpty() )
            mName = new QString( name );
        else
            mName = new QString( myself );
    } else
        mName = new QString( name );

    if ( company.isEmpty() )
        mCompany = 0;
    else
        mCompany = new QString( company );

    mIgnoreMissingVersion = true;
    mVersionMajor = -1;
    mVersionMinor = -1;
}


/*
 * Frees all the allocated data.
 */
XMLPreferences::~XMLPreferences() {
    delete mRoot;
    delete mDefaultMap;

    delete mName;
    delete mCompany;

    delete mDefaultSection;
}


/*
 * Returns true if there is no data in this handler.
 */
bool XMLPreferences::isNull() {
    return ( mRoot == 0 );
}

/*
 * Attempts to load settings from a file.
 */
bool XMLPreferences::load( const QString& filename ) {
    QFile file( filename );

    if ( !file.open( QIODevice::ReadOnly ) )
        return false;

    bool res = load( &file );
    file.close();
    return res;
}


bool XMLPreferences::load( QFile* file ) {
    if ( file == 0 )
        return false;

    QDomDocument doc;
    if ( !doc.setContent( file, false ) )
        return false;

    QDomElement root = doc.documentElement();
    if ( root.isNull() )
        return false;

    if ( root.tagName() != "XMLPreferences" )
        return false;

    QString attribute;

    // get file version
    attribute = root.attribute( "update" );
    if ( !checkVersion( attribute ) )
        return false;

    if ( mRoot == 0 )
        init();

    QDomNode node = root.firstChild();
    QDomElement el;

    while ( !node.isNull() ) {
        el = node.toElement();

        if ( ( el.tagName() == "section" ) && el.hasChildNodes() ) {
            QString sName = el.attribute( "name" );
            if ( sName.isEmpty() ) {
                node = node.nextSibling();
                continue;
            }

            if ( sName == ( *mDefaultSection ) )
                parseSection( el, *mDefaultMap );
            else {
                // find or create the right section
                QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->find( sName );
                if ( itr == mRoot->end() ) {
                    // create a new section
                    QMap<QString, EncVariant> section;
                    parseSection( el, section );
                    mRoot->insert( sName, section );
                } else
                    mRoot->insert( sName, itr.value() );
            }
        }

        node = node.nextSibling();

    } // while


    return true;
}


/*
 * Attempts to write the settings to a file.
 */
bool XMLPreferences::save( const QString& filename ) {
    QFile file( filename );
    if ( !file.open( QIODevice::WriteOnly ) )
        return false;

    bool res = save( &file );
    file.close();
    return res;
}

bool XMLPreferences::save( QFile* file ) {
    if ( file == 0 )
        return false;

    if ( mRoot == 0 )
        init();

    XMLWriter out( file );
    QMap<QString, QString>* attrs = new QMap<QString, QString>();

    attrs->insert( "version", "1.0" );
    out.writeOpenTag( "XMLPreferences", attrs );
    attrs->clear();

    if ( mCompany != 0 )
        attrs->insert( "company", *mCompany );
    if ( mVersionMajor > 0 )
        attrs->insert( "version", QString::number( mVersionMajor ) + "." + QString::number( mVersionMinor ) );
    attrs->insert( "product", *mName );
    out.writeAtomTag( "software_info", attrs );

    delete attrs;

    // DEFAULT SECTION
    if ( mDefaultMap != 0 )
        if ( !mDefaultMap->isEmpty() )
            writeSection( out, *mDefaultSection, *mDefaultMap );

    QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->begin();

    while ( itr != mRoot->end() ) {
        writeSection( out, itr.key(), itr.value() );
        ++itr;
    }

    out.writeCloseTag( "XMLPreferences" );

    return true;
}


/*
 * Reads the string value associated to the given name in the given section.
 * Returns QString() if no such value could be found.
 */
QString XMLPreferences::getString( const QString& section, const QString& name ) {
    if ( mRoot == 0 )
        return QString();

    if ( section.isEmpty() ) {
        QMap<QString, EncVariant>::Iterator itr = mDefaultMap->find( name );
        if ( itr == mDefaultMap->end() ) {
            // no such value
            return QString();
        } else {
            if ( itr.value().data.type() != QVariant::String )
                return QString();
            else
                return itr.value().data.toString();
        }
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator sec = mRoot->find( section );
        if ( sec == mRoot->end() ) {
            // no such section
            return QString();
        } else {
            QMap<QString, EncVariant>::Iterator itr = sec.value().find( name );
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


/*
 * Reads the string list associated to the given name in the given section.
 * Returns an empty QStringList if no such value could be found.
 */
QStringList XMLPreferences::getStringList( const QString& section, const QString& name ) {
    if ( mRoot == 0 )
        return QStringList();

    if ( section.isEmpty() ) {
        QMap<QString, EncVariant>::Iterator itr = mDefaultMap->find( name );
        if ( itr == mDefaultMap->end() ) {
            // no such value
            return QStringList();
        } else {
            if ( itr.value().data.type() != QVariant::StringList )
                return QStringList();
            else
                return itr.value().data.toStringList();
        }
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator sec = mRoot->find( section );
        if ( sec == mRoot->end() ) {
            // no such section
            return QStringList();
        } else {
            QMap<QString, EncVariant>::Iterator itr = sec.value().find( name );
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


/*
 * Reads the bool value associated to the given name in the given section.
 * ok is set to false if no value could be found and ok is not null.
 */
bool XMLPreferences::getBool( const QString& section, const QString& name, bool* ok ) {
    if ( mRoot == 0 ) {
        if ( ok != 0 )
            * ok = false;
        return false;
    }

    if ( ok != 0 )
        * ok = true;

    if ( section.isEmpty() ) {
        QMap<QString, EncVariant>::Iterator itr = mDefaultMap->find( name );
        if ( itr == mDefaultMap->end() ) {
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
        QMap<QString, QMap<QString, EncVariant> >::Iterator sec = mRoot->find( section );
        if ( sec == mRoot->end() ) {
            // no such section
            if ( ok != 0 )
                * ok = false;
            return false;
        } else {
            QMap<QString, EncVariant>::Iterator itr = sec.value().find( name );
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


/*
 * Reads the int value associated to the given name in the given section.
 * ok is set to false if no value could be found and ok is not null.
 */
int XMLPreferences::getInt( const QString& section, const QString& name, bool* ok ) {
    if ( mRoot == 0 ) {
        if ( ok != 0 )
            * ok = false;
        return 0;
    }

    if ( ok != 0 )
        * ok = true;

    if ( section.isEmpty() ) {
        QMap<QString, EncVariant>::Iterator itr = mDefaultMap->find( name );
        if ( itr == mDefaultMap->end() ) {
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
        QMap<QString, QMap<QString, EncVariant> >::Iterator sec = mRoot->find( section );
        if ( sec == mRoot->end() ) {
            // no such section
            if ( ok != 0 )
                * ok = false;
            return 0;
        } else {
            QMap<QString, EncVariant>::Iterator itr = sec.value().find( name );
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


/*
 * Reads the qint64 value associated to the given name in the given section.
 * ok is set to false if no value could be found and ok is not null.
 */
qint64 XMLPreferences::getInt64( const QString& section, const QString& name, bool* ok ) {
    if ( mRoot == 0 ) {
        if ( ok != 0 )
            * ok = false;
        return Q_INT64_C( 0 );
    }

    if ( ok != 0 )
        * ok = true;

    if ( section.isEmpty() ) {
        QMap<QString, EncVariant>::Iterator itr = mDefaultMap->find( name );
        if ( itr == mDefaultMap->end() ) {
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
        QMap<QString, QMap<QString, EncVariant> >::Iterator sec = mRoot->find( section );
        if ( sec == mRoot->end() ) {
            // no such section
            if ( ok != 0 )
                * ok = false;
            return Q_INT64_C( 0 );
        } else {
            QMap<QString, EncVariant>::Iterator itr = sec.value().find( name );
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


/*
 * Reads the QBitArray value associated to the given name in the given section.
 * Returns a default object of this type if no value could be found.
 */
QBitArray XMLPreferences::getBitArray( const QString& section, const QString& name ) {
    if ( mRoot == 0 )
        return QBitArray();

    if ( section.isEmpty() ) {
        QMap<QString, EncVariant>::Iterator itr = mDefaultMap->find( name );
        if ( itr == mDefaultMap->end() ) {
            // no such value
            return QBitArray();
        } else {
            if ( itr.value().data.type() != QVariant::BitArray )
                return QBitArray();
            else
                return itr.value().data.toBitArray();
        }
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator sec = mRoot->find( section );
        if ( sec == mRoot->end() ) {
            // no such section
            return QBitArray();
        } else {
            QMap<QString, EncVariant>::Iterator itr = sec.value().find( name );
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


/*
 * Reads the QByteArray value associated to the given name in the given section.
 * Returns a default object of this type if no value could be found.
 */
QByteArray XMLPreferences::getByteArray( const QString& section, const QString& name ) {
    if ( mRoot == 0 )
        return QByteArray();

    if ( section.isEmpty() ) {
        QMap<QString, EncVariant>::Iterator itr = mDefaultMap->find( name );
        if ( itr == mDefaultMap->end() ) {
            // no such value
            return QByteArray();
        } else {
            if ( itr.value().data.type() != QVariant::ByteArray )
                return QByteArray();
            else
                return itr.value().data.toByteArray();
        }
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator sec = mRoot->find( section );
        if ( sec == mRoot->end() ) {
            // no such section
            return QByteArray();
        } else {
            QMap<QString, EncVariant>::Iterator itr = sec.value().find( name );
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


/*
 * Reads the QRect value associated to the given name in the given section.
 * A null QRect is returned if no such value exists.
 */
QRect XMLPreferences::getRect( const QString& section, const QString& name ) {
    if ( mRoot == 0 ) {
        return QRect();
    }

    if ( section.isEmpty() ) {
        QMap<QString, EncVariant>::Iterator itr = mDefaultMap->find( name );
        if ( itr == mDefaultMap->end() ) {
            // no such value
            return QRect();
        } else {
            if ( itr.value().data.type() != QVariant::Rect ) {
                return QRect();
            } else
                return itr.value().data.toRect();
        }
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator sec = mRoot->find( section );
        if ( sec == mRoot->end() ) {
            // no such section
            return QRect();
        } else {
            QMap<QString, EncVariant>::Iterator itr = sec.value().find( name );
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


/*
 * Reads the QPoint value associated to the given name in the given section.
 * A null QPoint is returned if no such value exists.
 */
QPoint XMLPreferences::getPoint( const QString& section, const QString& name ) {
    if ( mRoot == 0 ) {
        return QPoint();
    }

    if ( section.isEmpty() ) {
        QMap<QString, EncVariant>::Iterator itr = mDefaultMap->find( name );
        if ( itr == mDefaultMap->end() ) {
            // no such value
            return QPoint();
        } else {
            if ( itr.value().data.type() != QVariant::Point ) {
                return QPoint();
            } else
                return itr.value().data.toPoint();
        }
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator sec = mRoot->find( section );
        if ( sec == mRoot->end() ) {
            // no such section
            return QPoint();
        } else {
            QMap<QString, EncVariant>::Iterator itr = sec.value().find( name );
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


/*
 * Reads the QSize value associated to the given name in the given section.
 * A null QSize is returned if no such value exists.
 */
QSize XMLPreferences::getSize( const QString& section, const QString& name ) {
    if ( mRoot == 0 ) {
        return QSize();
    }

    if ( section.isEmpty() ) {
        QMap<QString, EncVariant>::Iterator itr = mDefaultMap->find( name );
        if ( itr == mDefaultMap->end() ) {
            // no such value
            return QSize();
        } else {
            if ( itr.value().data.type() != QVariant::Size ) {
                return QSize();
            } else
                return itr.value().data.toSize();
        }
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator sec = mRoot->find( section );
        if ( sec == mRoot->end() ) {
            // no such section
            return QSize();
        } else {
            QMap<QString, EncVariant>::Iterator itr = sec.value().find( name );
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

/*
 * Reads the QColor value associated to the given name in the given section.
 * Returns a default object of this type if no value could be found.
 */
QColor XMLPreferences::getColor( const QString& section, const QString& name ) {
    if ( mRoot == 0 )
        return QColor();

    if ( mRoot == 0 )
        return QColor();

    if ( section.isEmpty() ) {
        QMap<QString, EncVariant>::Iterator itr = mDefaultMap->find( name );
        if ( itr == mDefaultMap->end() ) {
            // no such value
            return QColor();
        } else {
            if ( itr.value().data.type() != QVariant::Color )
                return QColor();
            else
                return itr.value().data.value<QColor>();
        }
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator sec = mRoot->find( section );
        if ( sec == mRoot->end() ) {
            // no such section
            return QColor();
        } else {
            QMap<QString, EncVariant>::Iterator itr = sec.value().find( name );
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

/*
 * Adds a new section to the settings.
 */
void XMLPreferences::addSection( const QString& section ) {
    if ( section.isEmpty() || ( section == *mDefaultSection ) )
        return ;

    if ( mRoot == 0 )
        init();

    if ( mRoot->contains( section ) )
        return ;

    QMap<QString, EncVariant> map;
    mRoot->insert( section, map );
}


/*
 * Removes a section and all its data.
 * Returns false if no such section exists.
 */
bool XMLPreferences::deleteSection( const QString& section ) {
    if ( mRoot == 0 )
        return false;

    if ( section.isEmpty() || ( section == *mDefaultSection ) ) {
        delete mDefaultMap;
        mDefaultMap = new QMap<QString, EncVariant>();
        return true;
    }

    QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->find( section );
    if ( itr == mRoot->end() )
        return false;

    mRoot->erase( itr );
    return true;
}


/*
 * Adds a QString type value to given section and binds it to the given name.
 * Returns false if the value could not be added (invalid value or section or name).
 */
bool XMLPreferences::setString( const QString& section, const QString& name, const QString& value ) {
    if ( name.isEmpty() )
        return false;

    if ( mRoot == 0 )
        init();

    if ( section.isEmpty() || ( section == *mDefaultSection ) ) {
        EncVariant v;
        v.data = QVariant( value );
        mDefaultMap->insert( name, v );
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->find( section );
        if ( itr == mRoot->end() ) {
            if ( !mAutoAddSections )
                return false;
            QMap<QString, EncVariant> map;
            itr = mRoot->insert( section, map );
        }

        EncVariant v;
        v.data = QVariant( value );
        itr.value().insert( name, v );
    }

    return true;
}


/*
 * Adds a QStringList type value to given section and binds it to the given name.
 * Returns false if the value could not be added (invalid value or section or name).
 */
bool XMLPreferences::setStringList( const QString& section, const QString& name, const QStringList& value ) {
    if ( name.isEmpty() )
        return false;

    if ( mRoot == 0 )
        init();

    if ( section.isEmpty() || ( section == *mDefaultSection ) ) {
        EncVariant v;
        v.data = QVariant( value );
        mDefaultMap->insert( name, v );
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->find( section );
        if ( itr == mRoot->end() ) {
            if ( !mAutoAddSections )
                return false;
            QMap<QString, EncVariant> map;
            itr = mRoot->insert( section, map );
        }

        EncVariant v;
        v.data = QVariant( value );
        itr.value().insert( name, v );
    }

    return true;
}


/*
 * Adds an bool type value to given section and binds it to the given name.
 * Returns false if the value could not be added (invalid value or section or name).
 */
bool XMLPreferences::setBool( const QString& section, const QString& name, bool value ) {
    if ( name.isEmpty() )
        return false;

    if ( mRoot == 0 )
        init();

    if ( section.isEmpty() || ( section == *mDefaultSection ) ) {
        EncVariant v;
        v.data = QVariant( value );
        mDefaultMap->insert( name, v );
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->find( section );
        if ( itr == mRoot->end() ) {
            if ( !mAutoAddSections )
                return false;
            QMap<QString, EncVariant> map;
            itr = mRoot->insert( section, map );
        }

        EncVariant v;
        v.data = QVariant( value );
        itr.value().insert( name, v );
    }

    return true;
}


/*
 * Adds an int type value to given section and binds it to the given name.
 * Returns false if the value could not be added (invalid value or section or name).
 */
bool XMLPreferences::setInt( const QString& section, const QString& name, int value ) {
    if ( name.isEmpty() )
        return false;

    if ( mRoot == 0 )
        init();

    if ( section.isEmpty() || ( section == *mDefaultSection ) ) {
        EncVariant v;
        v.data = QVariant( value );
        mDefaultMap->insert( name, v );
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->find( section );
        if ( itr == mRoot->end() ) {
            if ( !mAutoAddSections )
                return false;
            QMap<QString, EncVariant> map;
            itr = mRoot->insert( section, map );
        }

        EncVariant v;
        v.data = QVariant( value );
        itr.value().insert( name, v );
    }

    return true;
}


/*
 * Adds a qint64 type value to given section and binds it to the given name.
 * Returns false if the value could not be added (invalid value or section or name).
 */
bool XMLPreferences::setInt64( const QString& section, const QString& name, qint64 value ) {
    if ( name.isEmpty() )
        return false;

    if ( mRoot == 0 )
        init();

    if ( section.isEmpty() || ( section == *mDefaultSection ) ) {
        EncVariant v;
        v.data = QVariant( ( qlonglong ) value );
        mDefaultMap->insert( name, v );
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->find( section );
        if ( itr == mRoot->end() ) {
            if ( !mAutoAddSections )
                return false;
            QMap<QString, EncVariant> map;
            itr = mRoot->insert( section, map );
        }

        EncVariant v;
        v.data = QVariant( value );
        itr.value().insert( name, v );
    }

    return true;
}


/*
 * Adds a QBitArray type value to given section and binds it to the given name.
 * Returns false if the value could not be added (invalid value or section or name).
 */
bool XMLPreferences::setBitArray( const QString& section, const QString& name, const QBitArray& value, BinEncoding encoding ) {
    if ( name.isEmpty() )
        return false;

    if ( mRoot == 0 )
        init();

    if ( section.isEmpty() || ( section == *mDefaultSection ) ) {
        EncVariant v;
        v.data = QVariant( value );
        v.encoding = encoding;
        mDefaultMap->insert( name, v );
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->find( section );
        if ( itr == mRoot->end() ) {
            if ( !mAutoAddSections )
                return false;
            QMap<QString, EncVariant> map;
            itr = mRoot->insert( section, map );
        }

        EncVariant v;
        v.data = QVariant( value );
        v.encoding = encoding;
        itr.value().insert( name, v );
    }

    return true;
}


/*
 * Adds a QByteArray type value to given section and binds it to the given name.
 * Returns false if the value could not be added (invalid value or section or name).
 */
bool XMLPreferences::setByteArray( const QString& section, const QString& name, const QByteArray& value, BinEncoding encoding ) {
    if ( name.isEmpty() )
        return false;

    if ( mRoot == 0 )
        init();

    if ( section.isEmpty() || ( section == *mDefaultSection ) ) {
        EncVariant v;
        v.data = QVariant( value );
        v.encoding = encoding;
        mDefaultMap->insert( name, v );
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->find( section );
        if ( itr == mRoot->end() ) {
            if ( !mAutoAddSections )
                return false;
            QMap<QString, EncVariant> map;
            itr = mRoot->insert( section, map );
        }

        EncVariant v;
        v.data = QVariant( value );
        v.encoding = encoding;
        itr.value().insert( name, v );
    }

    return true;
}


/*
 * Adds a QRect type value to given section and binds it to the given name.
 * Returns false if the value could not be added (invalid value or section or name).
 */
bool XMLPreferences::setRect( const QString& section, const QString& name, const QRect& value ) {
    if ( name.isEmpty() )
        return false;

    if ( mRoot == 0 )
        init();

    if ( section.isEmpty() || ( section == *mDefaultSection ) ) {
        EncVariant v;
        v.data = value;
        mDefaultMap->insert( name, v );
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->find( section );
        if ( itr == mRoot->end() ) {
            if ( !mAutoAddSections )
                return false;
            QMap<QString, EncVariant> map;
            itr = mRoot->insert( section, map );
        }

        EncVariant v;
        v.data = value;
        itr.value().insert( name, v );
    }

    return true;
}


/*
 * Adds a QPoint type value to given section and binds it to the given name.
 * Returns false if the value could not be added (invalid value or section or name).
 */
bool XMLPreferences::setPoint( const QString& section, const QString& name, const QPoint& value ) {
    if ( name.isEmpty() )
        return false;

    if ( mRoot == 0 )
        init();

    if ( section.isEmpty() || ( section == *mDefaultSection ) ) {
        EncVariant v;
        v.data = value;
        mDefaultMap->insert( name, v );
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->find( section );
        if ( itr == mRoot->end() ) {
            if ( !mAutoAddSections )
                return false;
            QMap<QString, EncVariant> map;
            itr = mRoot->insert( section, map );
        }

        EncVariant v;
        v.data = value;
        itr.value().insert( name, v );
    }

    return true;
}


/*
 * Adds a QSize type value to given section and binds it to the given name.
 * Returns false if the value could not be added (invalid value or section or name).
 */
bool XMLPreferences::setSize( const QString& section, const QString& name, const QSize& value ) {
    if ( name.isEmpty() )
        return false;

    if ( mRoot == 0 )
        init();

    if ( section.isEmpty() || ( section == *mDefaultSection ) ) {
        EncVariant v;
        v.data = value;
        mDefaultMap->insert( name, v );
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->find( section );
        if ( itr == mRoot->end() ) {
            if ( !mAutoAddSections )
                return false;
            QMap<QString, EncVariant> map;
            itr = mRoot->insert( section, map );
        }

        EncVariant v;
        v.data = value;
        itr.value().insert( name, v );
    }

    return true;
}


#ifndef QT_CORE_ONLY

/*
 * Adds a QColor type value to given section and binds it to the given name.
 * Returns false if the value could not be added (invalid value or section or name).
 */
bool XMLPreferences::setColor( const QString& section, const QString& name, const QColor& value ) {
    if ( name.isEmpty() )
        return false;

    if ( mRoot == 0 )
        init();

    if ( section.isEmpty() || ( section == *mDefaultSection ) ) {
        EncVariant v;
        v.data = value;
        mDefaultMap->insert( name, v );
    } else {
        QMap<QString, QMap<QString, EncVariant> >::Iterator itr = mRoot->find( section );
        if ( itr == mRoot->end() ) {
            if ( !mAutoAddSections )
                return false;
            QMap<QString, EncVariant> map;
            itr = mRoot->insert( section, map );
        }

        EncVariant v;
        v.data = value;
        itr.value().insert( name, v );
    }

    return true;
}

#endif // QT_CORE_ONLY

/*
 * Clears all the data.
 */
void XMLPreferences::clear() {
    delete mRoot;
    mRoot = 0;
    delete mDefaultMap;
    mDefaultMap = 0;
}


/*
 * Initializes the document root.
 */
void XMLPreferences::init() {
    delete mRoot;
    delete mDefaultMap;

    // CREATE DOCUMENT ROOT
    mRoot = new QMap<QString, QMap<QString, EncVariant> >();

    // CREATE DEFAULT SECTION
    mDefaultMap = new QMap<QString, EncVariant>();
}


/*
 * Writes out the contents of a section.
 */
void XMLPreferences::writeSection( XMLWriter& out, const QString& name, const QMap<QString, EncVariant>& map ) {
    QMap<QString, EncVariant>::ConstIterator itr = map.constBegin();
    QMap<QString, QString>* attrs = new QMap<QString, QString>();

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
                        case BE_BASE64:
                            attrs->insert( "encoding", "base64" );
                            out.writeTaggedString( "setting", Base64::encodeString( ba ), attrs );
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
                        case BE_BASE64:
                            attrs->insert( "encoding", "base64" );
                            out.writeTaggedString( "setting", Base64::encodeString( ba ), attrs );
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


/*
 * Attempts to parse a major and a minor version number.
 */
bool XMLPreferences::checkVersion( const QString& version ) {
    if ( version.isEmpty() )
        return mIgnoreMissingVersion;

    QString sub = version.section( '.', 0, 0 );
    if ( sub.isEmpty() )
        return false;

    bool ok;
    int tmp = sub.toInt( &ok );
    if ( !ok || ( tmp < 0 ) || ( tmp > 1 ) )
        return false;

    mVersionMajor = tmp;

    sub = version.section( '.', 1, 1 );
    if ( sub.isEmpty() )
        return false;

    tmp = sub.toInt( &ok );
    if ( !ok || ( tmp < 0 ) )
        return false;

    mVersionMinor = tmp;

    return true;
}


/*
 * Attempts to load settings from the given QDomElement and to put them in the given section.
 */
void XMLPreferences::parseSection( const QDomElement& rootEl, QMap<QString, EncVariant>& map ) {
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
                QByteArray ba = Base64::decodeString( el.text() );
                EncVariant v;
                v.data = QVariant( ba );
                v.encoding = BE_BASE64;
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
                v.encoding = BE_CSV;
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
                QBitArray ba = Base64::decodeString( el.text(), arr_sz );
                EncVariant v;
                v.data = QVariant( ba );
                v.encoding = BE_BASE64;
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
                v.encoding = BE_CSV;
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


/*
 * Sets the company name.
 */
void XMLPreferences::setCompany( const QString& name ) {
    if ( !name.isEmpty() ) {
        delete mCompany;
        mCompany = new QString( name );
    }
}


/*
 * Convenience function. Sets the product version.
 */
void XMLPreferences::setVersion( const QString& version ) {
    checkVersion( version );
}


/*
 * Sets the product version.
 */
void XMLPreferences::setVersion( int major, int minor ) {
    mVersionMajor = ( major < 0 ) ? 0 : major;
    mVersionMinor = ( minor < 0 ) ? 0 : minor;
}


/*
 * Sets the name of the default section.
 */
void XMLPreferences::setDefaultSection( const QString& name ) {
    if ( !name.isEmpty() ) {
        delete mDefaultSection;
        mDefaultSection = new QString( name );
    }
}

/*
 * If enabled, automatically creates unexisting sections before adding some item.
 */
bool XMLPreferences::autoAddSections() const {
    return mAutoAddSections;
}

void XMLPreferences::setAutoAddSections( bool enable ) {
    mAutoAddSections = enable;
}
