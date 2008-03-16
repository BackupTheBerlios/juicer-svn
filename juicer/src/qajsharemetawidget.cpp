//
// C++ Implementation: qajsharemetawidget
//
// Description: 
//
//
// Author: Holger Gerth <holger.gerth@informatik.tu-chemnitz.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "qajsharemetawidget.h"

QAjShareMetaWidget::QAjShareMetaWidget( QWidget *parent ) : QMainWindow(parent)
{
    dock = new AjShareFilesDockWidget( tr("files"), this );
    #if QT_VERSION >= 0x040300
        connect( dock, SIGNAL(dockLocationChanged( Qt::DockWidgetArea )), this, SLOT(shareFilesDockChanged(Qt::DockWidgetArea)));
    #endif

    this->sharedFilesWidget = NULL;
}


QAjShareMetaWidget::~QAjShareMetaWidget()
{
}


/*!
    \fn QAjShareMetaWidget::shareFilesDockChanged( Qt::DockWidgetArea area )
    prepared for qt 4.3
 */
void QAjShareMetaWidget::shareFilesDockChanged( Qt::DockWidgetArea area )
{
    QSettings localSettings;
    localSettings.beginGroup("FilesDock");
    localSettings.setValue("pos", area);
    localSettings.endGroup();
}


/*!
    \fn QAjShareMetaWidget::setShareWidget(QAjShareWidget* shareWidget)
 */
void QAjShareMetaWidget::setShareWidget(QAjShareWidget* shareWidget)
{
    this->shareWidget = shareWidget;
    this->setCentralWidget(shareWidget);

    shareWidget->popup->addSeparator();
    showFiles = new QAction(tr("show shared files in this directory"), this );
    showFiles->setCheckable( true );
    QObject::connect(showFiles, SIGNAL(toggled(bool)), dock, SLOT(setVisible(bool)));
    QObject::connect(showFiles, SIGNAL(toggled(bool)), this, SLOT(dockVisibleSlot(bool)));
    shareWidget->popup->addAction( showFiles );

    QSettings localSettings;
    showFiles->setChecked(localSettings.value("filesVisible", true).toBool());
}

void QAjShareMetaWidget::setShareFilesWidget(QAjShareFilesWidget* shareFilesWidget) {
    this->sharedFilesWidget = shareFilesWidget;
    dock->setWidget(sharedFilesWidget);

    QSettings localSettings;
    localSettings.beginGroup("FilesDock");
    addDockWidget((Qt::DockWidgetArea)localSettings.value("pos", Qt::RightDockWidgetArea).toInt(), dock);
    dock->setVisible(localSettings.value("visible", true).toBool());
    localSettings.endGroup();

    shareWidget->copyLinkButton = shareWidget->toolBar->addAction( QIcon(":/text_block.png"), tr("copy ajfsp link to clipboard"), sharedFilesWidget, SLOT(linkSlot()) );

    shareWidget->copyLinkButton->setEnabled( false );

    shareWidget->createLinkListButton = shareWidget->toolBar->addAction( QIcon(":/toggle_log.png"), tr("create AJ link list from selected files"), sharedFilesWidget, SLOT(createAjL()) );

    shareWidget->createLinkListButton->setEnabled( false );

    sharedFilesWidget->popup->addAction( shareWidget->copyLinkButton );
    sharedFilesWidget->popup->addAction( shareWidget->createLinkListButton );

    QObject::connect( sharedFilesWidget, SIGNAL( newSelection( bool ) ) ,
                      shareWidget->copyLinkButton, SLOT( setEnabled( bool ) ) );
    QObject::connect( sharedFilesWidget, SIGNAL( newSelection( bool ) ) ,
                      shareWidget->createLinkListButton, SLOT( setEnabled( bool ) ) );

    shareWidget->toolBar->addSeparator();

    shareWidget->prioCheck = new QCheckBox( shareWidget->toolBar );
    shareWidget->prioCheck->setText( tr("Priority:") );
    shareWidget->prioCheck->setChecked( false );
    shareWidget->prioCheck->adjustSize();
    shareWidget->toolBar->addWidget( shareWidget->prioCheck );

    shareWidget->prioSpin = new QSpinBox( shareWidget->toolBar );
    shareWidget->prioSpin->setRange( 1, 250 );
    shareWidget->prioSpin->setSingleStep( 1 );
    shareWidget->toolBar->addWidget( shareWidget->prioSpin );

    shareWidget->prioOkButton = shareWidget->toolBar->addAction( QIcon(":/ok.png"), tr("apply priority"), this, SLOT( setPrioritySlot() ) );
}


/*!
    \fn QAjShareMetaWidget::dockVisibleSlot( bool visible )
 */
void QAjShareMetaWidget::dockVisibleSlot( bool visible )
{
    dock->enabled = visible;
}

void QAjShareMetaWidget::selectionChanged( bool oneSelected )
{
    if ( oneSelected && (this->sharedFilesWidget != NULL ) ) {
      QAjShareItem *shareItem = (QAjShareItem*)this->shareWidget->selectedAjItems()[0];
      sharedFilesWidget->updateVisibleFiles( shareItem->path );
      shareWidget->copyLinkButton->setEnabled( ! sharedFilesWidget->selectedItems().isEmpty() );
      shareWidget->createLinkListButton->setEnabled( ! sharedFilesWidget->selectedItems().isEmpty() );
    }
}

void QAjShareMetaWidget::setPrioritySlot()
{
    int value = 1;
    if ( shareWidget->prioCheck->isChecked() )
        value = shareWidget->prioSpin->value();
    sharedFilesWidget->setPriority( value );
}
