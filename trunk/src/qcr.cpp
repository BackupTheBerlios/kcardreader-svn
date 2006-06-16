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

void QCardReader::slotKVKCard()
{
    QMap<QString, QString> kvkMap;

    QCRChipCard *qcrc = new QCRChipCard();
    kvkMap = qcrc->getKVKCardData( listWidgetLogMessage );

    if( kvkMap.size() <= 0 )
	return;
    
    
    QMap<QString, QString>::const_iterator c_it = kvkMap.begin();
    while ( c_it != kvkMap.end() ) {
	if( c_it.key() == "insuranceCompanyName" )
	    lineEditCompanyName->setText( c_it.value() );

	if( c_it.key() == "insuranceCompanyCode" )
	    lineEditCompanyCode->setText( c_it.value() );

	if( c_it.key() == "cardNumber" )
	    lineEditCardNumber->setText( c_it.value() );
    
	if( c_it.key() == "insuranceNumber" )
	    lineEditInsuranceNumber->setText( c_it.value() );

	if( c_it.key() == "insuranceState" )
	    lineEditInsuranceState->setText( c_it.value() );

	if( c_it.key() == "eastOrWest" )
	    lineEditEastOrWest->setText( c_it.value() );

	if( c_it.key() == "foreName" )
	    lineEditForeName->setText( c_it.value() );

	if( c_it.key() == "name" )
	    lineEditName->setText( c_it.value() );

	if( c_it.key() == "dateOfBirth" )
	    lineEditDateOfBirth->setText( c_it.value() );

	if( c_it.key() == "addrStreet" )
	    lineEditStreet->setText( c_it.value() );

	if( c_it.key() == "addrPostalCode" )
	    lineEditPostalCode->setText( c_it.value() );

	if( c_it.key() == "addrCity" )
	    lineEditCity->setText( c_it.value() );

	if( c_it.key() == "bestBefore" )
	    lineEditValidUntil->setText( c_it.value() );

	if( c_it.key() == "checksum" or c_it.key() == "" )
	    lineEditEastOrWest->setText( c_it.value() );
        ++c_it;
    }
}






