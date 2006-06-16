/*
*   Copyright (C) 2006 by Alexander Saal <alex.saal@gmx.de>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or (at
*   your option) any later version.
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software Foundation,
*   Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/* Project include */
#include "qcr_chipcard.h"

/* ChipCard2 include */
#include <gwenhywfar/db.h>
#include <chipcard2/version.h>
#include <chipcard2/chipcard2.h>
#include <chipcard2-client/client/card.h>
#include <chipcard2-client/client/client.h>
#include <chipcard2-client/cards/ddvcard.h>
#include <chipcard2-client/cards/geldkarte.h>
#include <chipcard2-client/cards/kvkcard.h>
#include <chipcard2-client/cards/memorycard.h>
#include <chipcard2-client/cards/processorcard.h>
#include <chipcard2-client/cards/starcos.h>

/* 3rdparty include */
#include "XMLPreferences.h"

/* Qt4 include files - QtGui */
#include <QtGui>

/* Qt4 include files - QtCore */
#include <QtCore>

QCRChipCard::QCRChipCard()
{}

QCRChipCard::~QCRChipCard()
{
    kvkCardData.clear();
    delete qlwItem;
}

/*
 * TODO Public
 */
QString QCRChipCard::getDDVCardData( QListWidget *qlw )
{
    return "";
}

QMap<QString, QString> QCRChipCard::getKVKCardData( QListWidget *qlw )
{
    qlw->clear();

    LC_CLIENT * cl;
    LC_CARD *card = 0;
    LC_CLIENT_RESULT res;
    GWEN_DB_NODE *dbData;
    int rv;

    cl = LC_Client_new( "QCardReader", "0.0.1", 0 );
    if ( LC_Client_ReadConfigFile( cl, 0 ) ) {
        QMessageBox::critical( 0, "QCardReader", "Can't read the configuration." );
        deinit( card, res );
        LC_Client_free( cl );
        return kvkCardData;
    }

    res = LC_Client_StartWait( cl, 0, 0 );
    if ( res != LC_Client_ResultOk ) {
        errorMsg( card, res, qlw );
        deinit( card, res );
        return kvkCardData;
    }

    QMessageBox::information( 0, "QCardReader", "Please insert a German medical card." );
    card = LC_Client_WaitForNextCard( cl, 30 );
    if ( !card ) {
        QMessageBox::critical( 0, "QCardReader", "No card found." );
        errorMsg( card, res, qlw );
        LC_Client_StopWait( cl );
        deinit( card, res );
        return kvkCardData;
    }

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO: Telling the server that we need no more cards ..." );

    res = LC_Client_StopWait( cl );
    if ( res != LC_Client_ResultOk ) {
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
        deinit( card, res );
        return kvkCardData;
    }

    rv = LC_KVKCard_ExtendCard( card );
    if ( rv ) {
        QMessageBox::warning( 0, "QCardReader", "Could not extend card as German medical card, abort" );
        deinit( card, res );
        return kvkCardData;
    }

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Opening card ..." );

    res = LC_Card_Open( card );
    if ( res != LC_Client_ResultOk ) {
        QMessageBox::critical( 0, "QCardReader", "Error executing command CardOpen." );
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
        deinit( card, res );
        return kvkCardData;
    }

    dbData = LC_KVKCard_GetCardData( card );
    if ( !dbData ) {
        QMessageBox::critical( 0, "QCardReader", "No card data available." );
        deinit( card, res );
        return kvkCardData;
    }

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Get card data ..." );

    QString tmpFile;
    tmpFile = QDir::homePath();
    tmpFile.append( "/.qcardreader/kvkcard.tmp" );

    GWEN_DB_WriteFile( dbData, tmpFile.toAscii(), GWEN_DB_FLAGS_OVERWRITE_VARS );
    
    QFile f( tmpFile );

    if ( !f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
        return kvkCardData;
    }

    QTextStream in( &f );
    QString line;
    QStringList tmpList, tmpList1;
    
    while ( !in.atEnd() ) {
        line = in.readLine();
        tmpList = line.split( "=" );
        tmpList1 = tmpList.value( 0 ).split( " " );
        kvkCardData[ tmpList1.value( 1 ) ] = tmpList.value( 1 );
    }

    f.close();    
    f.remove();    
    
    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Get card data ... done" );
   
    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Close card and free memory ..." );
    deinit( card, res );
    LC_Client_free( cl );
    return kvkCardData;
}

QString QCRChipCard::getMemoryCardData( QListWidget *qlw )
{
    return "";
}

QStringList QCRChipCard::getMoneyCardData( QListWidget *qlw )
{
    return moneyCardData;
}

QString QCRChipCard::getProcessorCardData( QListWidget *qlw )
{
    return "";
}

/*
 * TODO Private
 */
bool QCRChipCard::deinit( LC_CARD *card, LC_CLIENT_RESULT res )
{
    res = LC_Card_Close( card );
    if ( res == LC_Client_ResultOk ) {
        LC_Card_free( card );
        return true;
    }

    LC_Card_free( card );
    return false;
}

QString QCRChipCard::errorMsg( LC_CARD *card, LC_CLIENT_RESULT res, QListWidget *qlw )
{
    QString s;

    switch ( res ) {
        case LC_Client_ResultOk:
            s = "Ok.";
            break;
        case LC_Client_ResultWait:
            s = "Timeout.";
            break;
        case LC_Client_ResultIpcError:
            s = "IPC error.";
            break;
        case LC_Client_ResultCmdError:
            s = "Command error.";
            break;
        case LC_Client_ResultDataError:
            s = "Data error.";
            break;
        case LC_Client_ResultAborted:
            s = "Aborted.";
            break;
        case LC_Client_ResultInvalid:
            s = "Invalid argument to command.";
            break;
        case LC_Client_ResultInternal:
            s = "Internal error.";
            break;
        case LC_Client_ResultGeneric:
            s = "Generic error.";
            break;
        default:
            s = "Unknown error.";
            break;
    }

    if ( res == LC_Client_ResultCmdError ) {
        int sw1;
        int sw2;

        sw1 = LC_Card_GetLastSW1( card );
        sw2 = LC_Card_GetLastSW2( card );
        qlwItem = new QListWidgetItem( qlw );
        if ( sw1 != -1 && sw2 != -1 )
            qlwItem->setText( "Last card command result: SW1= " + QString( sw1 ) + ", SW2 = " + QString( sw2 ) );

        s = LC_Card_GetLastResult( card );
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "Result: " + s );

        s = LC_Card_GetLastText( card );
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "Text: " + s );
    }
    return s;
}
