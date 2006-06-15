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

/* Chipcard2/Gwen include */
#include <chipcard2/chipcard2.h>
#include <chipcard2-client/client/client.h>


/* Qt4 include files */
#include <QtCore>
#include <QtGui>

class QCRChipCard
{
    public:
        QCRChipCard();
        inline virtual ~QCRChipCard() {}

        QString getDDVCardData( QListWidget *qlw );
        QStringList getKVKCardData( QListWidget *qlw );
        QString getMemoryCardData( QListWidget *qlw );
        QString getMoneyCardData( QListWidget *qlw );
        QString getProcessorCardData( QListWidget *qlw );
	
    private:
        bool init( LC_CLIENT * cl );
        bool deinit( LC_CARD *card, LC_CLIENT_RESULT res );
        QString errorMsg( LC_CARD *card, LC_CLIENT_RESULT res, QListWidget *qlw );
	
        QListWidgetItem *qlwItem;
        QStringList kvkCardData;
};
