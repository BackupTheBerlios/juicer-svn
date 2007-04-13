/***************************************************************************
 *   Copyright (C) 2004 by Matthias Reif                                   *
 *   matthias.reif@informatik.tu-chemnitz.de                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "qajsearchwidget.h"

QAjSearchWidget::QAjSearchWidget( QWidget *parent ) : QAjListWidget( ID_SERVER_INDEX, parent )
{
    setColumnCount( NUM_SEARCH_COL );
    QStringList headers;
    int i;
    for ( i=0; i<NUM_SEARCH_COL; i++)
    {
        switch (i)
        {
        case ID_SEARCH_INDEX:
            headers.append( tr("id") );
            break;
        case TEXT_SEARCH_INDEX:
            headers.append( tr("search") );
            break;
        case SIZE_SEARCH_INDEX:
            headers.append( tr("size") );
            break;
        case COUNT_SEARCH_INDEX:
            headers.append( tr("hits") );
            break;
        case CHECKSUM_SEARCH_INDEX:
            headers.append( tr("MD5 checksum") );
            break;
        }
    }
    setHeaderLabels( headers );
    header()->resizeSection( TEXT_SEARCH_INDEX, 300 );
    header()->resizeSection( SIZE_SEARCH_INDEX, 100 );
    header()->resizeSection( COUNT_SEARCH_INDEX, 50 );

    setColumnHidden( ID_SEARCH_INDEX, true );
    setColumnHidden( CHECKSUM_SEARCH_INDEX, true );

    popup->setTitle( tr("&Search") );
    popup->addAction( QIcon(":/small/save.png"), "download", this, SLOT(downloadSlot()) );
    popup->addAction( QIcon(":/small/cancel.png"), "remove", this, SLOT(removeSlot()) );
}

QAjSearchWidget::~QAjSearchWidget()
{}

void QAjSearchWidget::insertSearch( QString id, QString searchText, QString running, QString foundFiles )
{
    QAjSearchItem *item = findSearch( id );
    if ( item == NULL )
    {
        item = new QAjSearchItem( this );
        searches[id] = item;
        item->setText( ID_SEARCH_INDEX, id );
        item->setText( TEXT_SEARCH_INDEX, searchText );
        item->setText( COUNT_SEARCH_INDEX, "0" );
    }
    item->setText( TEXT_SEARCH_INDEX, searchText + " (" + foundFiles + ")" );
    if ( running == "true" )
        item->setIcon( TEXT_SEARCH_INDEX, QIcon(":/small/searching.png") );
    else
        item->setIcon( TEXT_SEARCH_INDEX, QIcon(":/small/ok.png") );

}

void QAjSearchWidget::insertSearchEntry( QString id, QString searchId, QString size, QString checksum, QStringList filenames )
{
    QAjSearchItem *searchItem = findSearch( searchId );
    if ( searchItem == NULL )
    {
        searchItem = new QAjSearchItem( this );
        searches[searchId] = searchItem;
        searchItem->setText( ID_SEARCH_INDEX, searchId );
    }

    if ( searchItem->entriesCount < MAX_SEARCH_ENTRIES )
    {
        if ( searchItem->find( id ) == NULL )
        {
            QAjSearchItem *searchEntryItem = new QAjSearchItem( searchItem );
            searchItem->results[ id ] = searchEntryItem;
            searchEntries[ id ] = searchEntryItem;
            searchEntryItem->setText( ID_SEARCH_INDEX, id );
            searchEntryItem->setText( SIZE_SEARCH_INDEX, QConvert::bytes(size) );
            searchEntryItem->setText( CHECKSUM_SEARCH_INDEX, checksum );
            QStringListIterator it(filenames);
            while (it.hasNext())
            {
                QString filename = it.next();
                if ( searchEntryItem->text( TEXT_SEARCH_INDEX ) == "" )
                {
                    searchEntryItem->setText( TEXT_SEARCH_INDEX, filename);
                }
                else
                {
                    QAjSearchItem *searchSubEntryItem = new QAjSearchItem( searchEntryItem );
                    searchSubEntryItem->setText( TEXT_SEARCH_INDEX, filename);
                }
            }
            searchEntryItem->size = size;
            searchItem->hits++;
            searchItem->setText( COUNT_SEARCH_INDEX, QString::number(searchItem->hits) );
            searchItem->entriesCount++;
        }
    }
}

bool QAjSearchWidget::remove( QString id )
{
    QAjSearchItem* item = findSearch( id );
    if( item != NULL )
    {
        searches.remove( id );
        delete item;
        return true;
    }
    else
    {
        item = findSearchEntry( id );
        if ( item != NULL )
        {
            searchEntries.remove( id );
            delete item;
            return true;
        }
    }
    return false;
}

void QAjSearchWidget::removeSlot()
{
    remove();
}

void QAjSearchWidget::downloadSlot()
{
    download();
}

QAjSearchItem* QAjSearchWidget::findSearch( QString id )
{
    if (searches.contains( id ))
        return searches[ id ];
    else
        return NULL;
}

QAjSearchItem* QAjSearchWidget::findSearchEntry( QString id )
{
    if (searchEntries.contains( id ))
        return searchEntries[ id ];
    else
        return NULL;
}