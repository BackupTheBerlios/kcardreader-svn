/****************************************************************************
** Filename: xmlwriter.h
** Last updated [dd/mm/yyyy]: 07/05/2006
**
** Class for XML file generation.
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

#ifndef OSDAB_XMLWRITER_H
#define OSDAB_XMLWRITER_H

#include <QHash>

class QIODevice;
class QTextCodec;

class XMLWriterPrivate;

class XMLWriter
{
    public:
        XMLWriter( QIODevice* device, QTextCodec* codec = 0, bool writeEncoding = true );
        virtual ~XMLWriter();

        enum LineBreakType { Unix, Windows, Macintosh };

        virtual void writeString( const QString& string );
        virtual void writeLine();

        virtual void writeOpenTag( const QString& name, const QHash<QString, QString>* attrs = 0 );
        virtual void writeCloseTag( const QString& name );
        virtual void writeAtomTag( const QString& name, const QHash<QString, QString>* attrs = 0 );
        virtual void writeTaggedString( const QString& name, const QString& string, const QHash<QString, QString>* attrs = 0 );

        virtual void writeComment( const QString& comment );
        virtual void startComment();
        virtual void endComment();

        virtual LineBreakType lineBreakType() const;
        virtual void setLineBreakType( LineBreakType type );

        virtual bool pauseIndent() const;
        virtual void setPauseIndent( bool pause );

        virtual void writeCurrentIndent();

        virtual int indentType() const;
        virtual void setIndentType( int spaces );

        virtual bool autoNewLine() const;
        virtual void setAutoNewLine( bool on );

        virtual bool skipEmptyTags() const;
        virtual void setSkipEmptyTags( bool skip );

        virtual bool skipEmptyAttributes() const;
        virtual void setSkipEmptyAttributes( bool skip );

    private:
        XMLWriterPrivate* d;
};

#endif // OSDAB_XMLWRITER_H
