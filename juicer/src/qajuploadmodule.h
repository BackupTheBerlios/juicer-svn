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
#ifndef QAJUPLOADWIDGET_H
#define QAJUPLOADWIDGET_H

#include <QHash>
#include <QIcon>
#include <QFileInfo>
#include "qajmodulebase.h"
#include "qajuploaditem.h"
#include "qconvert.h"

/**
@author Matthias Reif
*/
class QAjUploadModule : public QAjModuleBase {
    Q_OBJECT
public:
    QAjUploadModule(Juicer* juicer);

    ~QAjUploadModule();

    bool insertUpload(const QString& id, const QString& shareId, const QString& version,
                      const QString& os, const QString& status, const QString& directState,
                      const QString& priority, const QString& nick, const QString& speed);

    bool remove(QString id);
    void setFilename(QString shareId, QString filename);
public slots:
    void adjustSizeOfColumns();
    void selectionChanged();
    void hideQueuedSlot(bool checked);
private:
    QAjUploadItem* findUpload( QString id );
    QHash<QString, QString> uploadStatusDescr;
    QHash<QString, QString> uploadDirectStateDescr;
    QHash<QString, QAjUploadItem*> uploads;
};

#endif
