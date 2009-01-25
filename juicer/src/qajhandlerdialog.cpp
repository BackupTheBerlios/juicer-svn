//
// C++ Implementation: 
//
// Description: 
//
//
// Author: Matthias Reif <matthias.reif@informatik.tu-chemnitz.de>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "qajhandlerdialog.h"

QAjHandlerDialog::QAjHandlerDialog( const QString& caption,
                                    const QString & text,
                                    QDialogButtonBox::StandardButtons buttons,
                                    QStyle::StandardPixmap icon,
                                    QWidget* parent /*= 0*/,
                                    Qt::WFlags fl /*= 0*/ )
  : QDialog( parent, fl ), Ui::HandlerDialog()
  , dontAskAgain_( false )
{
    setupUi( this );
    buttonBox->setStandardButtons(buttons);
    setWindowTitle( caption );
    QWindowsStyle qStyle;
    iconLabel->setPixmap(qStyle.standardIcon(icon).pixmap(32,32));
    setText(text);

    connect ( chkAskAgain, SIGNAL( stateChanged(int) ), this, SLOT( reaskSlot(int) ) );
}


void QAjHandlerDialog::setText( const QString& text )
{
    textLabel->setText( text );
}

void QAjHandlerDialog::setIcon( const QPixmap& pixmap )
{
    iconLabel->setPixmap( pixmap );
}

bool QAjHandlerDialog::dontAskAgain() const
{
    return dontAskAgain_;
}

void QAjHandlerDialog::reaskSlot( int state )
{
    dontAskAgain_ = ( state == Qt::Checked );
}


/*!
    \fn QAjHandlerDialog::exec(const QString& saveString)
    if the dialog was already answered permanently, return the saved answer,
    otherwise the dialog is shown and the answer will be saved
 */
int QAjHandlerDialog::exec(const QString& saveString)
{
    if(QAjOptionsDialog::hasSetting("accepted", saveString)) {
        return QAjOptionsDialog::getGroupSetting("accepted", saveString).toInt();
    } else {
        int result = QDialog::exec();
        if(chkAskAgain->isChecked()) {
            QAjOptionsDialog::setSetting("accepted", saveString, result);
        } else {
            QAjOptionsDialog::removeSetting("accepted", saveString);
        }
        return result;
    }
}
