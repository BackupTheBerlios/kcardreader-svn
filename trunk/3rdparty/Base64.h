/****************************************************************************
** Filename: Base64.h
** Last updated [dd/mm/yyyy]: 29/09/2005
**
** Class for encoding/decoding of text using base 64 coding [Requires Qt4 - QtCore module].
**
** See RFC 2045 for details on base 64 coding.
** Code is based on the original GPLed work by Dawit Alemayehu and Rik Hemsley
** (KDE development team). Many changes have been made and some optimization
** should have been reached. Original version did not ignore padding-chars
** while decoding - see RFC 2045 ;)
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

#ifndef BASE64_H
#define BASE64_H

class QString;
class QBitArray;
class QByteArray;

class Base64 {
    public:
        static QByteArray decodeString( const QString& encoded );
        static QBitArray decodeString( const QString& encoded, unsigned int size );
        static QString encodeString( const QByteArray& decoded, bool limitLines = false );
        static QString encodeString( const QBitArray& decoded, bool limitLines = false );

    private:
        static const char mEncode[ 64 ];
        static const char mDecode[ 128 ];
};

#endif // BASE64_H
