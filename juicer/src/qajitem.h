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
#ifndef QAJITEM_H
#define QAJITEM_H

#include <QTreeWidget>

typedef enum { DOWN, UP, USER, SEARCH, SHARE, SHARED_FILE, GENERIC } AjItemType;

/**
@author Matthias Reif
*/
class QAjItem : public QTreeWidgetItem
{
public:
    QAjItem( AjItemType type, QTreeWidget *parent = 0 );
    QAjItem( AjItemType type, QTreeWidgetItem *parent = 0 );

    virtual ~QAjItem();

    QString getStatus()
    {
        return status;
    }
    void setStatus( QString newStatus )
    {
        status = newStatus;
    }
    int getType();

protected:
    AjItemType type;
    QString status;
};

#endif