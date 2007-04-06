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

#include "qxmlmodule.h"

#include "ajqtgui.h"

QXMLModule::QXMLModule(AjQtGUI *ajQtGUI, QAjTab *ajTab, QObject *parent, const char *name) : QHttp()
{
    this->ajQtGUI = ajQtGUI;
    this->ajTab = ajTab;
    timeStamp = "0";
    QObject::connect(this, SIGNAL(done( bool )), this, SLOT( httpDone( bool ) ) );
    QObject::connect(this, SIGNAL(requestFinished( int , bool )), this, SLOT(requestFinished(int, bool )));
    QObject::connect(this, SIGNAL(responseHeaderReceived( const QHttpResponseHeader&)), this, SLOT(responseHeaderReceived( const QHttpResponseHeader&)));
}

QXMLModule::~QXMLModule()
{
}

int QXMLModule::setHost( const QString & hostname, quint16 portnumber )
{
    host = hostname;
    port = portnumber;
    return QHttp::setHost( hostname, portnumber );
}

int QXMLModule::get( QString request, QString param )
{
    if(request == "modified")
    {
        param += "&session=" + session + "&timestamp=" + timeStamp;
        param += "&filter=down;uploads;user;server;search;informations";
    }

    int httpRequest = QHttp::get("/xml/" + request + ".xml?password=" + passwordMD5 + param);

    if(request == "downloadpartlist")
    {
        if(!param.contains("simple"))
            partListRequests[ httpRequest ] = param.split( "=" )[1];
        else
            partListSimpleRequests[ httpRequest ] = param.split( "=" )[1];
    }

    return httpRequest;
}

int QXMLModule::set( QString request, QString param )
{
    return QHttp::get( "/function/" + request + "?password=" + passwordMD5 + param );
}

void QXMLModule::requestFinished( int id, bool error )
{
    if ( ! error )
    {
        doc.setContent(readAll());

        QDomElement root = doc.documentElement();
        QDomNode n;

        if (root.tagName() == "applejuice")
        {
            for (n = root.firstChild(); !n.isNull(); n = n.nextSibling())
            {
                QDomElement e = n.toElement();
                if (!e.isNull())
                {
                    if ( e.tagName() == "session" )
                    {
                        session = e.attribute("id");
                        gotSession();
                    }
                    else if ( e.tagName() == "time" )
                    {
                        timeStamp = e.text();
                    }
                    else if ( e.tagName() == "shares" )
                    {
                        handleShares(n);
                    }
                    else if ( e.tagName() == "ids" )
                    {
                        handleIds(n);
                    }
                    else if ( e.tagName() == "removed" )
                    {
                        handleRemoved(e);
                    }
                    else if ( e.tagName() == "generalinformation" )
                    {
                        handleGeneralInformation(n);
                    }
                    else if ( e.tagName() == "information" )
                    {
                        ajQtGUI->setStatusBarText(
                            e.attribute("downloadspeed"),
                            e.attribute("uploadspeed"),
                            e.attribute("credits"),
                            e.attribute("sessiondownload"),
                            e.attribute("sessionupload")
                        );
                    }
                    else if ( e.tagName() == "networkinfo" )
                    {
                        handleNetworkInfo( e );
                    }
                    else if ( e.tagName() == "upload" )
                    {
                        handleUpload( e );
                    }
                    else if ( e.tagName() == "download" )
                    {
                        handleDownload( e );
                    }
                    else if ( e.tagName() == "user" )
                    {
                        handleUser( e );
                    }
                    else if ( e.tagName() == "server" )
                    {
                        handleServer( e );
                    }
                    else if ( e.tagName() == "search" )
                    {
                        handleSearch( e );
                    }
                    else if ( e.tagName() == "searchentry" )
                    {
                        handleSearchEntry( e );
                    }
                    else if ( e.tagName() == "part" )
                    {
                        handlePart( e );
                    }
                    else if ( e.tagName() == "fileinformation" )
                    {
                        partsSize = e.attribute("filesize").toULongLong();
                    }
                    else
                    {
                        cerr << "unhandled: " << e.tagName().toLatin1().data() << endl;
                    }
                }
            }
            modifiedDone();
        }
        else if (root.tagName() == "settings" )
        {
            handleSettings(root);
        }
        else
        {
            cerr << "unhandled: " << root.tagName().toLatin1().data() << endl;
        }
        handlePartList(id);
    }
}


void QXMLModule::responseHeaderReceived ( const QHttpResponseHeader & resp )
{
    if( resp.statusCode() != 200 )
    {
        error( resp.statusCode() );
    }
}

void QXMLModule::setPassword( QString password )
{
    CMD5 md5( password.toAscii().data() );
    passwordMD5 = md5.getMD5Digest();
}


/*!
    \fn QXMLModule::handleSettings( QDomElement e )
 */
void QXMLModule::handleSettings( QDomElement e )
{
    AjSettings settings;
    settings.nick = e.firstChildElement("nick").text();
    settings.tcpPort = e.firstChildElement("port").text();
    settings.xmlPort = e.firstChildElement("xmlport").text();
    settings.autoconnect = e.firstChildElement("autoconnect").text().toLower() == "true";
    settings.maxUp = e.firstChildElement("maxupload").text();
    settings.maxDown = e.firstChildElement("maxdownload").text();
    settings.maxCon = e.firstChildElement("maxconnections").text();
    settings.maxSources = e.firstChildElement("maxsourcesperfile").text();
    settings.maxSlot = e.firstChildElement("speedperslot").text();
    settings.maxNewCon = e.firstChildElement("maxnewconnectionsperturn").text();
    settings.incomingDir = e.firstChildElement("incomingdirectory").text();
    settings.tempDir = e.firstChildElement("temporarydirectory").text();
    settingsReady(settings);

    QDomElement shareE;
    for(shareE=e.firstChildElement("share").firstChildElement("directory");
        !shareE.isNull(); shareE = shareE.nextSiblingElement("directory"))
    {
        ajTab->ajShareWidget->insertShare(
            shareE.attribute("name"), shareE.attribute("sharemode"));
    }
}


/*!
    \fn QXMLModule::handleShares( QDomNode node )
 */
void QXMLModule::handleShares( QDomNode node )
{
    QDomNode shareNode;
    for (shareNode=node.firstChild();!shareNode.isNull();shareNode=shareNode.nextSibling())
    {
        QDomElement shareElement = shareNode.toElement();

    }
}


/*!
    \fn QXMLModule::handleIds( QDomNode node )
 */
void QXMLModule::handleIds( QDomNode node )
{
    QDomNode childNode;
    for (childNode=node.firstChild();!childNode.isNull();childNode=childNode.nextSibling())
    {
        QDomElement childElement = childNode.toElement();
        if(childElement.tagName().toLower() == "serverid")
        {

        }
        else if(childElement.tagName().toLower() == "uploadid")
        {

        }
        else if(childElement.tagName().toLower() == "downloadid")
        {

        }
        else if(childElement.tagName().toLower() == "userid")
        {

        }
    }
}


/*!
    \fn QXMLModule::handleNetworkInfo( QDomElement e )
 */
void QXMLModule::handleNetworkInfo( QDomElement e )
{
    ajQtGUI->networkWidget->setUsers( e.attribute("users") );
    ajQtGUI->networkWidget->setFiles( e.attribute("files") );
    ajQtGUI->networkWidget->setSize( QConvert::bytesLong( e.attribute("filesize")) );
    ajQtGUI->networkWidget->setIp( e.attribute("ip") );
    ajTab->ajServerWidget->connectedWith( e.attribute("connectedwithserverid") );
    ajQtGUI->connectedSince( e.attribute("connectedSince") );
}


/*!
    \fn QXMLModule::handleUpload( QDomElement e )
 */
void QXMLModule::handleUpload( QDomElement e )
{
    ajTab->ajUploadWidget->insertUpload(
        e.attribute("id"),
        e.attribute("shareid"),
        e.attribute("version"),
        e.attribute("operatingsystem"),
        e.attribute("status"),
        e.attribute("directstate"),
        e.attribute("priority"),
        e.attribute("nick"),
        e.attribute("speed"));
}


/*!
    \fn QXMLModule::handleUser( QDomElement e )
 */
void QXMLModule::handleUser( QDomElement e )
{
//     ajTab->ajDownloadWidget->mutex.lock();
    ajTab->ajDownloadWidget->insertUser(
        e.attribute("downloadid"),
        e.attribute("id"),
        e.attribute("filename"),
        e.attribute("speed"),
        e.attribute("status"),
        e.attribute("powerdownload"),
        e.attribute("queueposition"),
        e.attribute("operatingsystem"));
//     ajTab->ajDownloadWidget->mutex.unlock();
}


/*!
    \fn QXMLModule::handleDownload( QDomElement e )
 */
void QXMLModule::handleDownload( QDomElement e )
{
//     ajTab->ajDownloadWidget->mutex.lock();
    ajTab->ajDownloadWidget->insertDownload(
        e.attribute("id"),
        e.attribute("filename"),
        e.attribute("status"),
        e.attribute("size"),
        e.attribute("ready"),
        e.attribute("powerdownload"));
//     ajTab->ajDownloadWidget->mutex.unlock();
}


/*!
    \fn QXMLModule::handleServer( QDomElement e )
 */
void QXMLModule::handleServer( QDomElement e )
{
    ajTab->ajServerWidget->insertServer(
        e.attribute("id"),
        e.attribute("name"),
        e.attribute("host"),
        e.attribute("port"),
        e.attribute("lastseen"),
        e.attribute("connectiontry"));
}


/*!
    \fn QXMLModule::handleSearch( QDomElement e )
 */
void QXMLModule::handleSearch( QDomElement e )
{
    ajTab->ajSearchWidget->insertSearch(
        e.attribute("id"),
        e.attribute("searchtext"),
        e.attribute("running"),
        e.attribute("foundfiles"));
}


/*!
    \fn QXMLModule::handleSearchEntry( QDomElement e )
 */
void QXMLModule::handleSearchEntry( QDomElement e )
{
    QStringList filenames;
    QDomElement fileE;
    for(fileE=e.firstChildElement("filename"); !fileE.isNull(); fileE = fileE.nextSiblingElement("filename")) {
        filenames.append(fileE.attribute("name"));
    }
    ajTab->ajSearchWidget->insertSearchEntry(
        e.attribute("id"),
        e.attribute("searchid"),
        e.attribute("size"),
        e.attribute("checksum"),
        filenames);
}


/*!
    \fn QXMLModule::handleGeneralInformation( QDomNode node )
 */
void QXMLModule::handleGeneralInformation( QDomNode node )
{
    ajQtGUI->setFilesystemSeparator(
        node.firstChildElement("filesystem").attribute("seperator"));
    ajQtGUI->setCoreVersion(node.firstChildElement("version").text());
}


/*!
    \fn QXMLModule::handleRemoved( QDomElement e )
 */
void QXMLModule::handleRemoved( QDomElement e )
{
    QDomElement objectE;
    for(objectE = e.firstChildElement("object");
        !objectE.isNull(); objectE = objectE.nextSiblingElement("object"))
    {
        QString id = objectE.attribute("id");
//         ajTab->ajDownloadWidget->mutex.lock();
        if ( ! ajTab->ajDownloadWidget->remove( id ) )
            if ( ! ajTab->ajUploadWidget->remove( id ) )
                if ( ! ajTab->ajServerWidget->remove( id ) )
                    ajTab->ajSearchWidget->remove( id );
//         ajTab->ajDownloadWidget->mutex.unlock();
    }
}


/*!
    \fn QXMLModule::httpDone(bool error)
 */
void QXMLModule::httpDone(bool error)
{
    if(error) {
        this->error(-1);
    }
}


/*!
    \fn QXMLModule::handlePart( QDomElement e )
 */
void QXMLModule::handlePart( QDomElement e )
{
    Part part;
    part.type = e.attribute("type").toInt();
    part.fromPosition = e.attribute("fromposition").toULongLong();
    partList.push_back(part);
}


/*!
    \fn QXMLModule::handlePartList( int id )
 */
void QXMLModule::handlePartList( int id )
{
    if ( partList.size() > 0 )
    {
        if( partListRequests.contains( id ) )
        {
            QAjDownloadItem* item = ajTab->ajDownloadWidget->findDownload( partListRequests[id] );
            if( item != NULL )
            {
                item->getPartListWidget()->update( partsSize, partList );
            }
            partListRequests.remove( id );
        }
        else if( partListSimpleRequests.contains( id ) )
        {
            QAjDownloadItem* item = ajTab->ajDownloadWidget->findDownload( partListSimpleRequests[id] );
            if ( item != NULL )
            {
                item->setParts( partsSize, partList );
            }
        }
        partList.clear();
    }
}
