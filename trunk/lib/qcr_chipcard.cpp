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

/* ChipCard3 include */
#include <gwenhywfar/db.h>
#include <gwenhywfar/version.h>
#include <gwenhywfar/gwentime.h>

#include <chipcard3/version.h>
#include <chipcard3/chipcard3.h>

#include <chipcard3/client/card.h>
#include <chipcard3/client/client.h>

#include <chipcard3/client/cards/ddvcard.h>

#include <chipcard3/client/cards/geldkarte.h>
#include <chipcard3/client/cards/geldkarte_values.h>
#include <chipcard3/client/cards/geldkarte_blog.h>
#include <chipcard3/client/cards/geldkarte_llog.h>

#include <chipcard3/client/cards/kvkcard.h>
#include <chipcard3/client/cards/memorycard.h>
#include <chipcard3/client/cards/processorcard.h>
#include <chipcard3/client/cards/starcos.h>

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
	moneyCardData.clear();
	moneyCardAccData.clear();
	moneyCardTransactionData.clear();
	kvkCardData.clear();
	moneyCardMoneyData.clear();
	delete qlwItem;
}

bool QCRChipCard::init( QListWidget *qlw )
{
	card = 0;
	cl = LC_Client_new( "QCardReader", "0.0.1" );
	res = LC_Client_Init( cl );

	if ( res != LC_Client_ResultOk )
	{
		errorMsg( card, res, qlw );
		deinit( card, cl, res );
		return false;
	}

	res = LC_Client_Start( cl );
	if ( res != LC_Client_ResultOk )
	{
		errorMsg( card, res, qlw );
		deinit( card, cl, res );
		return false;
	}

	res = LC_Client_GetNextCard( cl, &card, 20 );
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical( 0, "QCardReader", "No card found." );
		errorMsg( card, res, qlw );
		LC_Client_Stop( cl );
		deinit( card, cl, res );
		return false;
	}

	return true;
}

/* TODO Destroy all card objects */
bool QCRChipCard::deinit( LC_CARD *card, LC_CLIENT * cl, LC_CLIENT_RESULT res )
{
	res = LC_Card_Close( card );
	if ( res == LC_Client_ResultOk )
	{
		LC_Client_ReleaseCard( cl, card );
		LC_Card_free( card );
		LC_Client_free( cl );
		return true;
	}

	LC_Client_ReleaseCard( cl, card );
	LC_Card_free( card );
	LC_Client_free( cl );
	return false;
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

	QMessageBox::information( 0, "QCardReader", "Please insert a German medical card." );

	if ( init( qlw ) )
		;
	else
	{
		deinit( card, cl, res );
		return kvkCardData;
	}

	rv = LC_KVKCard_ExtendCard( card );
	if ( rv )
	{
		QMessageBox::warning( 0, "QCardReader", "Could not extend card as German medical card, abort." );
		deinit( card, cl, res );
		return kvkCardData;
	}

	qlwItem = new QListWidgetItem( qlw );
	qlwItem->setText( "INFO:  Opening card ..." );

	res = LC_Card_Open( card );
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical( 0, "QCardReader", "Error executing command CardOpen." );
		qlwItem = new QListWidgetItem( qlw );
		qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
		deinit( card, cl, res );
		return kvkCardData;
	}

	dbData = LC_KVKCard_GetCardData( card );
	if ( !dbData )
	{
		QMessageBox::critical( 0, "QCardReader", "No card data available." );
		deinit( card, cl, res );
		return kvkCardData;
	}

	qlwItem = new QListWidgetItem( qlw );
	qlwItem->setText( "INFO:  Get card data ..." );

	kvkCardData[ "insuranceCompanyName" ] = QString( GWEN_DB_GetCharValue( dbData, "insuranceCompanyName", 0, "nothing" ) );
	kvkCardData[ "insuranceCompanyCode" ] = QString( GWEN_DB_GetCharValue( dbData, "insuranceCompanyCode", 0, "nothing" ) );
	kvkCardData[ "cardNumber" ] = QString( GWEN_DB_GetCharValue( dbData, "cardNumber", 0, "nothing" ) );
	kvkCardData[ "insuranceNumber" ] = QString( GWEN_DB_GetCharValue( dbData, "insuranceNumber", 0, "nothing" ) );
	kvkCardData[ "insuranceState" ] = QString( GWEN_DB_GetCharValue( dbData, "insuranceState", 0, "nothing" ) );
	kvkCardData[ "eastOrWest" ] = QString( GWEN_DB_GetCharValue( dbData, "eastOrWest", 0, "nothing" ) );
	kvkCardData[ "foreName" ] = QString( GWEN_DB_GetCharValue( dbData, "foreName", 0, "nothing" ) );
	kvkCardData[ "name" ] = QString( GWEN_DB_GetCharValue( dbData, "name", 0, "nothing" ) );
	kvkCardData[ "dateOfBirth" ] = QString( GWEN_DB_GetCharValue( dbData, "dateOfBirth", 0, "nothing" ) );
	kvkCardData[ "addrStreet" ] = QString( GWEN_DB_GetCharValue( dbData, "addrStreet", 0, "nothing" ) );
	kvkCardData[ "addrPostalCode" ] = QString( GWEN_DB_GetCharValue( dbData, "addrPostalCode", 0, "nothing" ) );
	kvkCardData[ "addrCity" ] = QString( GWEN_DB_GetCharValue( dbData, "addrCity", 0, "nothing" ) );
	kvkCardData[ "bestBefore" ] = QString( GWEN_DB_GetCharValue( dbData, "bestBefore", 0, "nothing" ) );
	kvkCardData[ "checksum" ] = QString( GWEN_DB_GetCharValue( dbData, "checksum", 0, "nothing" ) );


	qlwItem = new QListWidgetItem( qlw );
	qlwItem->setText( "INFO:  Get card data ... done" );

	qlwItem = new QListWidgetItem( qlw );
	qlwItem->setText( "INFO:  Close card and free memory ..." );
	deinit( card, cl, res );
	return kvkCardData;
}

bool QCRChipCard::formatMemoryCard( QListWidget *qlw )
{
	qlw->clear();

	QMessageBox::critical( 0, "QCardReader", "Please insert a card." );

	if ( init( qlw ) )
		;
	else
	{
		deinit( card, cl, res );
		return false;
	}

	qlwItem = new QListWidgetItem( qlw );
	qlwItem->setText( "INFO: Telling the server that we need no more cards ..." );

	rv = LC_MemoryCard_ExtendCard( card );
	if ( rv )
	{
		QMessageBox::warning( 0, "QCardReader", "Could not extend card as memory card, abort." );
		deinit( card, cl, res );
		return false;
	}

	qlwItem = new QListWidgetItem( qlw );
	qlwItem->setText( "INFO:  Opening card ..." );

	res = LC_Card_Open( card );
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical( 0, "QCardReader", "No card found." );
		errorMsg( card, res, qlw );
		LC_Client_Stop( cl );
		deinit( card, cl, res );
		return false;
	}

	res = LC_Client_Stop( cl );
	if ( res != LC_Client_ResultOk )
	{
		qlwItem = new QListWidgetItem( qlw );
		qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
		deinit( card, cl, res );
		return false;
	}

	memCap = LC_MemoryCard_GetCapacity( card );

	const char *data;
	data = "";

	for ( unsigned int x = 0; x < memCap - 1; x++ )
		res = LC_MemoryCard_WriteBinary( card, x, data, QByteArray( data ).length() );

	rv = LC_MemoryCard_UnextendCard( card );
	if ( rv )
	{
		QMessageBox::critical( 0, "QCardReader", "Error executing command UnextendCard." );
		qlwItem = new QListWidgetItem( qlw );
		qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
		deinit( card, cl, res );
		return false;
	}

	deinit( card, cl, res );
	return true;
}

/* TODO Write data to memory card */
bool QCRChipCard::writeMemoryCardData( QString data, QListWidget *qlw )
{
	return true;
}

/* TODO Read data from memory card */
QStringList QCRChipCard::readMemoryCardData( QTreeWidget *qtw, QListWidget *qlw )
{
	qlw->clear();
	
	if ( init( qlw ) )
		;
	else
	{
		deinit( card, cl, res );
		return memCardData;
	}

	qlwItem = new QListWidgetItem( qlw );
	qlwItem->setText( "INFO: Telling the server that we need no more cards ..." );

	rv = LC_MemoryCard_ExtendCard( card );
	if ( rv )
	{
		QMessageBox::warning( 0, "QCardReader", "Could not extend card as memory card, abort." );
		deinit( card, cl, res );
		return memCardData;
	}

	qlwItem = new QListWidgetItem( qlw );
	qlwItem->setText( "INFO:  Opening card ..." );

	res = LC_Card_Open( card );
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical( 0, "QCardReader", "No card found." );
		errorMsg( card, res, qlw );
		deinit( card, cl, res );
		return memCardData;
	}

	res = LC_Client_Stop( cl );
	if ( res != LC_Client_ResultOk )
	{
		qlwItem = new QListWidgetItem( qlw );
		qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
		deinit( card, cl, res );
		return memCardData;
	}

	memCap = LC_MemoryCard_GetCapacity( card );
	memBuff = GWEN_Buffer_new( 0, memCap, 0, 1 );

	res = LC_MemoryCard_ReadBinary( card, 0, memCap, memBuff );
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical( 0, "QCardReader", "Can't read memory card." );
		errorMsg( card, res, qlw );
		deinit( card, cl, res );
		GWEN_Buffer_free( memBuff );
		return memCardData;
	}

	qlwItem = new QListWidgetItem( qlw );
	qlwItem->setText( "INFO:  Get card data ..." );

	char *posPointer = GWEN_Buffer_GetPosPointer( memBuff );
	char *start = GWEN_Buffer_GetStart( memBuff );

	qlwItem = new QListWidgetItem( qlw );
	qlwItem->setText( "INFO:  Get card data ... done" );

	qlwItem = new QListWidgetItem( qlw );
	qlwItem->setText( "INFO:  Close card and free memory ..." );

	rv = LC_MemoryCard_UnextendCard( card );
	if ( rv )
	{
		QMessageBox::critical( 0, "QCardReader", "Error executing command UnextendCard." );
		qlwItem = new QListWidgetItem( qlw );
		qlwItem->setText( "ERROR:  " + errorMsg( card, res, qlw ) );
		deinit( card, cl, res );
		GWEN_Buffer_free( memBuff );
		return memCardData;
	}

	deinit( card, cl, res );
	GWEN_Buffer_free( memBuff );
	return memCardData;
}

/* TODO Get version of LibChipCard2 */
QString QCRChipCard::getLibchipcardVersion()
{
	return CHIPCARD_VERSION_FULL_STRING;
}

QString QCRChipCard::getGwenhywfarVersion()
{
	return GWENHYWFAR_VERSION_FULL_STRING;
}


QStringList QCRChipCard::getMoneyCardData( QListWidget *qlw )
{
	if ( init( qlw ) )
		;
	else
	{
		deinit( card, cl, res );
		return QStringList();
	}
	
	deinit( card, cl, res );
	return QStringList();
}

QStringList QCRChipCard::getMoneyCardTransaction( QListWidget *qlw, QTreeWidget *qtw, QTreeWidget *qtw1, QProgressBar *qpb )
{
	QApplication::setOverrideCursor(Qt::WaitCursor);

	qlw->clear();
	qtw->clear();
	qtw1->clear();
	
	if ( init( qlw ) )
		;
	else
	{
		deinit( card, cl, res );
		return QStringList();
	}
	
	if ( qpb )
		qpb->setValue( 1 );

	rv = LC_GeldKarte_ExtendCard( card );
	if ( rv )
	{
		QMessageBox::warning( 0, "QCardReader", QObject::tr( "Could not extend card as money card (%1)." ).arg( res ) );
		deinit( card, cl, res );
		return QStringList();
	}
	if ( qpb )
		qpb->setValue( 2 );

	res = LC_Card_Open( card );
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical( 0, "QCardReader", QObject::tr( "Error executing command LC_Card_Open (%1)." ).arg( res ) );
		errorMsg( card, res, qlw );
		deinit( card, cl, res );
		return QStringList();
	}
	if ( qpb )
		qpb->setValue( 3 );

	res = LC_Client_Stop( cl );
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical( 0, "QCardReader", QObject::tr( "Error executing command LC_Client_Stop (%1)." ).arg( res ) );
		errorMsg( card, res, qlw );
		deinit( card, cl, res );
		return QStringList();
	}
	if ( qpb )
		qpb->setValue( 4 );

	bll = LC_GeldKarte_BLog_List2_new();
	Lll = LC_GeldKarte_LLog_List2_new(); 
	
	res = LC_GeldKarte_ReadBLogs( card, bll );	
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical( 0, "QCardReader", QObject::tr( "Error executing command LC_GeldKarte_ReadBLogs (%1)." ).arg( res ) );
		errorMsg( card, res, qlw );
		deinit( card, cl, res );
		return QStringList();
	}
	
	res = LC_GeldKarte_ReadLLogs( card, Lll );
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical( 0, "QCardReader", QObject::tr( "Error executing command LC_GeldKarte_ReadLLogs (%1)." ).arg( res ) );
		errorMsg( card, res, qlw );
		deinit( card, cl, res );
		return QStringList();
	}

	
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical( 0, "QCardReader", QObject::tr( "Error executing command LC_GeldKarte_ReadBLogs (%1)." ).arg( res ) );
		errorMsg( card, res, qlw );
		deinit( card, cl, res );
		return QStringList();
	}

	if ( qpb )
		qpb->setValue( 5 );

	LC_GELDKARTE_BLOG_LIST2_ITERATOR *blli;
	blli = LC_GeldKarte_BLog_List2_First( bll );
	int i;
	i = 6;
	if ( blli )
	{
		LC_GELDKARTE_BLOG *bl;

		bl = LC_GeldKarte_BLog_List2Iterator_Data( blli );
		while ( bl )
		{
			qApp->processEvents();
			QTreeWidgetItem *item = new QTreeWidgetItem( qtw );

			item->setText( 0, QString( "%1" ).arg( LC_GeldKarte_BLog_GetStatus( bl ) ) );
			item->setTextAlignment( 0, Qt::AlignRight );
			
			item->setText( 1, QString( "%1" ).arg( LC_GeldKarte_BLog_GetBSeq( bl ) ) );
			item->setTextAlignment( 1, Qt::AlignRight );
			
			item->setText( 2, QString( "%1" ).arg( LC_GeldKarte_BLog_GetLSeq( bl ) ) );
			item->setTextAlignment( 2, Qt::AlignRight );
			
			item->setText( 3, QString( "%1" ).arg( LC_GeldKarte_BLog_GetHSeq( bl ) ) );
			item->setTextAlignment( 3, Qt::AlignRight );
			
			item->setText( 4, QString( "%1" ).arg( LC_GeldKarte_BLog_GetSSeq( bl ) ) );
			item->setTextAlignment( 4, Qt::AlignRight );
			
			item->setText( 5, QString( "%1" ).arg( LC_GeldKarte_BLog_GetKeyId( bl ) ) );
			item->setTextAlignment( 5, Qt::AlignRight );
			
			item->setText( 6, QString( "%1" ).arg( LC_GeldKarte_BLog_GetMerchantId( bl ) ) );
			item->setTextAlignment( 6, Qt::AlignRight );
			
			item->setText( 7, QString( "%1" ).arg(( float )( LC_GeldKarte_BLog_GetValue( bl ) / 100.0 ) ) );
			item->setTextAlignment( 7, Qt::AlignRight );
			
			item->setText( 8, QString( "%1" ).arg(( float )( LC_GeldKarte_BLog_GetLoaded( bl ) / 100.0 ) ) );
			item->setTextAlignment( 8, Qt::AlignRight );			

			bl = LC_GeldKarte_BLog_List2Iterator_Next( blli );

			++i;

			if ( qpb )
				qpb->setValue( i );
		}
		LC_GeldKarte_BLog_List2Iterator_free( blli );
	}


	LC_GELDKARTE_LLOG_LIST2_ITERATOR *Llli;
	Llli = LC_GeldKarte_LLog_List2_First( Lll );

	if ( Llli )
	{
		LC_GELDKARTE_LLOG *Ll;

		Ll = LC_GeldKarte_LLog_List2Iterator_Data( Llli );
		while ( Ll )
		{
			qApp->processEvents();
			QTreeWidgetItem *item = new QTreeWidgetItem( qtw1 );
			
			item->setText( 0, QString( "%1" ).arg( LC_GeldKarte_LLog_IsModified( Ll ) ) );
			item->setTextAlignment( 1, Qt::AlignRight );

			item->setText( 1, QString( "%1" ).arg( LC_GeldKarte_LLog_GetStatus( Ll ) ) );
			item->setTextAlignment( 1, Qt::AlignRight );
			
			item->setText( 2, QString( "%1" ).arg( LC_GeldKarte_LLog_GetBSeq( Ll ) ) );
			item->setTextAlignment( 2, Qt::AlignRight );
			
			item->setText( 3, QString( "%1" ).arg( LC_GeldKarte_LLog_GetLSeq( Ll ) ) );
			item->setTextAlignment( 3, Qt::AlignRight );
			
			item->setText( 4, QString( "%1" ).arg( ( float )( LC_GeldKarte_LLog_GetValue( Ll ) / 100.0 ) ) );
			item->setTextAlignment( 4, Qt::AlignRight );
			
			item->setText( 5, QString( "%1" ).arg( LC_GeldKarte_LLog_GetCenterId( Ll ) ) );
			item->setTextAlignment( 5, Qt::AlignRight );
			
			item->setText( 6, QString( "%1" ).arg( LC_GeldKarte_LLog_GetTerminalId( Ll ) ) );
			item->setTextAlignment( 6, Qt::AlignRight );
			
			item->setText( 7, QString( "%1" ).arg( LC_GeldKarte_LLog_GetTraceId( Ll ) ) );
			item->setTextAlignment( 7, Qt::AlignRight );
			
			item->setText( 8, QString( "%1" ).arg(( float )( LC_GeldKarte_LLog_GetLoaded( Ll ) / 100.0 ) ) );
			item->setTextAlignment( 8, Qt::AlignRight );
			
			const GWEN_TIME *gkTime;
			gkTime = LC_GeldKarte_LLog_GetTime( Ll );
			
			dbData = GWEN_DB_Group_new( "LC_GeldKarte_LLog_GetTime" );
			GWEN_Time_toDb( gkTime, dbData );
			
			item->setText( 9, QString( "%1" ).arg( GWEN_DB_GetIntValue( dbData, "LC_GeldKarte_LLog_GetTime", 0, 0 ) ) );
			item->setTextAlignment( 9, Qt::AlignRight );			
			
			GWEN_DB_Group_free( dbData );

			Ll = LC_GeldKarte_LLog_List2Iterator_Next( Llli );

			++i;

			if ( qpb )
				qpb->setValue( i );
		}
		LC_GeldKarte_LLog_List2Iterator_free( Llli );
	}
	
	deinit( card, cl, res );	
	
	QApplication::restoreOverrideCursor();
	return moneyCardTransactionData;

}

QMap<QString, double> QCRChipCard::getMoneyCardMoneyData( QListWidget *qlw )
{
	if ( init( qlw ) )
		;
	else
	{
		deinit( card, cl, res );
		return QMap<QString, double>();
	}

	rv = LC_GeldKarte_ExtendCard( card );
	if ( rv )
	{
		QMessageBox::warning( 0, "QCardReader", QObject::tr( "Could not extend card as money card (%1)." ).arg( res ) );
		deinit( card, cl, res );
		return QMap<QString, double>();
	}

	res = LC_Card_Open( card );
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical( 0, "QCardReader", QObject::tr( "Error executing command LC_Card_Open (%1)." ).arg( res ) );
		errorMsg( card, res, qlw );
		deinit( card, cl, res );
		return QMap<QString, double>();
	}

	res = LC_Client_Stop( cl );
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical( 0, "QCardReader", QObject::tr( "Error executing command LC_Client_Stop (%1)." ).arg( res ) );
		errorMsg( card, res, qlw );
		deinit( card, cl, res );
		return QMap<QString, double>();
	}

	values = LC_GeldKarte_Values_new();
	res = LC_GeldKarte_ReadValues( card, values );
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical( 0, "QCardReader", QObject::tr( "Error executing command LC_GeldKarte_ReadValues (%1)." ).arg( res ) );
		errorMsg( card, res, qlw );
		deinit( card, cl, res );
		return QMap<QString, double>();
	}

	moneyCardMoneyData[ "loaded" ] = ( double )LC_GeldKarte_Values_GetLoaded( values ) / 100.0;
	moneyCardMoneyData[ "maxLoaded" ] = ( double )LC_GeldKarte_Values_GetMaxLoad( values ) / 100.0;
	moneyCardMoneyData[ "maxTransfer" ] = ( double )LC_GeldKarte_Values_GetMaxXfer( values ) / 100.0;

	deinit( card, cl, res );
	return 	moneyCardMoneyData;
}

/* TODO Get money  from processor card */
QString QCRChipCard::getProcessorCardData( QListWidget *qlw )
{
	return "";
}

/* TODO Get error message */
QString QCRChipCard::errorMsg( LC_CARD *card, LC_CLIENT_RESULT res, QListWidget *qlw )
{
	QString s;

	switch ( res )
	{
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

	if ( res == LC_Client_ResultCmdError )
	{
		int sw1;
		int sw2;

		sw1 = LC_Card_GetLastSW1( card );
		sw2 = LC_Card_GetLastSW2( card );
		qlwItem = new QListWidgetItem( qlw );
		if ( sw1 != -1 && sw2 != -1 )
			qlwItem->setText( "Last card command result: SW1 = " + QString( sw1 ) + ", SW2 = " + QString( sw2 ) );

		s = LC_Card_GetLastResult( card );
		qlwItem = new QListWidgetItem( qlw );
		qlwItem->setText( "Result: " + s );

		s = LC_Card_GetLastText( card );
		qlwItem = new QListWidgetItem( qlw );
		qlwItem->setText( "Text: " + s );
	}
	return s;
}
