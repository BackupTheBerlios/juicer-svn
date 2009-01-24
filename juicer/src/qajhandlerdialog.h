//
// C++ Interface: 
//
// Description: 
//
//
// Author: Matthias Reif <matthias.reif@informatik.tu-chemnitz.de>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef QAJHANDLERDIALOG_H
#define QAJHANDLERDIALOG_H

#include <QDialog>
#include <QWindowsStyle>

#include "ui_qajhandlerdialogbase.h"
#include "qajoptionsdialog.h"

class QAjHandlerDialog : public QDialog, private Ui::HandlerDialog
{
  Q_OBJECT

    bool dontAskAgain_;

public:
    QAjHandlerDialog( const QString& caption, const QString & text,
        QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Yes | QDialogButtonBox::No,
        QStyle::StandardPixmap icon = QStyle::SP_MessageBoxQuestion,
        QWidget* parent = 0, Qt::WFlags fl = 0 );

    void setText( const QString& text );
    void setIcon( const QPixmap& pixmap );

    bool dontAskAgain() const;

protected slots:
    void reaskSlot( int );

};

#endif

