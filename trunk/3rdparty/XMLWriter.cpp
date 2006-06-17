/****************************************************************************
** Filename: XMLWriter.cpp
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

#include <QTextCodec>
#include <QTextStream>
#include <QIODevice>
#include <QRegExp>

#include "XMLWriter.h"

/*!
	\class XMLWriter xmlwriter.h
	\ingroup OSDaB
 
	\brief The XMLWriter class provides a convenient interface for
	writing XML files.
*/

/*!
	\enum XMLWriter::LineBreakType The type of line breaks to be used by this writer.
 
	\value Unix Unix-style \\n only
	\value Windows Windows-style \\r\\n
	\value Macintosh Mac-style \\r only
*/


/************************************************************************
XMLWriterPrivate interface
*************************************************************************/

//! \internal
class XMLWriterPrivate
{
    public:
        XMLWriterPrivate( QIODevice* device, QTextCodec* codec, bool writeEncoding );
        ~XMLWriterPrivate();

        QString escape( const QString& str ) const;
        QString openTag( const QString& tag, const QHash<QString, QString>* attributes );

        QTextStream* stream;
        QString lineBreak;
        QString indentString;
        bool autoLineBreak;
        bool lineStart;
        bool pauseIndent;
        int indentLevel;
        bool skipEmptyTags;
        bool skipEmptyAttributes;
};

//! \internal
XMLWriterPrivate::XMLWriterPrivate( QIODevice* device, QTextCodec* codec, bool writeEncoding )
{
    stream = new QTextStream( device );
    if ( codec == 0 )
        stream->setCodec( "UTF-8" );
    else
        stream->setCodec( codec );

    indentString = "\t";
    autoLineBreak = true;
    lineBreak = "\r\n";
    indentLevel = 0;
    pauseIndent = false;
    skipEmptyTags = false;
    skipEmptyAttributes = false;

    // <?xml version="1.0" encoding="SELECTED_ENCODING"?>
    if ( writeEncoding )
        ( *stream ) << "<?xml version=\"1.0\" encoding=\"" << escape( QString( stream->codec() ->name() ) ) << "\"?>" << lineBreak << lineBreak;
}

//! \internal
XMLWriterPrivate::~XMLWriterPrivate()
{
    stream->flush();
    delete stream;
}

//! \internal
QString XMLWriterPrivate::escape( const QString& string ) const
{
    QString s = string;
    s.replace( "&", "&amp;" );
    s.replace( ">", "&gt;" );
    s.replace( "<", "&lt;" );
    s.replace( "\"", "&quot;" );
    s.replace( "\'", "&apos;" );
    return s;
}

//! \internal
QString XMLWriterPrivate::openTag( const QString& tag, const QHash<QString, QString>* attributes )
{
    QString out = "<" + escape( tag );

    if ( attributes != 0 ) {
        for ( QHash<QString, QString>::ConstIterator itr = attributes->constBegin(); itr != attributes->constEnd(); ++itr ) {
            if ( skipEmptyAttributes && itr.value().isEmpty() )
                continue;

            out += " " + escape( itr.key() ) + "=\"" + escape( itr.value() ) + "\"";
        }
    }

    out += ">";
    return out;
}


/************************************************************************
Public interface
*************************************************************************/

/*!
	Builds a new XML writer for \a device using \a codec to encode
	text. A first line with XML encoding type is written if \a writeEncoding is true.
	UTF-8 encoding is used if \a codec is 0.
	Indenting uses the ASCII horizontal tab char (\t) by default.
	CRLF is automatically added after each tag. This behavior can be changed by
	calling \link setAutoNewLine(bool)
*/
XMLWriter::XMLWriter( QIODevice* device, QTextCodec* codec, bool writeEncoding )
{
    d = new XMLWriterPrivate( device, codec, writeEncoding );
}

/*!
	Flushes any remaining buffered data and destroys this object.
*/
XMLWriter::~XMLWriter()
{
    delete d;
}

/*!
	Returns the type of line breaks used by this writer.
*/
XMLWriter::LineBreakType XMLWriter::lineBreakType() const
{
    return ( d->lineBreak == "\r" ? Macintosh : d->lineBreak == "\n" ? Unix : Windows );
}

/*!
	Sets the type of line breaks the writer should use to either Unix (LF only),
	Windows (CRLF), or Mac (CR only) style.
*/
void XMLWriter::setLineBreakType( LineBreakType type )
{
    switch ( type ) {
        case Macintosh:
            d->lineBreak = "\r";
            break;
        case Windows:
            d->lineBreak = "\r\n";
            break;
        case Unix:
            d->lineBreak = "\n";
    }
}

/*!
	Writes a string to the stream encoding not allowed chars.
*/
void XMLWriter::writeString( const QString& string )
{
    ( *d->stream ) << d->escape( string );
}

/*!
	Writes an opening tag named \a name containing attributes from the \a attrs hash.
	No attributes are added if \a attrs is 0.
	Example: \verbatim <itemName attr1="value1"> \endverbatim
*/
void XMLWriter::writeOpenTag( const QString& name, const QHash<QString, QString>* attrs )
{
    if ( !d->pauseIndent )
        for ( int i = 0; i < d->indentLevel; ++i )
            ( *d->stream ) << d->indentString;

    d->indentLevel++;

    ( *d->stream ) << d->openTag( name, attrs );

    if ( d->autoLineBreak )
        ( *d->stream ) << d->lineBreak;
}

/*!
	Writes a closing tag named \a name.
	Example: \verbatim </itemName> \endverbatim
*/
void XMLWriter::writeCloseTag( const QString& name )
{
    d->indentLevel--;

    if ( !d->pauseIndent )
        for ( int i = 0; i < d->indentLevel; ++i )
            ( *d->stream ) << d->indentString;

    ( *d->stream ) << "</" << d->escape( name ) << ">";
    if ( d->autoLineBreak )
        ( *d->stream ) << d->lineBreak;
}

/*!
	Writes an atom named \a name with attributes from hash \a attrs.
	No attributes are added if \a attrs is 0.
	Example: \verbatim <itemName attr1="value1"/> \endverbatim
*/
void XMLWriter::writeAtomTag( const QString& name, const QHash<QString, QString>* attrs )
{
    if ( !d->pauseIndent )
        for ( int i = 0; i < d->indentLevel; ++i )
            ( *d->stream ) << d->indentString;

    QString atom = d->openTag( name, attrs );
    atom.truncate( atom.length() - 1 );
    ( *d->stream ) << atom << "/>";

    if ( d->autoLineBreak )
        ( *d->stream ) << d->lineBreak;
}

/*!
	Writes an opening tag, a string and a closing tag.
	No attributes are added if \a attrs is 0.
	Example: \verbatim <itemName attr1="value1">Some funny text</itemName> \endverbatim
*/
void XMLWriter::writeTaggedString( const QString& name, const QString& string, const QHash<QString, QString>* attrs )
{
    if ( d->skipEmptyTags && string.isEmpty() )
        return ;

    if ( !d->pauseIndent )
        for ( int i = 0; i < d->indentLevel; ++i )
            ( *d->stream ) << d->indentString;

    ( *d->stream ) << d->openTag( name, attrs ) << d->escape( string ) << "</" << d->escape( name ) << ">";

    if ( d->autoLineBreak )
        ( *d->stream ) << d->lineBreak;
}

/*!
	Writes a comment tag.
	Example: \verbatim <!-- comment string --> \endverbatim
*/
void XMLWriter::writeComment( const QString& comment )
{
    if ( !d->pauseIndent )
        for ( int i = 0; i < d->indentLevel; ++i )
            ( *d->stream ) << d->indentString;

    // we do not want the comments to end before WE want it ;)
    QString com( comment );
    com.replace( QString( "-->" ), QString( "==>" ) );

    ( *d->stream ) << "<!-- " << com << " -->";

    if ( d->autoLineBreak )
        ( *d->stream ) << d->lineBreak;
}

/*
	Starts a comment tag. No line break is added.
*/
void XMLWriter::startComment()
{
    if ( !d->pauseIndent )
        for ( int i = 0; i < d->indentLevel; ++i )
            ( *d->stream ) << d->indentString;

    ( *d->stream ) << "<!-- ";
}

/*!
	Closes a comment tag. No line break or indent is added.
*/
void XMLWriter::endComment()
{
    ( *d->stream ) << " -->";
}

/*!
	Outputs a line break using the style set with \link
	XMLWriter::setLineBreakType(LineBreakType)
*/
void XMLWriter::writeLine()
{
    ( *d->stream ) << d->lineBreak;
}

/*!
	Writes out some indent (level is determined by previous tags).
	Ignores any previous call to XMLWriter::pauseIndent(bool).
*/
void XMLWriter::writeCurrentIndent()
{
    for ( int i = 0; i < d->indentLevel; ++i )
        ( *d->stream ) << d->indentString;
}

/*!
	Returns true if indent has been previously suspended by calling
	\link XMLWriter::setPauseIndent(bool)
*/
bool XMLWriter::pauseIndent() const
{
    return d->pauseIndent;
}

/*!
	Stops writing out indents.
	Indentation level is still recorded as tags get opened or closed.
*/
void XMLWriter::setPauseIndent( bool pause )
{
    d->pauseIndent = pause;
}

/*!
	Returns the number of space chars used for indenting or -1 if the ASCII HTAB char is used.
*/
int XMLWriter::indentType() const
{
    return d->indentString == "\t" ? -1 : d->indentString.length();
}

/*!
	Sets the number of spaces to use for indent. Uses ASCII HTAB "\t" tabs if \a spaces
	is negative.
*/
void XMLWriter::setIndentType( int spaces )
{
    if ( spaces < 0 )
        d->indentString = "\t";
    else {
        d->indentString.clear();
        d->indentString.fill( ' ', spaces );
    }
}

/*!
	Return true if a line break is automatically added after each tag.
*/
bool XMLWriter::autoNewLine() const
{
    return d->autoLineBreak;
}

/*!
	Automatically adds a line break after each tag if \a on is true.
*/
void XMLWriter::setAutoNewLine( bool on )
{
    d->autoLineBreak = on;
}

/*!
	Returns true if empty tags should not be written (i.e. no tag is written if you call
	writeTaggedString() passing an empty string). Default behavior is to write empty tags too.
	Only affects tags added by
	XMLWriter::writeTaggedString(const QString&, const QString&, const QHash<QString,QString>*)
*/
bool XMLWriter::skipEmptyTags() const
{
    return d->skipEmptyTags;
}

/*!
	Sets whether empty tags should be written or not.
*/
void XMLWriter::setSkipEmptyTags( bool skip )
{
    d->skipEmptyTags = skip;
}

/*!
	Returns true if empty attributes should not be written.
	Default behavior is to write empty attributes too.
*/
bool XMLWriter::skipEmptyAttributes() const
{
    return d->skipEmptyAttributes;
}

/*!
	Sets whether empty attributes should be written or not.
*/
void XMLWriter::setSkipEmptyAttributes( bool skip )
{
    d->skipEmptyAttributes = skip;
}
