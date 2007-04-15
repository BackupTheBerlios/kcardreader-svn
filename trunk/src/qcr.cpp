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
#include "qcr.h"
#include "qcr_login.h"
#include "qcr_chipcard.h"

/* 3rdparty include */
#include "XMLPreferences.h"

/* Qt4 include files */
#include <QtGui>
#include <QtCore>

QCardReader::QCardReader ( QWidget *parent, Qt::WFlags flags ) : QWidget ( parent, flags )
{
	setupUi ( this );

	/* Connection for general use */
	connect ( btnQuit, SIGNAL ( clicked() ), this, SLOT ( slotQuit() ) );
	connect ( btnClearLog, SIGNAL ( clicked() ), this, SLOT ( slotClearLog() ) );
	connect ( btnSaveLog, SIGNAL ( clicked() ), this, SLOT ( slotSaveLog() ) );
	connect ( btnAbout, SIGNAL ( clicked() ), this, SLOT ( slotAbout() ) );

	/* Connection for German Health Insurance Cards (KVK) */
	connect ( btnKVKCardInfo, SIGNAL ( clicked() ), this, SLOT ( slotKVKCard() ) );

	/* Connection for money cards */
	connect ( btnMoneyCardInfo, SIGNAL ( clicked() ), this, SLOT ( slotMoneyCard() ) );
	connect ( btnMoneyCardLoad, SIGNAL ( clicked() ), this, SLOT ( slotMoneyCardLoad() ) );

	/* Connection for memory cards (e-mail) */
	connect ( btnMemoryCardAdd, SIGNAL ( clicked() ), this, SLOT ( slotAddMemory() ) );
	connect ( btnMemoryCardRemove, SIGNAL ( clicked() ), this, SLOT ( slotRemoveMemory() ) );
	connect ( btnMemoryCardChange, SIGNAL ( clicked() ), this, SLOT ( slotChangeMemory() ) );
	connect ( btnMemoryCardWrite, SIGNAL ( clicked() ), this, SLOT ( slotWriteMemory() ) );
	connect ( btnMemoryCardRead, SIGNAL ( clicked() ), this, SLOT ( slotReadMemory() ) );
	connect ( btnMemoryCardCapacity, SIGNAL ( clicked() ), this, SLOT ( slotCapacityMemory() ) );
	/* TreeWidget Item Clicked */
	connect ( treeWidgetPassword, SIGNAL ( itemClicked ( QTreeWidgetItem *, int ) ), this, SLOT ( treeWidgetItemClicked ( QTreeWidgetItem *, int ) ) );
	connect ( treeWidgetEMail, SIGNAL ( itemClicked ( QTreeWidgetItem *, int ) ), this, SLOT ( treeWidgetItemClicked ( QTreeWidgetItem *, int ) ) );

	/* ComboBox Item Changed/Clicked */
	connect ( comboBoxUseCardAs, SIGNAL ( currentIndexChanged ( int ) ), this, SLOT ( comboboxItemClicked ( int ) ) );
	
	progressBarMoneyTransfer->setVisible( false );

	QDesktopWidget *desktop = qApp->desktop();
	const QRect rect = desktop->availableGeometry ( desktop->primaryScreen() );
	int left = ( rect.width() - width() ) / 2;
	int top = ( rect.height() - height() ) / 2;
	setGeometry ( left, top, width(), height() );
}

QCardReader::~QCardReader()
{}

void QCardReader::closeEvent ( QCloseEvent *e )
{
	e->accept();
}

void QCardReader::slotQuit()
{
	qApp->quit();
}

void QCardReader::slotClearLog()
{}

void QCardReader::slotSaveLog()
{}

void QCardReader::slotAbout()
{
	QCRChipCard *qcrc = new QCRChipCard();

	QString about, ccV, gwenV;
	ccV = qcrc->getLibchipcardVersion();
	gwenV = qcrc->getGwenhywfarVersion();

	about = tr ( "<h2>About QCardReader Version : 0.0.2</h2>" );
	about += tr ( "QCardReader is grafical tool for money cards, hbci cards, memory cards and many other cards if is supported by libchipcard3.<br /><br />" );
	about += tr ( "(c) 2006-2007 Alexander Saal <a href=\"mailto:alex.saal@gmx.de\">alex.saal@gmx.de</a><br /><br />" );
	about += tr ( "<b>LibChipCard3 Version:</b> " )  + ccV + "<br/>";
	about += tr ( "<b>Gwenhywfar Version:</b>   " )  + gwenV + "<br/><br />";
	about += tr ( "<b>This Program is released under the terms of the<br/>GNU GENERAL PUBLIC LICENSE Version 2, June 1991</b><br /><br />" );
	about += tr ( "Special thanks to :<br />" );
	about += tr ( "  Martin Preuss for libchipcard3 <a href=\"mailto:martin [at] libchipcard [dot] de\">martin@libchipcard.de</a><br />" );

	QMessageBox::about ( this, tr ( "About QCardReader" ), about );
}

/* TODO Memory Card */
void QCardReader::slotAddMemory()
{}

void QCardReader::slotRemoveMemory()
{}

void QCardReader::slotChangeMemory()
{}

void QCardReader::slotWriteMemory()
{}

void QCardReader::slotReadMemory()
{
	QCRChipCard *qcrc = new QCRChipCard();
	qcrc->formatMemoryCard( listWidgetLogMessage );
	qcrc->readMemoryCardData( treeWidgetEMail, listWidgetLogMessage );
}

void QCardReader::slotCapacityMemory()
{
}

/* TODO Processor Card */
void QCardReader::slotAddProcessorPwd()
{}

void QCardReader::slotRemoveProcessorPwd()
{}

void QCardReader::slotChangeProcessorPwd()
{}

void QCardReader::slotWriteProcessorPwd()
{}

void QCardReader::slotReadProcessorPwd()
{}

void QCardReader::slotCapacityProcessorPwd()
{}

/* TODO Money Card Data*/
void QCardReader::slotMoneyCard()
{
	// treeWidgetMoneyTransfer
	QStringList moneyMap, moneyMapAcc, tmpList;

	QCRChipCard *qcrc = new QCRChipCard();
	moneyMap = qcrc->getMoneyCardData ( listWidgetLogMessage );

	if ( moneyMap.size() <= 0 )
		return ;

	for ( int x = 0; x < moneyMap.size(); x++ )
	{
		tmpList = moneyMap.value ( x ).split ( ";" );
		if ( tmpList.value ( 0 ) == "branchKey" )
		{
			lineEditMoneyCardBranchKey->setText ( tmpList.value ( 1 ) );

		}
		else if ( tmpList.value ( 0 ) == "shortBankCode" )
		{
			lineEditMoneyCardShortBankcode->setText ( tmpList.value ( 1 ) );

		}
		else if ( tmpList.value ( 0 ) == "cardNumber" )
		{
			lineEditMoneyCardCardNumber->setText ( tmpList.value ( 1 ) );

		}
		else if ( tmpList.value ( 0 ) == "checksum" )
		{
			lineEditMoneyCardCheckSum->setText ( tmpList.value ( 1 ) );

		}
		else if ( tmpList.value ( 0 ) == "validUntil" )
		{
			lineEditMoneyCardValidUntil->setText ( tmpList.value ( 1 ) );

		}
		else if ( tmpList.value ( 0 ) == "validSince" )
		{
			lineEditMoneyCardValidSince->setText ( tmpList.value ( 1 ) );

		}
		else if ( tmpList.value ( 0 ) == "country" )
		{
			lineEditMoneyCardCountry->setText ( tmpList.value ( 1 ) );

		}
		else if ( tmpList.value ( 0 ) == "currency" )
		{
			lineEditMoneyCardCurrency->setText ( tmpList.value ( 1 ) );

		}
		else if ( tmpList.value ( 0 ) == "factor" )
		{
			lineEditMoneyCardFactor->setText ( tmpList.value ( 1 ) );

		}
		else if ( tmpList.value ( 0 ) == "chipversion" )
		{
			lineEditMoneyCardChipVersion->setText ( tmpList.value ( 1 ) );

		}
	}

	/*
	tmpList.clear();
	moneyMapAcc = qcrc->getMoneyCardAccData ( listWidgetLogMessage );

	if ( moneyMapAcc.size() <= 0 )
		return ;

	for ( int x = 0; x < moneyMapAcc.size(); x++ )
	{
		tmpList = moneyMapAcc.value ( x ).split ( ";" );
		if ( tmpList.value ( 0 ) == "cardType" )
			lineEditMoneyCardCardType->setText ( tmpList.value ( 1 ) );

		if ( tmpList.value ( 0 ) == "bankCode" )
			lineEditMoneyCardBankCode->setText ( tmpList.value ( 1 ) );

		if ( tmpList.value ( 0 ) == "accountId" )
			lineEditMoneyCardAccountId->setText ( tmpList.value ( 1 ) );
	}
	*/
}

/* TODO Money Card Load */
void QCardReader::slotMoneyCardLoad()
{
	QMap<QString, double> moneyLoadMap;
	QStringList moneyTransList;

	QCRChipCard *qcrc = new QCRChipCard();
	moneyLoadMap = qcrc->getMoneyCardMoneyData ( listWidgetLogMessage );

	if ( moneyLoadMap.size() <= 0 )
		return ;

	QMap<QString, double>::const_iterator c_it = moneyLoadMap.begin();
	QVariant value;
	while ( c_it != moneyLoadMap.end() )
	{
		if ( c_it.key() == "loaded" )
		{
			value = c_it.value();
			lineEditMoneyCardCurrentLoaded->setText ( value.toString() + " Euro" );
		}
		if ( c_it.key() == "maxLoaded" )
		{
			value = c_it.value();
			lineEditMoneyCardMaxLoaded->setText ( value.toString() + " Euro" );
		}
		if ( c_it.key() == "maxTransfer" )
		{
			value = c_it.value();
			lineEditMoneyCardMaxTransfer->setText ( value.toString() + " Euro" );
		}
		++c_it;
	}


	progressBarMoneyTransfer->setVisible( true );
	qcrc->getMoneyCardTransaction( listWidgetLogMessage, treeWidgetMoneyTransferBLogs, treeWidgetMoneyTransferLLogs, progressBarMoneyTransfer);
	progressBarMoneyTransfer->setVisible( false );
}

/* TODO KVK Card */
void QCardReader::slotKVKCard()
{
	QMap<QString, QString> kvkMap;

	QCRChipCard *qcrc = new QCRChipCard();
	kvkMap = qcrc->getKVKCardData ( listWidgetLogMessage );

	if ( kvkMap.size() <= 0 )
		return ;

	QMap<QString, QString>::const_iterator c_it = kvkMap.begin();
	while ( c_it != kvkMap.end() )
	{
		if ( c_it.key() == "insuranceCompanyName" )
			lineEditGHICCompanyName->setText ( c_it.value() );

		if ( c_it.key() == "insuranceCompanyCode" )
			lineEditGHICCompanyCode->setText ( c_it.value() );

		if ( c_it.key() == "cardNumber" )
			lineEditGHICCardNumber->setText ( c_it.value() );

		if ( c_it.key() == "insuranceNumber" )
			lineEditGHICInsuranceNumber->setText ( c_it.value() );

		if ( c_it.key() == "insuranceState" )
			lineEditGHICInsuranceState->setText ( c_it.value() );

		if ( c_it.key() == "eastOrWest" )
			lineEditGHICEastOrWest->setText ( c_it.value() );

		if ( c_it.key() == "foreName" )
			lineEditGHICForeName->setText ( c_it.value() );

		if ( c_it.key() == "name" )
			lineEditGHICName->setText ( c_it.value() );

		if ( c_it.key() == "dateOfBirth" )
			lineEditGHICDateOfBirth->setText ( c_it.value() );

		if ( c_it.key() == "addrStreet" )
			lineEditGHICStreet->setText ( c_it.value() );

		if ( c_it.key() == "addrPostalCode" )
			lineEditGHICPostalCode->setText ( c_it.value() );

		if ( c_it.key() == "addrCity" )
			lineEditGHICCity->setText ( c_it.value() );

		if ( c_it.key() == "bestBefore" )
			lineEditGHICValidUntil->setText ( c_it.value() );

		if ( c_it.key() == "checksum" or c_it.key() == "" or c_it.key().isEmpty() )
			lineEditGHICEastOrWest->setText ( c_it.value() );
		++c_it;
	}
}

/* TreeWidget Item Clicked */
void QCardReader::treeWidgetItemClicked ( QTreeWidgetItem *item, int column )
{}

/* ComboBox Item Changed/Clicked */
void QCardReader::comboboxItemClicked ( int index )
{
	cbxIndex = index;
}
