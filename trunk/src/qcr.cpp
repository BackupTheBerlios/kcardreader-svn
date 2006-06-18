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

QCardReader::QCardReader( QWidget *parent, Qt::WFlags flags ) : QWidget( parent, flags )
{
    setupUi( this );

    connect( btnKVKCardInfo, SIGNAL( clicked() ), this, SLOT( slotKVKCard() ) );
    connect( btnMoneyCardInfo, SIGNAL( clicked() ), this, SLOT( slotMoneyCard() ) );
    connect( btnMoneyCardLoad, SIGNAL( clicked() ), this, SLOT( slotMoneyCardLoad() ) );

    /* Connection for memory cards (password) */
    connect( btnMemoryCardAddPwd, SIGNAL( clicked() ), this, SLOT( slotAddMemory() ) );
    connect( btnMemoryCardRemovePwd, SIGNAL( clicked() ), this, SLOT( slotRemoveMemory() ) );
    connect( btnMemoryCardChangePwd, SIGNAL( clicked() ), this, SLOT( slotChangeMemory() ) );
    connect( btnMemoryCardWritePwd, SIGNAL( clicked() ), this, SLOT( slotWriteMemory() ) );
    connect( btnMemoryCardReadPwd, SIGNAL( clicked() ), this, SLOT( slotReadMemory() ) );
    connect( btnMemoryCardCapacityPwd, SIGNAL( clicked() ), this, SLOT( slotCapacityMemory() ) );

    /* Connection for memory cards (e-mail) */
    connect( btnMemoryCardAddEMail, SIGNAL( clicked() ), this, SLOT( slotAddMemory() ) );
    connect( btnMemoryCardRemoveEMail, SIGNAL( clicked() ), this, SLOT( slotRemoveMemory() ) );
    connect( btnMemoryCardChangeEMail, SIGNAL( clicked() ), this, SLOT( slotChangeMemory() ) );
    connect( btnMemoryCardWriteEMail, SIGNAL( clicked() ), this, SLOT( slotWriteMemory() ) );
    connect( btnMemoryCardReadEMail, SIGNAL( clicked() ), this, SLOT( slotReadMemory() ) );
    connect( btnMemoryCardCapacityEMail, SIGNAL( clicked() ), this, SLOT( slotCapacityMemory() ) );

    /* TreeWidget Item Clicked */
    connect( treeWidgetPassword, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( treeWidgetItemClicked( QTreeWidgetItem *, int ) ) );
    connect( treeWidgetEMail, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( treeWidgetItemClicked( QTreeWidgetItem *, int ) ) );

    /* ComboBox Item Changed/Clicked */
    connect( comboBoxUseCardAs, SIGNAL( currentIndexChanged( int ) ), this, SLOT( comboboxItemClicked( int ) ) );
    
    QDesktopWidget *desktop = qApp->desktop();
    const QRect rect = desktop->availableGeometry( desktop->primaryScreen() );
    int left = ( rect.width() - width() ) / 2;
    int top = ( rect.height() - height() ) / 2;
    setGeometry( left, top, width(), height() );
}

QCardReader::~QCardReader()
{}

void QCardReader::closeEvent( QCloseEvent *e )
{
    e->accept();
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
{}

void QCardReader::slotCapacityMemory()
{
    QCRChipCard *qcrc = new QCRChipCard();
    qcrc->getCapacityMemoryCardData( listWidgetLogMessage );
    delete qcrc;
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
    QStringList moneyMap, moneyMapAcc, tmpList;

    QCRChipCard *qcrc = new QCRChipCard();
    moneyMap = qcrc->getMoneyCardData( listWidgetLogMessage );

    if ( moneyMap.size() <= 0 )
        return ;

    for ( int x = 0; x < moneyMap.size(); x++ ) {
        tmpList = moneyMap.value( x ).split( ";" );
        if ( tmpList.value( 0 ) == "branchKey" ) {
            lineEditMoneyCardBranchKey->setText( tmpList.value( 1 ) );

        } else if ( tmpList.value( 0 ) == "shortBankCode" ) {
            lineEditMoneyCardShortBankcode->setText( tmpList.value( 1 ) );

        } else if ( tmpList.value( 0 ) == "cardNumber" ) {
            lineEditMoneyCardCardNumber->setText( tmpList.value( 1 ) );

        } else if ( tmpList.value( 0 ) == "checksum" ) {
            lineEditMoneyCardCheckSum->setText( tmpList.value( 1 ) );

        } else if ( tmpList.value( 0 ) == "validUntil" ) {
            lineEditMoneyCardValidUntil->setText( tmpList.value( 1 ) );

        } else if ( tmpList.value( 0 ) == "validSince" ) {
            lineEditMoneyCardValidSince->setText( tmpList.value( 1 ) );

        } else if ( tmpList.value( 0 ) == "country" ) {
            lineEditMoneyCardCountry->setText( tmpList.value( 1 ) );

        } else if ( tmpList.value( 0 ) == "currency" ) {
            lineEditMoneyCardCurrency->setText( tmpList.value( 1 ) );

        } else if ( tmpList.value( 0 ) == "factor" ) {
            lineEditMoneyCardFactor->setText( tmpList.value( 1 ) );

        } else if ( tmpList.value( 0 ) == "chipversion" ) {
            lineEditMoneyCardChipVersion->setText( tmpList.value( 1 ) );

        }
    }

    tmpList.clear();
    moneyMapAcc = qcrc->getMoneyCardAccData( listWidgetLogMessage );

    if ( moneyMapAcc.size() <= 0 )
        return ;

    for ( int x = 0; x < moneyMapAcc.size(); x++ ) {
        tmpList = moneyMapAcc.value( x ).split( ";" );
        if ( tmpList.value( 0 ) == "cardType" )
            lineEditMoneyCardCardType->setText( tmpList.value( 1 ) );

        if ( tmpList.value( 0 ) == "bankCode" )
            lineEditMoneyCardBankCode->setText( tmpList.value( 1 ) );

        if ( tmpList.value( 0 ) == "accountId" )
            lineEditMoneyCardAccountId->setText( tmpList.value( 1 ) );
    }
}

/* TODO Money Card Load */
void QCardReader::slotMoneyCardLoad()
{
    QMap<QString, double> moneyLoadMap;
    QStringList moneyTransList;
    
    QCRChipCard *qcrc = new QCRChipCard();
    moneyLoadMap = qcrc->getMoneyCardMoneyData( listWidgetLogMessage );

    if ( moneyLoadMap.size() <= 0 )
        return ;
    
    QMap<QString, double>::const_iterator c_it = moneyLoadMap.begin();
    QVariant value;
    while ( c_it != moneyLoadMap.end() ) {
	if ( c_it.key() == "loaded" ) {
	    value = c_it.value();
	    lineEditMoneyCardCurrentLoaded->setText( value.toString() + " Euro" );
	}
	if ( c_it.key() == "maxLoaded" ) {
	    value = c_it.value();	 
	    lineEditMoneyCardMaxLoaded->setText( value.toString() + " Euro" );
	}
	if ( c_it.key() == "maxTransfer" ) {
	    value = c_it.value();	 
	    lineEditMoneyCardMaxTransfer->setText( value.toString() + " Euro" );
	}
        ++c_it;
    }
    
    
    //moneyTransList = qcrc->getMoneyCardTransactionData( listWidgetLogMessage );
}

/* TODO KVK Card */
void QCardReader::slotKVKCard()
{
    QMap<QString, QString> kvkMap;

    QCRChipCard *qcrc = new QCRChipCard();
    kvkMap = qcrc->getKVKCardData( listWidgetLogMessage );

    if ( kvkMap.size() <= 0 )
        return ;

    QMap<QString, QString>::const_iterator c_it = kvkMap.begin();
    while ( c_it != kvkMap.end() ) {
        if ( c_it.key() == "insuranceCompanyName" )
            lineEditGHICCompanyName->setText( c_it.value() );

        if ( c_it.key() == "insuranceCompanyCode" )
            lineEditGHICCompanyCode->setText( c_it.value() );

        if ( c_it.key() == "cardNumber" )
            lineEditGHICCardNumber->setText( c_it.value() );

        if ( c_it.key() == "insuranceNumber" )
            lineEditGHICInsuranceNumber->setText( c_it.value() );

        if ( c_it.key() == "insuranceState" )
            lineEditGHICInsuranceState->setText( c_it.value() );

        if ( c_it.key() == "eastOrWest" )
            lineEditGHICEastOrWest->setText( c_it.value() );

        if ( c_it.key() == "foreName" )
            lineEditGHICForeName->setText( c_it.value() );

        if ( c_it.key() == "name" )
            lineEditGHICName->setText( c_it.value() );

        if ( c_it.key() == "dateOfBirth" )
            lineEditGHICDateOfBirth->setText( c_it.value() );

        if ( c_it.key() == "addrStreet" )
            lineEditGHICStreet->setText( c_it.value() );

        if ( c_it.key() == "addrPostalCode" )
            lineEditGHICPostalCode->setText( c_it.value() );

        if ( c_it.key() == "addrCity" )
            lineEditGHICCity->setText( c_it.value() );

        if ( c_it.key() == "bestBefore" )
            lineEditGHICValidUntil->setText( c_it.value() );

        if ( c_it.key() == "checksum" or c_it.key() == "" or c_it.key().isEmpty() )
            lineEditGHICEastOrWest->setText( c_it.value() );
        ++c_it;
    }
}

/* TreeWidget Item Clicked */
void QCardReader::treeWidgetItemClicked( QTreeWidgetItem *item, int column )
{}

/* ComboBox Item Changed/Clicked */
void QCardReader::comboboxItemClicked( int index )
{
    cbxIndex = index;
}
