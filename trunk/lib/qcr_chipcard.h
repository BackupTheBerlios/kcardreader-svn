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

/* ChipCard3 include */
#include <gwenhywfar/db.h>
#include <gwenhywfar/version.h>

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


/* Qt4 include files */
#include <QtCore>
#include <QtGui>

class QCRChipCard
{
	public:
		QCRChipCard();
		~QCRChipCard();

		QString getDDVCardData ( QListWidget *qlw );
		QString getProcessorCardData ( QListWidget *qlw );
		QString getLibchipcardVersion();
		QString getGwenhywfarVersion();

		/* TODO Format Cards */
		bool formatMemoryCard( QListWidget *qlw );

		/* TODO Funtion Memorycard */
		bool writeMemoryCardData ( QString data, QListWidget *qlw );
		QStringList  readMemoryCardData ( QTreeWidget *qtw, QListWidget *qlw );		

		/* TODO Funtion German Health Insurance Cards (KVK) */
		QMap<QString, QString> getKVKCardData ( QListWidget *qlw );

		/* TODO Funtion Moneycard */
		QStringList getMoneyCardData( QListWidget *qlw );
		QStringList getMoneyCardTransaction( QListWidget *qlw, QTreeWidget *qtw, QTreeWidget *qtw1, QProgressBar *qpb = 0 );
		
		QMap<QString, double> getMoneyCardMoneyData ( QListWidget *qlw );

	private:
		bool init( QListWidget *qlw );
		bool deinit( LC_CARD *card, LC_CLIENT * cl, LC_CLIENT_RESULT res );
		QString errorMsg ( LC_CARD *card, LC_CLIENT_RESULT res, QListWidget *qlw );

		unsigned int memCap;

		QListWidgetItem *qlwItem;
		QStringList moneyCardData, moneyCardAccData, moneyCardTransactionData;
		QMap<QString, QString> kvkCardData;
		QStringList memCardData;
		QMap<QString, double> moneyCardMoneyData;
		
		LC_CLIENT *cl;
		LC_CLIENT_RESULT res;
		
		LC_CARD *card;		
		
		LC_GELDKARTE_BLOG_LIST2 *bll;
		LC_GELDKARTE_LLOG_LIST2 *Lll;
		LC_GELDKARTE_VALUES *values;
		
		GWEN_DB_NODE *dbData;
		GWEN_BUFFER *memBuff;
		
		int rv;
};
