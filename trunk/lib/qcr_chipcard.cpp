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

/* TODO Constructor */
QCRChipCard::QCRChipCard()
{}

/* TODO Destructor (clear all memory) */
QCRChipCard::~QCRChipCard()
{
    kvkCardData.clear();
    delete qlwItem;
}

/* TODO Get data from ddv card (HBCI Card) */
QString QCRChipCard::getDDVCardData( QListWidget *qlw )
{
    return "";
}

/* TODO Get data from kvk card (German Health Insurance Cards (KVK)) */
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
        deinit( card, cl, res );
        return kvkCardData;
    }

    res = LC_Client_StartWait( cl, 0, 0 );
    if ( res != LC_Client_ResultOk ) {
        errorMsg( card, res, qlw );
        deinit( card, cl, res );
        return kvkCardData;
    }

    QMessageBox::information( 0, "QCardReader", "Please insert a German medical card." );
    card = LC_Client_WaitForNextCard( cl, 30 );
    if ( !card ) {
        QMessageBox::critical( 0, "QCardReader", "No card found." );
        errorMsg( card, res, qlw );
        LC_Client_StopWait( cl );
        deinit( card, cl, res );
        return kvkCardData;
    }

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO: Telling the server that we need no more cards ..." );

    res = LC_Client_StopWait( cl );
    if ( res != LC_Client_ResultOk ) {
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
        deinit( card, cl, res );
        return kvkCardData;
    }

    rv = LC_KVKCard_ExtendCard( card );
    if ( rv ) {
        QMessageBox::warning( 0, "QCardReader", "Could not extend card as German medical card, abort" );
        deinit( card, cl, res );
        return kvkCardData;
    }

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Opening card ..." );

    res = LC_Card_Open( card );
    if ( res != LC_Client_ResultOk ) {
        QMessageBox::critical( 0, "QCardReader", "Error executing command CardOpen." );
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
        deinit( card, cl, res );
        return kvkCardData;
    }

    dbData = LC_KVKCard_GetCardData( card );
    if ( !dbData ) {
        QMessageBox::critical( 0, "QCardReader", "No card data available." );
        deinit( card, cl, res );
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
        deinit( card, cl, res );
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
    deinit( card, cl, res );
    return kvkCardData;
}

/* TODO Get data from memory card */
QString QCRChipCard::getMemoryCardData( QListWidget *qlw )
{
    return "";
}

/* TODO Get version of LibChipCard2 */
QString QCRChipCard::getLibchipcardVersion()
{
    return CHIPCARD_VERSION_FULL_STRING;
}

/* TODO Get data from money card */
QStringList QCRChipCard::getMoneyCardData( QListWidget *qlw )
{
    qlw->clear();

    LC_CLIENT * cl;
    LC_CARD *card = 0;
    LC_CLIENT_RESULT res;

    GWEN_DB_NODE *dbCardData;

    int rv;

    cl = LC_Client_new( "QCardReader", "0.0.1", 0 );
    if ( LC_Client_ReadConfigFile( cl, 0 ) ) {
        QMessageBox::critical( 0, "QCardReader", "Can't read the configuration." );
        deinit( card, cl, res );
        return moneyCardData;
    }

    res = LC_Client_StartWait( cl, 0, 0 );
    if ( res != LC_Client_ResultOk ) {
        errorMsg( card, res, qlw );
        deinit( card, cl, res );
        return moneyCardData;
    }

    QMessageBox::information( 0, "QCardReader", "Please insert a Money card." );
    card = LC_Client_WaitForNextCard( cl, 30 );
    if ( !card ) {
        QMessageBox::critical( 0, "QCardReader", "No card found." );
        errorMsg( card, res, qlw );
        deinit( card, cl, res );
        return moneyCardData;
    }

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO: Telling the server that we need no more cards ..." );

    res = LC_Client_StopWait( cl );
    if ( res != LC_Client_ResultOk ) {
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
        deinit( card, cl, res );
        return moneyCardData;
    }

    rv = LC_GeldKarte_ExtendCard( card );
    if ( rv ) {
        QMessageBox::warning( 0, "QCardReader", "Could not extend card as Money card, abort" );
        deinit( card, cl, res );
        return moneyCardData;
    }

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Opening card ..." );

    /* TODO Open Card */
    res = LC_Card_Open( card );
    if ( res != LC_Client_ResultOk ) {
        QMessageBox::critical( 0, "QCardReader", "Error executing command CardOpen." );
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
        deinit( card, cl, res );
        return moneyCardData;
    }

    /* TODO Read the data of card */
    dbCardData = LC_GeldKarte_GetCardDataAsDb( card );
    if ( !dbCardData ) {
        QMessageBox::critical( 0, "QCardReader", "No card data available." );
        deinit( card, cl, res );
        return moneyCardData;
    }

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Get card data ..." );

    /* TODO Write the data db to file and read it and store the readed file into QMap */
    QString tmpFile;
    tmpFile = QDir::homePath();
    tmpFile.append( "/.qcardreader/moneycard.tmp" );

    GWEN_DB_WriteFile( dbCardData, tmpFile.toAscii(), GWEN_DB_FLAGS_OVERWRITE_VARS );
    QFile f( tmpFile );

    if ( !f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
        deinit( card, cl, res );
        return moneyCardData;
    }

    QTextStream in( &f );
    QString line, value;
    QStringList tmpList, tmpList1;

    while ( !in.atEnd() ) {
        line = in.readLine();
        tmpList = line.split( "=" );
        tmpList1 = tmpList.value( 0 ).split( " " );
        if ( !tmpList1.value( 2 ).isEmpty() ) {
            value = tmpList1.value( 2 );
        } else {
            value = tmpList1.value( 1 );
        }
        moneyCardData += value + ";" + tmpList.value( 1 );
    }

    f.close();
    f.remove();

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Get card data ... done" );

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Close card and free memory ..." );
    deinit( card, cl, res );
    return moneyCardData;
}

/* TODO Get account data from money card */
QStringList QCRChipCard::getMoneyCardAccData( QListWidget *qlw )
{
    qlw->clear();

    LC_CLIENT * cl;
    LC_CARD *card = 0;
    LC_CLIENT_RESULT res;
    GWEN_DB_NODE *dbCardAccData;

    int rv;

    cl = LC_Client_new( "QCardReader", "0.0.1", 0 );
    if ( LC_Client_ReadConfigFile( cl, 0 ) ) {
        QMessageBox::critical( 0, "QCardReader", "Can't read the configuration." );
        deinit( card, cl, res );
        return moneyCardAccData;
    }

    res = LC_Client_StartWait( cl, 0, 0 );
    if ( res != LC_Client_ResultOk ) {
        errorMsg( card, res, qlw );
        deinit( card, cl, res );
        return moneyCardAccData;
    }

    card = LC_Client_WaitForNextCard( cl, 30 );
    if ( !card ) {
        QMessageBox::critical( 0, "QCardReader", "No card found." );
        errorMsg( card, res, qlw );
        deinit( card, cl, res );
        return moneyCardAccData;
    }

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO: Telling the server that we need no more cards ..." );

    res = LC_Client_StopWait( cl );
    if ( res != LC_Client_ResultOk ) {
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
        deinit( card, cl, res );
        return moneyCardAccData;
    }

    rv = LC_GeldKarte_ExtendCard( card );
    if ( rv ) {
        QMessageBox::warning( 0, "QCardReader", "Could not extend card as Money card, abort" );
        deinit( card, cl, res );
        return moneyCardAccData;
    }

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Opening card ..." );

    /* TODO Open Card */
    res = LC_Card_Open( card );
    if ( res != LC_Client_ResultOk ) {
        QMessageBox::critical( 0, "QCardReader", "Error executing command CardOpen." );
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
        deinit( card, cl, res );
        return moneyCardAccData;
    }

    /* TODO Read the account data of card */
    dbCardAccData = LC_GeldKarte_GetAccountDataAsDb( card );
    if ( !dbCardAccData ) {
        QMessageBox::critical( 0, "QCardReader", "No card account data available." );
        deinit( card, cl, res );
        return moneyCardAccData;
    }

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Get card data ..." );

    /* TODO Write the account db to file and read it and store the readed file into QMap */
    QString tmpFile;
    tmpFile = QDir::homePath();
    tmpFile.append( "/.qcardreader/moneycardacc.tmp" );

    GWEN_DB_WriteFile( dbCardAccData, tmpFile.toAscii(), GWEN_DB_FLAGS_OVERWRITE_VARS );
    QFile f( tmpFile );

    if ( !f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
        deinit( card, cl, res );
        return moneyCardAccData;
    }

    QTextStream in( &f );
    QString line, value;
    QStringList tmpList, tmpList1;

    while ( !in.atEnd() ) {
        line = in.readLine();
        tmpList = line.split( "=" );
        tmpList1 = tmpList.value( 0 ).split( " " );
        if ( !tmpList1.value( 2 ).isEmpty() ) {
            value = tmpList1.value( 2 );
        } else {
            value = tmpList1.value( 1 );
        }
        moneyCardAccData += value + ";" + tmpList.value( 1 );
    }

    f.close();
    f.remove();

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Get card data ... done" );

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Close card and free memory ..." );
    deinit( card, cl, res );
    return moneyCardAccData;
}

/* TODO Get money  from money card */
QMap<QString, float> QCRChipCard::getMoneyCardMoneyData( QListWidget *qlw )
{
    qlw->clear();

    LC_CLIENT * cl;
    LC_CARD *card = 0;
    LC_CLIENT_RESULT res;
    LC_GELDKARTE_VALUES *values;
    LC_GELDKARTE_BLOG *transBlog;
    LC_GELDKARTE_BLOG_LIST2 *transBlogList;

    int rv;

    cl = LC_Client_new( "QCardReader", "0.0.1", 0 );
    if ( LC_Client_ReadConfigFile( cl, 0 ) ) {
        QMessageBox::critical( 0, "QCardReader", "Can't read the configuration." );
        deinit( card, cl, res );
        return moneyCardMoneyData;
    }

    res = LC_Client_StartWait( cl, 0, 0 );
    if ( res != LC_Client_ResultOk ) {
        errorMsg( card, res, qlw );
        deinit( card, cl, res );
        return moneyCardMoneyData;
    }

    QMessageBox::information( 0, "QCardReader", "Please insert a Money card." );
    card = LC_Client_WaitForNextCard( cl, 30 );
    if ( !card ) {
        QMessageBox::critical( 0, "QCardReader", "No card found." );
        errorMsg( card, res, qlw );
        LC_Client_StopWait( cl );
        deinit( card, cl, res );
        return moneyCardMoneyData;
    }

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO: Telling the server that we need no more cards ..." );

    res = LC_Client_StopWait( cl );
    if ( res != LC_Client_ResultOk ) {
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
        deinit( card, cl, res );
        return moneyCardMoneyData;
    }

    rv = LC_GeldKarte_ExtendCard( card );
    if ( rv ) {
        QMessageBox::warning( 0, "QCardReader", "Could not extend card as Money card, abort" );
        deinit( card, cl, res );
        return moneyCardMoneyData;
    }

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Opening card ..." );

    /* TODO Open Card */
    res = LC_Card_Open( card );
    if ( res != LC_Client_ResultOk ) {
        QMessageBox::critical( 0, "QCardReader", "Error executing command CardOpen." );
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
        deinit( card, cl, res );
        return moneyCardMoneyData;
    }

    /* TODO Read the stored values off the card (loaded amount, maximum amount, transfer amount limit). and store this into db*/
    values = LC_GeldKarte_Values_new();
    res = LC_GeldKarte_ReadValues( card, values );
    if ( res != LC_Client_ResultOk ) {
        QMessageBox::critical( 0, "QCardReader", "Can't get the money values from card." );
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
        deinit( card, cl, res );
        return moneyCardMoneyData;
    }

    moneyCardMoneyData[ "loaded" ] = ( float ) LC_GeldKarte_Values_GetLoaded( values ) / 100.0;
    moneyCardMoneyData[ "maxLoaded" ] = ( float ) LC_GeldKarte_Values_GetMaxLoad( values ) / 100.0;
    moneyCardMoneyData[ "maxTransfer" ] = ( float ) LC_GeldKarte_Values_GetMaxXfer( values ) / 100.0;

    /* TODO Read the stored values off the card loading/unloading of the card at credit institutes and read transactions in shops etc */
    /* TODO contain transactions in shops */
    transBlog = LC_GeldKarte_BLog_new();
    transBlogList = LC_GeldKarte_BLog_List2_new();

    //res = LC_GeldKarte_ReadBLogs( card, transBlogList );
    if ( res != LC_Client_ResultOk ) {
        QMessageBox::critical( 0, "QCardReader", "Can't get the transaction values from card." );
        qlwItem = new QListWidgetItem( qlw );
        qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
        deinit( card, cl, res );
        return moneyCardMoneyData;
    }

    moneyCardMoneyData[ "transactions" ] = LC_GeldKarte_BLog_GetLoaded( transBlog );


    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Get card data ... done" );

    qlwItem = new QListWidgetItem( qlw );
    qlwItem->setText( "INFO:  Close card and free memory ..." );
    deinit( card, cl, res );
    return moneyCardMoneyData;
}

/* TODO Get money  from processor card */
QString QCRChipCard::getProcessorCardData( QListWidget *qlw )
{
    return "";
}

/* TODO Destroy all card objects */
bool QCRChipCard::deinit( LC_CARD *card, LC_CLIENT * cl, LC_CLIENT_RESULT res )
{
    res = LC_Card_Close( card );
    if ( res == LC_Client_ResultOk ) {
        LC_Card_free( card );
        LC_Client_free( cl );
        return true;
    }

    LC_Card_free( card );
    LC_Client_free( cl );
    return false;
}

/* TODO Get error message */
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
