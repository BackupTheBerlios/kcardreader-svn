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

/* TODO Get data from ddv card (HBCI Card) */
QString QCRChipCard::getDDVCardData ( QListWidget *qlw )
{
	return "";
}

/* TODO Get data from kvk card (German Health Insurance Cards (KVK)) */
QMap<QString, QString> QCRChipCard::getKVKCardData ( QListWidget *qlw )
{
	qlw->clear();

	LC_CLIENT *cl;
	LC_CARD *card = 0;
	LC_CLIENT_RESULT res;
	GWEN_DB_NODE *dbData;
	int rv;

	cl = LC_Client_new ( "QCardReader", "0.0.1" );
	res = LC_Client_Init( cl );

	if ( res != LC_Client_ResultOk )
	{
		errorMsg ( card, res, qlw );
		deinit ( card, cl, res );
		return kvkCardData;
	}
	
	res = LC_Client_Start(cl);
	if ( res != LC_Client_ResultOk )
	{
		errorMsg ( card, res, qlw );
		deinit ( card, cl, res );
		return kvkCardData;
	}

	QMessageBox::information ( 0, "QCardReader", "Please insert a German medical card." );
	res = LC_Client_GetNextCard(cl, &card, 30);
	if ( res!=LC_Client_ResultOk )
	{
		QMessageBox::critical ( 0, "QCardReader", "No card found." );
		errorMsg ( card, res, qlw );
		LC_Client_Stop( cl );
		deinit ( card, cl, res );
		return kvkCardData;
	}

	qlwItem = new QListWidgetItem ( qlw );
	qlwItem->setText ( "INFO: Telling the server that we need no more cards ..." );

	res = LC_Client_Stop ( cl );
	if ( res != LC_Client_ResultOk )
	{
		qlwItem = new QListWidgetItem ( qlw );
		qlwItem->setText ( "ERROR:  " + errorMsg ( card, res, qlw ) );
		deinit ( card, cl, res );
		return kvkCardData;
	}

	rv = LC_KVKCard_ExtendCard ( card );
	if ( rv )
	{
		QMessageBox::warning ( 0, "QCardReader", "Could not extend card as German medical card, abort." );
		deinit ( card, cl, res );
		return kvkCardData;
	}

	qlwItem = new QListWidgetItem ( qlw );
	qlwItem->setText ( "INFO:  Opening card ..." );

	res = LC_Card_Open ( card );
	if ( res != LC_Client_ResultOk )
	{
		QMessageBox::critical ( 0, "QCardReader", "Error executing command CardOpen." );
		qlwItem = new QListWidgetItem ( qlw );
		qlwItem->setText ( "ERROR:  " + errorMsg ( card, res, qlw ) );
		deinit ( card, cl, res );
		return kvkCardData;
	}

	dbData = LC_KVKCard_GetCardData ( card );
	if ( !dbData )
	{
		QMessageBox::critical ( 0, "QCardReader", "No card data available." );
		deinit ( card, cl, res );
		return kvkCardData;
	}

	qlwItem = new QListWidgetItem ( qlw );
	qlwItem->setText ( "INFO:  Get card data ..." );

	kvkCardData[ "insuranceCompanyName" ] = QString( GWEN_DB_GetCharValue( dbData, "insuranceCompanyName", 0, "nothing") );
	kvkCardData[ "insuranceCompanyCode" ] = QString( GWEN_DB_GetCharValue( dbData, "insuranceCompanyCode", 0, "nothing") );
	kvkCardData[ "cardNumber" ] = QString( GWEN_DB_GetCharValue( dbData, "cardNumber", 0, "nothing") );
	kvkCardData[ "insuranceNumber" ] = QString( GWEN_DB_GetCharValue( dbData, "insuranceNumber", 0, "nothing") );
	kvkCardData[ "insuranceState" ] = QString( GWEN_DB_GetCharValue( dbData, "insuranceState", 0, "nothing") );
	kvkCardData[ "eastOrWest" ] = QString( GWEN_DB_GetCharValue( dbData, "eastOrWest", 0, "nothing") );
	kvkCardData[ "foreName" ] = QString( GWEN_DB_GetCharValue( dbData, "foreName", 0, "nothing") );
	kvkCardData[ "name" ] = QString( GWEN_DB_GetCharValue( dbData, "name", 0, "nothing") );
	kvkCardData[ "dateOfBirth" ] = QString( GWEN_DB_GetCharValue( dbData, "dateOfBirth", 0, "nothing") );
	kvkCardData[ "addrStreet" ] = QString( GWEN_DB_GetCharValue( dbData, "addrStreet", 0, "nothing") );
	kvkCardData[ "addrPostalCode" ] = QString( GWEN_DB_GetCharValue( dbData, "addrPostalCode", 0, "nothing") );
	kvkCardData[ "addrCity" ] = QString( GWEN_DB_GetCharValue( dbData, "addrCity", 0, "nothing") );
	kvkCardData[ "bestBefore" ] = QString( GWEN_DB_GetCharValue( dbData, "bestBefore", 0, "nothing") );
	kvkCardData[ "checksum" ] = QString( GWEN_DB_GetCharValue( dbData, "checksum", 0, "nothing") );


	qlwItem = new QListWidgetItem ( qlw );
	qlwItem->setText ( "INFO:  Get card data ... done" );

	qlwItem = new QListWidgetItem ( qlw );
	qlwItem->setText ( "INFO:  Close card and free memory ..." );
	deinit ( card, cl, res );
	return kvkCardData;
}

/* TODO Write data to memory card */
bool QCRChipCard::writeMemoryCardData ( const QString &data, QListWidget *qlw )
{
	return true;
}

/* TODO Read data from memory card */
QStringList QCRChipCard::readMemoryCardData ( QTreeWidget *qtw, QListWidget *qlw )
{
	return QStringList();
}

/* TODO Get capacity data from memory card */
unsigned int QCRChipCard::getCapacityMemoryCardData ( QListWidget *qlw )
{
	qlw->clear();
	memCap = 0;

	return memCap;
}

/* TODO Get version of LibChipCard2 */
QString QCRChipCard::getLibchipcardVersion()
{
	return CHIPCARD_VERSION_FULL_STRING;
}

/* TODO Get data from money card */
QStringList QCRChipCard::getMoneyCardData ( QListWidget *qlw )
{
	qlw->clear();

	return moneyCardData;
}

/* TODO Get account data from money card */
QStringList QCRChipCard::getMoneyCardAccData ( QListWidget *qlw )
{
	qlw->clear();

	return moneyCardAccData;
}

/* TODO Get money  from money card */
QMap<QString, double> QCRChipCard::getMoneyCardMoneyData ( QListWidget *qlw )
{
	qlw->clear();

	return moneyCardMoneyData;
}

/* TODO Get transactions from money card */
QStringList QCRChipCard::getMoneyCardTransactionData ( QListWidget *qlw )
{
	qlw->clear();

	return moneyCardTransactionData;
}

/* TODO Get money  from processor card */
QString QCRChipCard::getProcessorCardData ( QListWidget *qlw )
{
	return "";
}

/* TODO Destroy all card objects */
bool QCRChipCard::deinit ( LC_CARD *card, LC_CLIENT * cl, LC_CLIENT_RESULT res )
{
	res = LC_Card_Close ( card );
	if ( res == LC_Client_ResultOk )
	{
		LC_Client_ReleaseCard(cl, card);
		LC_Card_free ( card );
		LC_Client_free ( cl );
		return true;
	}

	LC_Client_ReleaseCard(cl, card);
	LC_Card_free ( card );
	LC_Client_free ( cl );
	return false;
}

/* TODO Get error message */
QString QCRChipCard::errorMsg ( LC_CARD *card, LC_CLIENT_RESULT res, QListWidget *qlw )
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

		sw1 = LC_Card_GetLastSW1 ( card );
		sw2 = LC_Card_GetLastSW2 ( card );
		qlwItem = new QListWidgetItem ( qlw );
		if ( sw1 != -1 && sw2 != -1 )
			qlwItem->setText ( "Last card command result: SW1= " + QString ( sw1 ) + ", SW2 = " + QString ( sw2 ) );

		s = LC_Card_GetLastResult ( card );
		qlwItem = new QListWidgetItem ( qlw );
		qlwItem->setText ( "Result: " + s );

		s = LC_Card_GetLastText ( card );
		qlwItem = new QListWidgetItem ( qlw );
		qlwItem->setText ( "Text: " + s );
	}
	return s;
}
