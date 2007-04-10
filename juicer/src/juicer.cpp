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

#include "juicer.h"

Juicer::Juicer( ) : QMainWindow( )
{
    char* mode = getenv( "AJQTGUI_MODE" );
#ifdef AJQTGUI_MODE_SPECIAL
    special = true;
#else
    special = (( mode != NULL )) && ( strcmp(mode, "SPECIAL\0") == 0 );
#endif
    filesystemSeparator = "\\";
    zeroTime = QDateTime( QDate(1970,1,1), QTime(0,0), Qt::UTC );

    linkServer = new QAjServerSocket( QAjApplication::APP_PORT );
    connect( linkServer, SIGNAL( lineReady( QString ) ), this, SLOT( linkServerLine( QString ) ) );

    setWindowIcon(QIcon(":/juicer.png"));

    QTabWidget* ajTab = new QTabWidget(this);
    ajDownloadWidget = new QAjDownloadWidget( ajTab );
    ajUploadWidget = new QAjUploadWidget( ajTab );
    ajSearchWidget = new QAjSearchWidget( ajTab );
    ajServerWidget = new QAjServerWidget( ajTab );
    ajShareWidget = new QAjShareWidget( filesystemSeparator, ajTab );

    ajTab->setTabToolTip( ajTab->addTab( ajDownloadWidget, QIcon(":/small/down.png"), "Downloads" ), "dowloads" );
    ajTab->setTabToolTip( ajTab->addTab( ajUploadWidget, QIcon(":/small/up.png"), "Uploads" ), "uploads" );
    ajTab->setTabToolTip( ajTab->addTab( ajSearchWidget, QIcon(":/small/searching.png"), "Search" ), "servers" );
    ajTab->setTabToolTip( ajTab->addTab( ajServerWidget, QIcon(":/small/server.png"), "Server" ), "searches" );
    ajTab->setTabToolTip( ajTab->addTab( ajShareWidget, QIcon(":/small/shares.png"), "Shares" ), "shares" );

    ajFtpWidget = new QAjFtpWidget( ajTab );
    ajTab->setTabToolTip( ajTab->addTab( ajFtpWidget, QIcon(":/small/ftp.png"), "Ftp" ), "ftp" );

    setCentralWidget( ajTab );
    prevTab = ajDownloadWidget;

    networkDialog = new QAjNetworkDialog( this );
    optionsDialog = new QAjOptionsDialog( this );

    initToolBars();

    file = new QMenu( tr("&AppleJuice"), this );
    menuBar()->addMenu( file );

    file->addAction( QIcon(":/small/configure.png"), tr("C&onfigure"), this, SLOT( showOptions() ), QKeySequence( Qt::CTRL+Qt::Key_O ) );
    file->addAction( QIcon(":/small/network.png"), tr("&Net Info"), networkDialog, SLOT( exec() ), QKeySequence( Qt::CTRL+Qt::Key_N ) );
    file->addSeparator();
    file->addAction( QIcon(":/small/exit.png"), tr("&Exit Core"), this, SLOT( exitCore() ), QKeySequence( Qt::CTRL+Qt::Key_E ) );
    file->addAction( QIcon(":/small/close.png"), tr("&Quit GUI"), qApp, SLOT( closeAllWindows() ), QKeySequence( Qt::CTRL+Qt::Key_Q ) );

    downloadMenuBar = menuBar()->addMenu( /*tr("&Download"), */ajDownloadWidget->popup );
    serverMenuBar = menuBar()->addMenu( /*tr("&Server"), */ajServerWidget->popup );
    shareMenuBar = menuBar()->addMenu( /*tr("&Share"), */ajShareWidget->popup );
    searchMenuBar = menuBar()->addMenu( /*tr("&Search"),*/ ajSearchWidget->popup );

    menuBar()->addSeparator();

    help = new QMenu( this );
    help->setTitle( tr("&Help") );
    help->addAction( tr("&About"), this, SLOT(about()), QKeySequence( Qt::Key_F1  ) );
    help->addAction( tr("About &Qt"), this, SLOT(aboutQt()) );
    help->addSeparator();
// 	help->addAction( tr("What's &This"), this, SLOT(whatsThis()), QKeySequence( Qt::SHIFT+Qt::Key_F1 ) );
    menuBar()->addMenu( help );

    downSpeedLabel = new QLabel(this);
    upSpeedLabel = new QLabel(this);
    creditsLabel = new QLabel(this);
    downSizeLabel = new QLabel(this);
    upSizeLabel = new QLabel(this);
    coreVersionLabel = new QLabel(this);
    connectedLabel = new QLabel(this);

    statusBar()->addPermanentWidget( connectedLabel );
    statusBar()->addPermanentWidget( coreVersionLabel );
    statusBar()->addPermanentWidget( downSpeedLabel );
    statusBar()->addPermanentWidget( upSpeedLabel );
    statusBar()->addPermanentWidget( downSizeLabel );
    statusBar()->addPermanentWidget( upSizeLabel );
    statusBar()->addPermanentWidget( creditsLabel );

    QSettings lokalSettings;
    lokalSettings.beginGroup( "MainWindow" );
    resize( lokalSettings.value( "size", QSize(1000, 600) ).toSize() );
    move( lokalSettings.value( "pos", QPoint(100, 100) ).toPoint() );
    lokalSettings.endGroup();

    xml = new QXMLModule( this );
    connect( xml, SIGNAL( settingsReady( AjSettings ) ), this, SLOT( settingsReady( AjSettings ) ) );
    connect( xml, SIGNAL( error( int ) ), this, SLOT( xmlError( int ) ) );
    connect( xml, SIGNAL( gotSession() ), this, SLOT( gotSession() ) );
    connect( xml, SIGNAL( modifiedDone( ) ), ajDownloadWidget, SLOT( updateView( ) ) );
    connect( xml, SIGNAL( modifiedDone( ) ), this, SLOT( firstModified() ) );

    serverHttp = new QHttp();
    connect( serverHttp, SIGNAL( requestFinished ( int , bool ) ), this, SLOT( gotServer( int , bool ) ) );

    connected = false;
    password = lokalSettings.value( "password" ).toString();
    // no password in local file? => ask for it
    if ( password == NULL )
    {
        bool ok;
        password = QInputDialog::getText( this, "Juicer", "Enter core password:", QLineEdit::Password,  QString::null, &ok );
        if ( !ok ) // user canceld
            qApp->closeAllWindows();
        else
        {
            // save password in local file if user wants it
            if ( lokalSettings.value( "savePassword", "false" ).toString() == "true" )
                lokalSettings.setValue( "password", password );
        }
    }
    xml->setPassword( password );
    xml->setHost( lokalSettings.value("coreAddress", "localhost").toString(),
                  lokalSettings.value("/progeln.de/Juicer/xmlPort", 9851).toInt() );

    ftp = new QFtp();

    timer = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( timerSlot() ) );
    partListTimer = new QTimer( this );
    connect( partListTimer, SIGNAL( timeout() ), this, SLOT( partListTimerSlot() ) );

    connect( ajDownloadWidget, SIGNAL( pause() ), this, SLOT( pauseDownload() ) );
    connect( ajDownloadWidget, SIGNAL( resume() ), this, SLOT( resumeDownload() ) );
    connect( ajDownloadWidget, SIGNAL( cancel() ), this, SLOT( cancelDownload() ) );
    connect( ajDownloadWidget, SIGNAL( clean() ), this, SLOT( cleanDownload() ) );
    connect( ajDownloadWidget, SIGNAL( partListRequest() ), this, SLOT( partListRequest() ) );
    connect( ajDownloadWidget, SIGNAL( rename() ), this, SLOT( renameDownload() ) );
    connect( ajDownloadWidget, SIGNAL( renamePlus() ), this, SLOT( renamePlusDownload() ) );
    connect( ajDownloadWidget, SIGNAL( open() ), this, SLOT( openDownload() ) );

    connect( ajServerWidget, SIGNAL( remove() ), this, SLOT( removeServer() ) );
    connect( ajServerWidget, SIGNAL( connect() ), this, SLOT( connectServer() ) );
    connect( ajServerWidget, SIGNAL( find() ), this, SLOT( findServer() ) );

    connect( ajSearchWidget, SIGNAL( remove() ), this, SLOT( cancelSearch() ) );
    connect( ajSearchWidget, SIGNAL( download() ), this, SLOT( downloadSearch() ) );
    connect( ajSearchWidget, SIGNAL( itemDoubleClicked ( QTreeWidgetItem *, int ) ), this, SLOT( downloadSearch() ) );
    
    connect( ajTab, SIGNAL( currentChanged( QWidget* ) ), this, SLOT( tabChanged( QWidget* ) ) );

    connect( ajDownloadWidget, SIGNAL( itemSelectionChanged( ) ), this, SLOT( downloadSelectionChanged( ) ) );

    connect( ajServerWidget, SIGNAL( newSelection( bool ) ), connectServerButton, SLOT( setEnabled( bool ) ) );
    connect( ajServerWidget, SIGNAL( newSelection( bool ) ), removeServerButton, SLOT( setEnabled( bool ) ) );

    connect( ajShareWidget, SIGNAL( newSelection( bool ) ), removeShareButton, SLOT( setEnabled( bool ) ) );

    connect( ajShareWidget, SIGNAL( insert() ), this, SLOT( addShare() ) );
    connect( ajShareWidget, SIGNAL( remove() ), this, SLOT( removeShare() ) );
    connect( ajShareWidget, SIGNAL( reload() ), this, SLOT( reloadShare() ) );
    connect( ajShareWidget, SIGNAL( commit() ), this, SLOT( applyShare() ) );

    connect( ftp, SIGNAL( listInfo ( QUrlInfo ) ), this->ajFtpWidget, SLOT( insert( QUrlInfo ) ) );
    connect( ajFtpWidget, SIGNAL( itemDoubleClicked ( QTreeWidgetItem*, int ) ), this, SLOT( storeFtp( ) ) );
    connect( ajFtpWidget, SIGNAL( newSelection( bool ) ), storeFtpButton, SLOT( setEnabled( bool ) ) );

    login();
}

Juicer::~Juicer()
{
    delete file;
    delete help;
    delete downSpeedLabel;
    delete upSpeedLabel;
    delete creditsLabel;
    delete downSizeLabel;
    delete upSizeLabel;
    delete coreVersionLabel;
    delete connectedLabel;
    delete xml;
    delete serverHttp;
    delete timer;
    delete partListTimer;
}

void Juicer::initToolBars()
{
    QToolBar* ajTools = new QToolBar( "applejuice operations", this );
    ajTools->setToolTip( "applejuice operations" );

    ajTools->addAction( QIcon(":/configure.png"), "configure", this, SLOT( showOptions() ) )->setToolTip("configure");
    ajTools->addAction( QIcon(":/network.png"), "aj network info", this, SLOT( showNetworkInfo() ) )->setToolTip("aj network info");

    ajTools->addAction( QIcon(":/adjust.png"), "adjust columns", this, SLOT( adjustColumns() ) )->setToolTip("adjust colmuns");

    QToolBar* ajLinks = new QToolBar( "applejuice links", this );
    ajLinks->setToolTip( "applejuice links" );

    clipboardButton = ajLinks->addAction( QIcon(":/wizard.png"), "process link from clipboard", this, SLOT( processClipboard() ) );
    clipboardButton->setToolTip( "process link from clipboard" );

    ajAddressLabel = new QLabel(ajLinks);
    ajAddressLabel->setText("ajfsp link:");
    ajAddressLabel->adjustSize();
    ajLinks->addWidget( ajAddressLabel );

    ajAddressEdit = new QLineEdit(ajLinks);
    ajLinks->addWidget( ajAddressEdit );
    connect( ajAddressEdit, SIGNAL( returnPressed() ), this, SLOT( processLink() ) );

    ajLinks->addAction( QIcon(":/ok.png"), "process link", this, SLOT( processLink() ) );

// DOWNLOAD TOOLBAR

    downloadToolBar = new QToolBar( "download operations", this );

    pauseDownloadButton = downloadToolBar->addAction( QIcon(":/pause.png"), "pause download", this, SLOT( pauseDownload() ) );
    pauseDownloadButton->setToolTip( "pause download" );

    resumeDownloadButton = downloadToolBar->addAction( QIcon(":/resume.png"), "resume download", this, SLOT( resumeDownload() ) );
    resumeDownloadButton->setToolTip( "resume download" );

    cancelDownloadButton = downloadToolBar->addAction( QIcon(":/cancel.png"), "cancel download", this, SLOT( cancelDownload() ) );

    partListButton = downloadToolBar->addAction( QIcon(":/partlist.png"), "show part list", this, SLOT( partListRequest() ) );
    renameDownloadButton = downloadToolBar->addAction( QIcon(":/rename.png"), "rename download", this, SLOT( renameDownload() ) );
    renamePlusDownloadButton = downloadToolBar->addAction( QIcon(":/rename_plus.png"), "rename download by clipboard", this, SLOT( renamePlusDownload() ) );

    openDownloadButton = downloadToolBar->addAction( QIcon(":/exec.png"), "open download", this, SLOT( openDownload() ) );

    clearDownloadButton = downloadToolBar->addAction( QIcon(":/filter.png"), "remove finished/canceld download", this, SLOT( cleanDownload() ) );

    saveDownloadButton = downloadToolBar->addAction( QIcon(":/save.png"), "store file", this, SLOT( storeDownload() ) );


    pauseDownloadButton->setDisabled( true );
    resumeDownloadButton->setDisabled( true );
    cancelDownloadButton->setDisabled( true );
    partListButton->setDisabled( true );
    renameDownloadButton->setDisabled( true );
    renamePlusDownloadButton->setDisabled( true );
    saveDownloadButton->setDisabled( true );
    openDownloadButton->setDisabled( true );

    downloadToolBar->addSeparator();

    powerCheck = new QCheckBox( downloadToolBar );
    powerCheck->setText( tr("Power Download:") );
    powerCheck->setChecked( false );
    powerCheck->adjustSize();
    downloadToolBar->addWidget( powerCheck );

    powerSpin = new QDoubleSpinBox( downloadToolBar );
    powerSpin->setRange( 2.2, 50.0 );
    powerSpin->setSingleStep( 0.1 );
    powerSpin->setDecimals( 1 );
    downloadToolBar->addWidget( powerSpin );

    //connect( powerEdit, SIGNAL( returnPressed() ), this, SLOT( applyPowerDownload() ) );
    connect( powerSpin, SIGNAL( valueChanged( const QString&) ), this, SLOT( powerChanged( const  QString& ) ) );
    connect( powerSpin, SIGNAL( valueChanged( double ) ), this, SLOT( powerChanged( double ) ) );

    powerOkButton = downloadToolBar->addAction( QIcon(":/ok.png"), "apply power download", this, SLOT( applyPowerDownload() ) );

    powerMaxButton = downloadToolBar->addAction( QIcon(":/launch.png"), "set all downloads to 1:50", this, SLOT( maxPowerDownload() )  );
    powerMaxButton->setVisible( special );

// UPLOAD TOOLBAR

    uploadToolBar = new QToolBar( "upload operations", this );

    uploadToolBar->addAction( QIcon(":/dummy.png"), "" )->setDisabled( true );
    uploadToolBar->hide();

// SEARCH TOOLBAR

    searchToolBar = new QToolBar( "search operations", this );

    searchToolBar->addAction( QIcon(":/save.png"), "download", this, SLOT( downloadSearch() ) );
    searchToolBar->addAction( QIcon(":/cancel.png"), "cancel search", this, SLOT( cancelSearch() ) );

    searchToolBar->addSeparator();

    ajSearchLabel = new QLabel( searchToolBar );
    ajSearchLabel->setText( "search for:" );
    searchToolBar->addWidget( ajSearchLabel );
    ajSearchEdit = new QLineEdit( searchToolBar );
    ajSearchEdit->setMinimumWidth( 200 );
    searchToolBar->addWidget( ajSearchEdit );
    connect( ajSearchEdit, SIGNAL( returnPressed() ), this, SLOT( search() ) );
    ajSearchButton = searchToolBar->addAction( "search", this, SLOT( search() )  );

    searchToolBar->hide();

// SERVER TOOLBAR

    serverToolBar = new QToolBar( "server operations", this );

    connectServerButton = serverToolBar->addAction( QIcon(":/connect.png"), "connect to this server", this, SLOT( connectServer() ) );
    removeServerButton = serverToolBar->addAction( QIcon(":/cancel.png"), "remove server", this, SLOT( removeServer() ) );
    findServerButton = serverToolBar->addAction( QIcon(":/find.png"), "find server", this, SLOT( findServer() ) );

    removeServerButton->setDisabled( true );
    connectServerButton->setDisabled( true );
    serverToolBar->hide();

// SHARE TOOLBAR

    shareToolBar = new QToolBar( "share operations", this );

    shareToolBar->addAction( QIcon(":/add.png"), "add share", this, SLOT( addShare() ) );
    removeShareButton = shareToolBar->addAction( QIcon(":/remove.png"), "remove share", this, SLOT( removeShare() ) );
    reloadSharedFilesButton = shareToolBar->addAction( QIcon(":/update.png"), "reload shared files", this, SLOT( reloadShare() ) );
    applyShareButton = shareToolBar->addAction( QIcon(":/commit.png"), "commit changes to the core", this, SLOT( applyShare() ) );
    applyShareButton->setDisabled( true );
    removeShareButton->setDisabled( true );

    

    shareToolBar->hide();

    // FTP TOOLBAR
    ftpToolBar = new QToolBar( "ftp operations", this );
    ftpToolBar->addAction( QIcon(":/reload.png"), "reload files", this, SLOT( reloadFtp() ) );
    storeFtpButton = ftpToolBar->addAction( QIcon(":/save.png"), "store file", this, SLOT( storeFtp() ) );
    storeFtpButton->setDisabled( true );
    ftpToolBar->hide();

    this->setIconSize( QSize(22, 22) );
    this->addToolBar( ajTools );
    this->addToolBar( ajLinks );
    this->addToolBarBreak( );
    downloadToolBar->setIconSize( QSize(22, 22) );
    this->addToolBar( downloadToolBar );
    this->addToolBar( uploadToolBar );
    this->addToolBar( searchToolBar );
    this->addToolBar( serverToolBar );
    this->addToolBar( shareToolBar );
    this->addToolBar( ftpToolBar );

}

void Juicer::closeEvent( QCloseEvent* ce )
{
    QSettings lokalSettings;
    lokalSettings.beginGroup("MainWindow");
    lokalSettings.setValue( "size", size() );
    lokalSettings.setValue( "pos", pos() );
    lokalSettings.endGroup();
    ce->accept();
}

void Juicer::about()
{
    QMessageBox::about( this, tr("Juicer Info"),
                        tr("Juicer \n\nhttp://ajqtgui.sf.net\nhttp://www.progeln.de"));
}

void Juicer::aboutQt()
{
    QMessageBox::aboutQt( this, tr("Juicer: About Qt") );
}

void Juicer::timerSlot()
{
    if ( xml->session == "" )
        return;
    xml->get( "modified" );
}

void Juicer::partListTimerSlot()
{
    QString id = ajDownloadWidget->getNextIdRoundRobin();
    if (!id.isEmpty())
    {
        xml->get( "downloadpartlist", "&simple&id=" + id);
        ajDownloadWidget->doItemsLayout();
    }
}

void Juicer::showOptions()
{
    xml->get( "settings" );
    if ( optionsDialog->exec() == QDialog::Accepted )
    {
        // save options
        AjSettings settings = optionsDialog->getAjSettings();
        QSettings lokalSettings;
        lokalSettings.setValue( "coreAddress",  settings.coreAddress );
        lokalSettings.setValue( "savePassword",  settings.savePassword );
        lokalSettings.setValue( "showSplash",  settings.showSplash );
        lokalSettings.setValue( "serverURL",  settings.serverURL );
        lokalSettings.setValue( "refresh",  settings.refresh );

        lokalSettings.setValue( "launcher",  settings.launcher );
        lokalSettings.setValue( "location",  settings.location );
        lokalSettings.setValue( "incomingDirSpecific",  settings.incomingDirSpecific );
        lokalSettings.setValue( "tempDirSpecific",  settings.tempDirSpecific );
        
        lokalSettings.setValue( "ftpServer",  settings.ftpServer );
        lokalSettings.setValue( "ftpPort",  settings.ftpPort );
        lokalSettings.setValue( "ftpUser",  settings.ftpUser );
        lokalSettings.setValue( "ftpPassword",  settings.ftpPassword );
        lokalSettings.setValue( "ftpDir",  settings.ftpDir );

        timer->stop();
        timer->setSingleShot( false );
        timer->start( lokalSettings.value( "refresh", 3 ).toInt() * 1000 );

        // save password in local file if user want's it
        if ( settings.savePassword == "true" )
            lokalSettings.setValue( "password", password );
        else
            lokalSettings.remove( "password" );

        QString settingsString = "";
        settingsString += "&Nickname=" + settings.nick;
        settingsString += "&Port=" + settings.tcpPort;
        settingsString += "&XMLPort=" + settings.xmlPort;
        settingsString += "&AllowBrowse=" + settings.allowBrowse;
        settingsString += "&MaxUpload=" + QString::number(settings.maxUp.toInt() * 1024);
        settingsString += "&MaxDownload=" + QString::number(settings.maxDown.toInt() * 1024);
        settingsString += "&MaxSourcesPerFile=" + settings.maxSources;
        settingsString += "&MaxConnections=" + settings.maxCon;
        settingsString += "&AutoConnect=" + settings.autoconnect;
        settingsString += "&Speedperslot=" + settings.maxSlot;
        settingsString += "&Incomingdirectory=" + settings.incomingDir;
        settingsString += "&Temporarydirectory=" + settings.tempDir;
        settingsString += "&MaxNewConnectionsPerTurn=" + settings.maxNewCon;
        printf( "%s\n", settingsString.toAscii().data() );
        xml->set( "setsettings", settingsString );
    }
}

void Juicer::settingsReady( AjSettings settings )
{
    tempDir = QFileInfo( settings.tempDir );
    incomingDir = QFileInfo( settings.incomingDir );
    if ( optionsDialog != NULL )
    {
        QSettings lokalSettings;
        settings.coreAddress = lokalSettings.value( "coreAddress", "localhost" ).toString();
        settings.savePassword = lokalSettings.value( "savePassword",  "false" ).toString();
        settings.showSplash = lokalSettings.value( "showSplash",  "true" ).toString();
        settings.serverURL = lokalSettings.value( "serverURL",  "http://www.applejuicenet.de/18.0.html" ).toString();
        settings.refresh = lokalSettings.value( "refresh", 3 ).toInt();

        settings.launcher = lokalSettings.value( "launcher", optionsDialog->launchCombo->itemText(0)).toString();

        settings.location = lokalSettings.value( "location", "same" ).toString();
        settings.tempDirSpecific = lokalSettings.value( "tempDirSpecific", "/" ).toString();
        settings.incomingDirSpecific = lokalSettings.value( "incomingDirSpecific", "/" ).toString();
        
        settings.ftpServer = lokalSettings.value( "ftpServer", "localhost" ).toString();
        settings.ftpPort = lokalSettings.value( "ftpPort", "21" ).toString();
        settings.ftpUser = lokalSettings.value( "ftpUser", "anonymous" ).toString();
        settings.ftpPassword = lokalSettings.value( "ftpPassword", "" ).toString();
        settings.ftpDir = lokalSettings.value( "ftpDir", "/" ).toString();

        optionsDialog->setAjSettings( settings );
    }
}

bool Juicer::login()
{
    firstModifiedCnt = 0;
    ajDownloadWidget->clear();
//	ajUploadWidget->clear();
    ajServerWidget->clear();
    ajSearchWidget->clear();
    connected = false;
    qApp->processEvents();
    xml->get( "getsession" );
    return true;
}

void Juicer::setStatusBarText( QString downSpeed, QString upSpeed, QString credits, QString downSize, QString upSize )
{
    downSpeedLabel->setText( tr("Downstream: ") + QConvert::bytes( downSpeed ) + tr("/s") );
    upSpeedLabel->setText( tr("Upstream: ") + QConvert::bytes( upSpeed ) + tr("/s") );
    creditsLabel->setText( tr("Credits: ") + QConvert::bytesExtra( credits ) );
    downSizeLabel->setText( tr("Downloaded: ") + QConvert::bytesExtra( downSize ) );
    upSizeLabel->setText( tr("Uploaded: ") + QConvert::bytesExtra( upSize ) );
}

void Juicer::xmlError( int code )
{
    connected = false;
    timer->stop();
    partListTimer->stop();
    QSettings lokalSettings;
    QString errorString;
    if ( code == 302 )
        errorString = "Either wrong password or connection lost.";
    else
        errorString = xml->errorString() + ".";

    QAjLoginDialog *loginDialog = new QAjLoginDialog(
                                      lokalSettings.value("coreAddress", "localhost").toString(),
                                      lokalSettings.value("xmlPort", "9851").toString(),
                                      lokalSettings.value("/progeln.de/Juicer/password", "").toString(),
                                      errorString, this, "logionDialog" );
    if ( loginDialog->exec() == QDialog::Accepted )
    {
        password = loginDialog->getPassword();
        xml->abort();
        xml->setPassword( loginDialog->getPassword() );
        xml->setHost( loginDialog->getHost(), loginDialog->getPort().toInt() );
        lokalSettings.setValue( "coreAddress", loginDialog->getHost() );
        lokalSettings.setValue( "xmlPort", loginDialog->getPort() );
        if ( lokalSettings.value( "savePassword", "false" ).toString() == "true" )
            lokalSettings.setValue( "password",  password);
        login();
    }
    delete loginDialog;
}

void Juicer::gotSession()
{
    connected = true;
    xml->get( "information" );
    xml->get( "settings" );
    processQueuedLinks();
    QSettings lokalSettings;
    timerSlot();
    timer->setSingleShot( false );
    timer->start( lokalSettings.value( "refresh", 3 ).toInt() * 1000 );
    partListTimer->setSingleShot( false );
    partListTimer->start( 3000 );
}

void Juicer::showNetworkInfo()
{
    networkDialog->exec();
}

void Juicer::powerChanged( const QString& )
{
    powerCheck->setChecked( true );
}

void Juicer::applyPowerDownload()
{
    float value;
    if ( powerCheck->isChecked() )
        value = powerSpin->value();
    else
        value = 1.0;
    processSelected( "setpowerdownload", "&Powerdownload=" + QConvert::power( value ) );
}

void Juicer::maxPowerDownload()
{
    QList<QString> ids = ajDownloadWidget->getIds();
    int i;
    for ( i=0; i<ids.size(); i++ )
    {
        xml->set( "setpowerdownload", "&Powerdownload="+QConvert::power( 50 )+"&id="+ids[i] );
    }
}

void Juicer::processSelected( QString request, QString para )
{
    QList<QTreeWidgetItem *>  selectedItems = ajDownloadWidget->selectedItems();
    int i;
    for ( i=0; i<selectedItems.size(); i++ )
    {
        xml->set( request, para + "&id=" + selectedItems.at(i)->text(ID_DOWN_INDEX) );
    }
}

void Juicer::requestSelected( QString request, QString para )
{
    QList<QTreeWidgetItem *>  selectedItems = ajDownloadWidget->selectedItems();
    int i;
    for ( i=0; i<selectedItems.size(); i++ )
    {
        xml->get( request, para + "&id=" + selectedItems.at(i)->text(ID_DOWN_INDEX) );
    }
}

void Juicer::processLink()
{
    QString link = QString( QUrl::toPercentEncoding( ajAddressEdit->text().trimmed() ) );
    xml->set( "processlink", "&link=" + link );
    ajAddressEdit->clear();
}

void Juicer::processClipboard()
{
    QString link = QString( QUrl::toPercentEncoding( qApp->clipboard()->text( QClipboard::Clipboard ).trimmed() ) );
    xml->set( "processlink", "&link=" + link );
}

void Juicer::downloadSearch()
{
    QList<QTreeWidgetItem *>  selectedItems = ajSearchWidget->selectedItems();
    int i;
    for ( i=0; i<selectedItems.size(); i++ )
    {
        if ( selectedItems.at(i)->text( CHECKSUM_SEARCH_INDEX ) != "" )
        {
            QAjSearchItem *searchItem = (QAjSearchItem*)selectedItems.at(i);
            QString link = "ajfsp://file|";
            link += searchItem->text( TEXT_SEARCH_INDEX );
            link += "|" + searchItem->text( CHECKSUM_SEARCH_INDEX );
            link += "|" + searchItem->size + "/";
            link = QString( QUrl::toPercentEncoding(link) );
            xml->set( "processlink", "&link=" +link );
        }
    }
}

void Juicer::cancelDownload()
{
    if ( QMessageBox::question( this, "Confirm", "Do you realy want to cancel this download(s)?", QMessageBox::No, QMessageBox::Yes ) == QMessageBox::Yes )
    {
        processSelected( "canceldownload" );
    }
}

void Juicer::cleanDownload()
{
    xml->set( "cleandownloadlist" );
}
void Juicer::pauseDownload()
{
    processSelected( "pausedownload" );
}
void Juicer::resumeDownload()
{
    processSelected( "resumedownload" );
}

void Juicer::partListRequest( )
{
    requestSelected( "downloadpartlist" );
}

void Juicer::renameDownload()
{
    QString oldFilename;
    QString newFilename;
    bool ok;
    QList<QTreeWidgetItem *>  selectedItems = ajDownloadWidget->selectedItems();
    int i;
    for ( i=0; i<selectedItems.size(); i++ )
    {
        oldFilename = selectedItems[i]->text( FILENAME_DOWN_INDEX );
        newFilename = QInputDialog::getText( this, "rename download", "enter new filename for " + oldFilename, QLineEdit::Normal, oldFilename, &ok );
        newFilename = QString( QUrl::toPercentEncoding( newFilename ) );
        if ( ok && !newFilename.isEmpty() )
        {
            xml->set( "renamedownload", "&id=" + selectedItems.at(i)->text( ID_DOWN_INDEX ) + "&name=" + newFilename );
        }
    }
}

void Juicer::renamePlusDownload()
{
    QString oldFilename;
    QString newFilename;
    QString newFilenameBase = qApp->clipboard()->text( QClipboard::Clipboard );
    QList<QTreeWidgetItem *>  selectedItems = ajDownloadWidget->selectedItems();
    int i;
    for ( i=0; i<selectedItems.size(); i++ )
    {
        oldFilename = selectedItems[i]->text( FILENAME_DOWN_INDEX );
        newFilename = newFilenameBase;
        if (selectedItems.size() > 1)
        {
            newFilename += "_" + QString::number(i+1);
        }
        QStringList s = oldFilename.split(".");
        if (s.size() > 1)
        {
            newFilename += "." + s[s.size() - 1];
        }
        newFilename = QString( QUrl::toPercentEncoding( newFilename ) );
        if (!newFilename.isEmpty())
        {
            xml->set( "renamedownload", "&id=" + selectedItems[i]->text( ID_DOWN_INDEX ) + "&name=" + newFilename );
        }
    }
}


void Juicer::removeServer()
{
    QList<QTreeWidgetItem *>  selectedItems = ajServerWidget->selectedItems();
    int i;
    for ( i=0; i<selectedItems.size(); i++ )
    {
        xml->set( "removeserver", "&id=" + selectedItems.at(i)->text(ID_SERVER_INDEX) );
    }
}

void Juicer::connectServer()
{
    QList<QTreeWidgetItem *>  selectedItems = ajServerWidget->selectedItems();
    if ( ! selectedItems.empty() )
    {
        xml->set( "serverlogin", "&id=" + selectedItems.first()->text(ID_SERVER_INDEX) );
    }
}

void Juicer::addShare()
{
    QString dir = QFileDialog::getExistingDirectory( this, "Choose a directory" );
    if ( dir != "" )
    {
        int result = QMessageBox::question( this, "question", "Share subdirectories?", QMessageBox::Yes, QMessageBox::No );
        QString mode;
        if ( result == QMessageBox::Yes )
            mode = "subdirectory";
        else
            mode = "directory";
        ajShareWidget->insertShare( dir, mode );
    }
    ajShareWidget->setChanged();
    applyShareButton->setEnabled( true );
}

void Juicer::removeShare()
{
    QList<QTreeWidgetItem *>  selectedItems = ajShareWidget->selectedItems();
    int i;
    for ( i=0; i<selectedItems.size(); i++ )
    {
        //delete it.current();
        selectedItems.at(i)->setFlags( 0 );
    }
    ajShareWidget->setChanged();
    applyShareButton->setEnabled( true );
}

void Juicer::applyShare()
{
    QString sharesString;
    int i;
    int cnt = 1;
    for ( i=0; i<ajShareWidget->topLevelItemCount() ; i++)
    {
        QTreeWidgetItem* item = ajShareWidget->topLevelItem( i );
        if ( item->flags() & Qt::ItemIsEnabled )
        {
            sharesString += "&sharedirectory" + QString::number(cnt) + "=" + item->text( PATH_SHARE_INDEX );
            sharesString += "&sharesub" + QString::number(cnt) + "=" + ((QAjShareItem*)item)->recursiv;
            cnt++;
        }
    }
    sharesString = "&countshares=" + QString::number( cnt-1 );
    xml->set( "setsettings", sharesString );
    xml->get( "settings" );
    ajShareWidget->setChanged( false );
    applyShareButton->setDisabled( true );
}

QString Juicer::getSelectedDownloads()
{
    QString ids = "";
    QList<QTreeWidgetItem *>  selectedItems = ajDownloadWidget->selectedItems();
    int i;
    for ( i=0; i<selectedItems.size(); i++ )
    {
        if ( i == 0 )
            ids += "&id=" + selectedItems.at(i)->text(ID_DOWN_INDEX);
        else
            ids += "&id" + QString::number(i) + "=" + selectedItems.at(i)->text(ID_DOWN_INDEX);
    }
    return ids;
}

void Juicer::tabChanged( QWidget *tab )
{
    QSettings lokalSettings;
    if ( tab != ajDownloadWidget )
    {
        downloadToolBar->hide();
        downloadMenuBar->setDisabled( true );
    }
    else
    {
        downloadToolBar->show();
        downloadMenuBar->setEnabled( true );
    }
    if ( tab != ajUploadWidget )
    {
        uploadToolBar->hide();
    }
    else
    {
        uploadToolBar->show();
    }
    if ( tab != ajSearchWidget )
    {
        searchToolBar->hide();
        searchMenuBar->setDisabled( true );
    }
    else
    {
        searchToolBar->show();
        searchMenuBar->setEnabled( true );
    }
    if ( tab != ajServerWidget )
    {
        serverToolBar->hide();
        serverMenuBar->setDisabled( true );
    }
    else
    {
        serverToolBar->show();
        serverMenuBar->setEnabled( true );
    }
    if ( tab != ajShareWidget )
    {
        shareToolBar->hide();
        shareMenuBar->setDisabled( true );
    }
    else
    {
        shareToolBar->show();
        shareMenuBar->setEnabled( true );
    }
    if ( (prevTab == ajShareWidget) && ( ajShareWidget->wasChanged() ) )
    {
        if ( QMessageBox::question( this, "question", "You've changed your shares.\nDo you want to transfer it to the core?", QMessageBox::Yes, QMessageBox::No ) == QMessageBox::Yes)
        {
            applyShare();
        }
    }
    if ( tab != ajFtpWidget )
    {
        ftpToolBar->hide();
    }
    else
    {
        ftpToolBar->show();
        reloadFtp();
    }
    prevTab = tab;
}

void Juicer::downloadSelectionChanged( )
{
    bool oneSelected = false;
    bool onePaused = false;
    bool oneActive = false;
    bool oneFinished = false;

    QList<QTreeWidgetItem *>  selectedItems = ajDownloadWidget->selectedItems();
    int i;
    for ( i=0; i<selectedItems.size(); i++ )
    {
        oneSelected = true;
        QAjDownloadItem *downloadItem = (QAjDownloadItem*)selectedItems.at(i);
        if ( downloadItem->getStatus() == DOWN_PAUSED )
            onePaused = true;
        if ( ( downloadItem->getStatus() == DOWN_SEARCHING ) || ( downloadItem->getStatus() == DOWN_LOADING ) )
            oneActive = true;
        if ( downloadItem->getStatus() == DOWN_FINISHED )
            oneFinished = true;
        if ( oneSelected && onePaused && oneActive && oneFinished )
            break;
    }
    cancelDownloadButton->setEnabled( oneSelected );
    partListButton->setEnabled( oneSelected );
    renameDownloadButton->setEnabled( oneSelected );
    renamePlusDownloadButton->setEnabled( oneSelected );
    saveDownloadButton->setEnabled( oneFinished );
    resumeDownloadButton->setEnabled( onePaused );
    pauseDownloadButton->setEnabled( oneActive );
    openDownloadButton->setEnabled( oneSelected );
}

void Juicer::exitCore()
{
    if ( QMessageBox::question( this, "Confirm", "Do you realy want to exit the core?\n All your credits will be lost!", QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
    {
        xml->set( "exitcore" );
    }
}

void Juicer::search()
{
    QString text( QUrl::toPercentEncoding( ajSearchEdit->text() ) );
    xml->set( "search", "&search=" + text );
    ajSearchEdit->clear();
}

void Juicer::cancelSearch()
{
    QList<QTreeWidgetItem *>  selectedItems = ajSearchWidget->selectedItems();
    int i;
    for ( i=0; i<selectedItems.size(); i++ )
    {
        xml->set( "cancelsearch", "&id=" + selectedItems.at(i)->text( ID_SEARCH_INDEX ));
    }
}

void Juicer::findServer()
{
    QSettings lokalSettings;
    QString serverURL = lokalSettings.value( "serverURL", "http://www.applejuicenet.de/18.0.html" ).toString();

    QString host;
    if ( serverURL.indexOf( "http://", 0, Qt::CaseInsensitive ) == 0 )
        host = serverURL.section( '/', 2, 2);
    else
        host = serverURL.section( '/', 0, 0);

    printf("get server from: %s\n", host.toLatin1().data() );

    serverHttp->setHost( host );
    serverHttp->get( serverURL );
}

void Juicer::gotServer( int , bool error )
{
    if ( error )
    {
        QMessageBox::critical( NULL, "error", "Could not fetch server source." , QMessageBox::Abort, QMessageBox::Cancel );
    }
    else
    {
        QString data( serverHttp->readAll() );
        // TODO: use regular expressions
        int begin, start, end;
        begin = 0;
        QString link;
        // while we found an ajfsp server link
        while ( ( start = data.indexOf( "ajfsp://server|", begin, Qt::CaseInsensitive ) ) != -1 )
        {
            end = data.indexOf( "/", start + 15 );
            link = QString( QUrl::toPercentEncoding( data.mid(start, end - start +1) ) );
            xml->set( "processlink", "&link=" + link );
            begin = end;
        }
    }
}

void Juicer::reloadShare()
{
    ajShareWidget->clear();
    xml->get( "settings" );
    xml->get( "share" );
}

void Juicer::setCoreVersion( QString version )
{
    coreVersionLabel->setText( "Core: " + version );
}

void Juicer::connectedSince( QString since )
{
    if( since != "0" )
    {
        QString time = zeroTime.addMSecs( since.toULongLong() ).toLocalTime().toString( Qt::LocalDate );
        connectedLabel->setText( "connected since " + time );
    }
    else
    {
        connectedLabel->setText( "NOT connected" );
    }
}

void Juicer::firstModified()
{
    if (firstModifiedCnt < 5)
    {
        if (firstModifiedCnt == 4)
        {
            ajDownloadWidget->updateView( true );
            ajDownloadWidget->adjustSizeOfColumns();
            ajDownloadWidget->adjustSizeOfColumns();
            ajUploadWidget->adjustSizeOfColumns();
            ajServerWidget->adjustSizeOfColumns();
            ajShareWidget->adjustSizeOfColumns();
            ajDownloadWidget->sortItems( 0, Qt::AscendingOrder );
            ajUploadWidget->sortItems( 0, Qt::AscendingOrder );
            ajServerWidget->sortItems( 0, Qt::AscendingOrder );
            ajShareWidget->sortItems( 0, Qt::AscendingOrder );
            this->show();
        }
        firstModifiedCnt++;
    }
}

void Juicer::linkServerLine( QString line )
{
    line = QString( QUrl::toPercentEncoding( line.trimmed() ) );
    xml->set( "processlink", "&link=" + line );
}

void Juicer::processQueuedLinks()
{
    while ( ! queuedLinks.isEmpty() )
    {
        QString link = QString( QUrl::toPercentEncoding( queuedLinks.takeFirst() ) );
        xml->set( "processlink", "&link=" + link );
    }
}

void Juicer::queueLinks( QStringList links )
{
    queuedLinks = links;
}


/*!
    \fn Juicer::powerChanged( double )
 */
void Juicer::powerChanged( double )
{
    applyPowerDownload();
}

void Juicer::storeDownload()
{
    FTP* ftp = NULL;
    QString filename, localDir;
    QList<QTreeWidgetItem *>  selectedItems = ajDownloadWidget->selectedItems();

    QSettings lokalSettings;
    QString server = lokalSettings.value( "ftpServer", "localhost" ).toString();
    int port = lokalSettings.value( "ftpPort", "21" ).toInt();
    QString user = lokalSettings.value( "ftpUser", "anonymous" ).toString();
    QString password = lokalSettings.value( "ftpPassword", "" ).toString();
    QString dir = lokalSettings.value( "ftpDir", "/" ).toString();
    if ( ! dir.endsWith( '/' ) )
    {
        dir += '/';
    }
    ftp = new FTP( server, port, user, password, true, this );

    int i;
    for ( i=0; i<selectedItems.size(); i++ )
    {
        filename = selectedItems.at(i)->text( FILENAME_DOWN_INDEX );
        localDir = QFileDialog::getExistingDirectory( this, "save \"" + filename + "\" + to" );
        if ( localDir != "" )
        {
            if ( ! localDir.endsWith( QDir::separator() ) )
            {
                localDir += QDir::separator();
            }
            QFile* dstFile = new QFile( localDir + filename );
            if ( ! dstFile->exists() )
            {
                dstFile->open( QIODevice::WriteOnly );
                ftp->add( dir + filename, dstFile );
            }
        }
    }
    ftp->start();
}


/*!
    \fn Juicer::reloadFtp()
 */
void Juicer::reloadFtp()
{
    ajFtpWidget->clear();

    QSettings lokalSettings;
    QString server = lokalSettings.value( "ftpServer", "localhost" ).toString();
    int port = lokalSettings.value( "ftpPort", "21" ).toInt();
    QString user = lokalSettings.value( "ftpUser", "anonymous" ).toString();
    QString password = lokalSettings.value( "ftpPassword", "" ).toString();
    QString dir = lokalSettings.value( "ftpDir", "/" ).toString();

    if ( ftp->state() != QFtp::Unconnected )
    {
        ftp->close();
    }
    ftp->connectToHost( server, port );
    ftp->login( user, password );
    ftp->setTransferMode( QFtp::Passive );
    ftp->list( dir );
}


/*!
    \fn Juicer::storeFtp()
 */
void Juicer::storeFtp()
{
    QString filename, localDir;
    QList<QTreeWidgetItem *>  selectedItems = ajFtpWidget->selectedItems();

    QSettings lokalSettings;
    QString server = lokalSettings.value( "ftpServer", "localhost" ).toString();
    int port = lokalSettings.value( "ftpPort", "21" ).toInt();
    QString user = lokalSettings.value( "ftpUser", "anonymous" ).toString();
    QString password = lokalSettings.value( "ftpPassword", "" ).toString();
    QString dir = lokalSettings.value( "ftpDir", "/" ).toString();
    if ( ! dir.endsWith( '/' ) )
    {
        dir += '/';
    }
    FTP* ftp = new FTP( server, port, user, password, true, this );

    int i;
    for ( i=0; i<selectedItems.size(); i++ )
    {
        filename = selectedItems.at(i)->text( FILENAME_FTP_INDEX );
        localDir = QFileDialog::getExistingDirectory( this, "save \"" + filename + "\" + to" );
        if ( localDir != "" )
        {
            if ( ! localDir.endsWith( QDir::separator() ) )
            {
                localDir += QDir::separator();
            }
            QFile* dstFile = new QFile( localDir + filename );
            if ( ! dstFile->exists() )
            {
                dstFile->open( QIODevice::WriteOnly );
                ftp->add( dir + filename, dstFile );
            }
            else
            {
                QMessageBox::critical( this, "error", "\"" + dstFile->fileName() + "\" already exists", QMessageBox::Ok, QMessageBox::NoButton );
            }
        }
    }
    ftp->start();
}

/*!
    \fn Juicer::openDownload( QList<QTreeWidgetItem *>  items )
 */
void Juicer::openDownload( QList<QTreeWidgetItem *>  items )
{
    QSettings lokalSettings;
    QString launcher = lokalSettings.value( "launcher", optionsDialog->launchCombo->itemText(0)).toString().simplified();
    QStringList args = launcher.split(" ");

    QString exec = args.takeFirst();
    if ( launcher == optionsDialog->kdeLauncher )
    {
        args.removeFirst();
        args.push_front("exec");
    }
    else if ( launcher == optionsDialog->gnomeLauncher )
    {
        args.removeFirst();
    }
    else if ( launcher == optionsDialog->macLauncher )
    {
        exec = "open";
        args.clear();
    }
    else if ( launcher == optionsDialog->winLauncher )
    {
        exec = "start";
        args.clear();
        args.push_back("\"\"");
    }

    QString iDir, tDir;
    // determine the path
    QString location = lokalSettings.value( "location", "same" ).toString();
    if( location == "specific" )
    {
        iDir = lokalSettings.value( "incomingDirSpecific", "/" ).toString() + QDir::separator();
        tDir = lokalSettings.value( "tempDirSpecific", "/" ).toString() + QDir::separator();
    }
    else
    {
        iDir = incomingDir.absolutePath() + QDir::separator();
        tDir = tempDir.absolutePath() + QDir::separator();
    }

    int i;
    for (i=0; i<items.size(); i++)
    {
        QAjItem* ajItem = (QAjItem*)items[i];
        if (ajItem->getType() == DOWN)
        {
            QAjDownloadItem* ajDownloadItem = (QAjDownloadItem*)ajItem;
            QString filename;
            if( ajDownloadItem->getStatus() == DOWN_FINISHED )
            {
                args.push_back( iDir + ajDownloadItem->text( FILENAME_DOWN_INDEX ) );
            }
            else
            {
                args.push_back( tDir + ajDownloadItem->getTempNumber() + ".data" );
            }
            QProcess::startDetached( exec, args );
            args.pop_back();
        }
    }
}

/*!
    \fn Juicer::openDownload( QTreeWidgetItem *item, int )
 */
void Juicer::openDownload( QTreeWidgetItem *item, int )
{
    QList<QTreeWidgetItem *> items;
    items << item;
    openDownload( items );
}

/*!
    \fn Juicer::openDownload()
 */
void Juicer::openDownload()
{
    openDownload( ajDownloadWidget->selectedItems() );
}

/*!
    \fn Juicer::setUploadFilename( QString shareId, QString filename )
 */
void Juicer::setUploadFilename( QString shareId, QString filename )
{
    QFileInfo fileInfo(filename);
    if (tempDir.absolutePath() == fileInfo.absolutePath())
    {
        QAjDownloadItem* item = ajDownloadWidget->findDownloadByTempNum( fileInfo.baseName() );
        if ( item != NULL )
            ajUploadWidget->setFilename( shareId, item->text( FILENAME_DOWN_INDEX ) );
    }
    else
    {
        ajUploadWidget->setFilename( shareId, fileInfo.fileName() );
    }
}


/*!
    \fn Juicer::adjustColumns()
 */
void Juicer::adjustColumns()
{
    if ( ajDownloadWidget->isVisible() )
        ajDownloadWidget->adjustSizeOfColumns();
    if ( ajUploadWidget->isVisible() )
        ajUploadWidget->adjustSizeOfColumns();
    if ( ajServerWidget->isVisible() )
        ajServerWidget->adjustSizeOfColumns();
    if ( ajShareWidget->isVisible() )
        ajShareWidget->adjustSizeOfColumns();
}
