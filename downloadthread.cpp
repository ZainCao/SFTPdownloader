#include "downloadthread.h"
#include <QFileInfo>
#include <QSettings>
#include <QMessageBox>
#include <QCoreApplication>

DownloadThread::DownloadThread(QObject *parent) : QObject(parent)
{
    iStatus = -1;
    my_thread = new QThread();
    connect(this,SIGNAL(sg_readDir(QString)),this,SLOT(readDir(QString)));
}

DownloadThread::~DownloadThread()
{
    emit sg_close();
    delete my_thread;
}

void DownloadThread::start()
{
    connect(this,SIGNAL(sg_init()),this,SLOT(initSftpSession()));
    connect(this,SIGNAL(sg_close()),this,SLOT(closeSftpSession()));
    this->moveToThread(my_thread);
    my_thread->start();
    emit sg_init();
}

int DownloadThread::initSftpSession()
{
    readConfigure();
    int rc;
    iStatus |= 1;
    rc = libssh2_init(0);
    if(rc != 0)
    {
        return -1;
    }
    iStatus |= 2;
    my_socket = new QTcpSocket();
    QString host_ip = hostIp;
    qDebug()<<"socket connectting";
    my_socket->connectToHost(host_ip,22);
    if(!my_socket->waitForConnected(3000))
    {
        qDebug("Error connecting to host %s",host_ip.toLocal8Bit().constData());
        emit sg_errorInfo("Unable connecting to host "+host_ip);
        return -1;
    }
    iStatus |= 4;
    //create a session instance
    qDebug()<<"session init";
    my_session = libssh2_session_init();
    if(!my_session)
    {
        qDebug()<<"session init failed";
        return -1;
    }

    //tell libssh2 we are blocking
    libssh2_session_set_blocking(my_session,1);
    qDebug()<<"session handshake";
    while((rc = libssh2_session_handshake(my_session,my_socket->socketDescriptor())) == LIBSSH2_ERROR_EAGAIN);
    if(rc)
    {
        qDebug()<<"handshake failed";
        return -1;
    }
    iStatus |= 8;
    fingerprint = libssh2_hostkey_hash(my_session, LIBSSH2_HOSTKEY_HASH_SHA1);

    //server default authentication methods is password.
    //qDebug()<<"use password to connect: "<<"meshon";
    while((rc = libssh2_userauth_password(my_session, hostName.toLocal8Bit().constData(), hostPassword.toLocal8Bit().constData())) == LIBSSH2_ERROR_EAGAIN);
    //qDebug()<<"log rc: "<<rc;
    switch(rc) {
        case 0:
            do
            {
                sftp_session = libssh2_sftp_init(my_session);
                if(!sftp_session)
                {
                    if(libssh2_session_last_errno(my_session) ==
                       LIBSSH2_ERROR_EAGAIN)
                    {
                        qDebug()<<"non-blocking init,wait for read\n";
                        my_socket->waitForReadyRead();
                    }
                    else
                    {
                        qDebug()<<"Unable to init SFTP session\n";
                        return -5;
                    }
                }
            }while(!sftp_session);

            emit sg_readDir(sftp_path);
            return 0;
        case -18:
            qDebug()<<"authentication failed. check configure.";
            emit sg_errorInfo("authentication failed. check configure.");
        break;
        case -16:
            qDebug()<<"file error,check private key";
        break;
    }
    return -1;
}

int DownloadThread::closeSftpSession()
{
    if (iStatus&8)
    {
        libssh2_sftp_shutdown(sftp_session);
        libssh2_session_disconnect(my_session, "Normal Shutdown");
        libssh2_session_free(my_session);
    }

    if (iStatus&4)
    {
        my_socket->close();
    }

    if (iStatus&2)
    {
        libssh2_exit();
    }
    iStatus = 0;
    return 0;
}

int DownloadThread::uploadFile(QString filepath)
{
    //open local file
    FILE *hlocalfile;
    hlocalfile = fopen(filepath.toLocal8Bit().constData(),"rb");
    if(!hlocalfile)
    {
        hlocalfile = NULL;
        return -1;
    }
    //open a sftp handle
    LIBSSH2_SFTP_HANDLE *sftp_handle;
    do{
        sftp_handle = libssh2_sftp_open(sftp_session,"/home/meshon/Pictures/0619.png",
                                        LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC,
                                          0666);
        if(!sftp_handle)
        {
            if(libssh2_session_last_errno(my_session) != LIBSSH2_ERROR_EAGAIN)
            {
                qDebug()<<"Unable to open file with SFTP,error code:"<< libssh2_session_last_errno(my_session);
                libssh2_sftp_close(sftp_handle);
                return -1;
            }
            else
            {
                qDebug()<<"non-blocking open";
                my_socket->waitForReadyRead();
            }
        }
    }while(!sftp_handle);

    //tell libssh2 we are non-blocking
    //libssh2_session_set_blocking(my_session,0);
    int rc;
    size_t nread;
    char mem[1024*10];
    char *ptr;
    do
    {
        nread = fread(mem, 1, sizeof(mem), hlocalfile);
        if(nread <= 0)
        {
            //end of file
            break;
        }
        ptr = mem;

        do
        {
            rc = libssh2_sftp_write(sftp_handle, ptr, nread);
            if(rc < 0)
            {
                qDebug()<<"ERROR"<<rc;
                break;
            }
            else {
                //rc indicates how many bytes were written this time
                ptr += rc;
                nread -= rc;
            }

        }while(nread);

    }while(1);

    //tell libssh2 we are blocking
    //libssh2_session_set_blocking(my_session,1);

    libssh2_sftp_close(sftp_handle);
    fclose(hlocalfile);
    hlocalfile = NULL;
    return 0;
}

int DownloadThread::downloadFile(QString downloadPath,int indexOfFile)
{
    //total bytes were written
    unsigned long long tt_bytes = 0;

    //open local file
    FILE *hlocalfile;

    hlocalfile = fopen((downloadPath+"/"+filelist[indexOfFile]).toLocal8Bit().constData(),"wb");
    if(!hlocalfile)
    {
        hlocalfile = NULL;
        return -1;
    }

    //open a sftp handle
    LIBSSH2_SFTP_HANDLE *sftp_handle;
    do
    {
        sftp_handle = libssh2_sftp_open(sftp_session,(sftp_path+"/"+filelist[indexOfFile]).toLocal8Bit().constData(),LIBSSH2_FXF_READ, 0);
        if(!sftp_handle)
        {
            if(libssh2_session_last_errno(my_session) != LIBSSH2_ERROR_EAGAIN)
            {
                qDebug()<<"Unable to open file with SFTP, error code: "<<libssh2_session_last_errno(my_session);
                emit sg_errorInfo("Unable to open host file: "+sftp_path+"/"+filelist[indexOfFile]);
                libssh2_sftp_close(sftp_handle);                
                fclose(hlocalfile);
                unlink((downloadPath+"/"+filelist[indexOfFile]).toLocal8Bit().constData());
                return -1;
            }
            else {
                qDebug()<<"open sftp";
                my_socket->waitForReadyRead();
            }
        }
    }while(!sftp_handle);



    //tell libssh2 we are non-blocking
    libssh2_session_set_blocking(my_session,0);
    size_t rc;
    int nread;
    char mem[1024*10];
    char *ptr;
    do
    {
        nread = libssh2_sftp_read(sftp_handle,mem,sizeof(mem));
        //qDebug()<<"nread: "<<nread;
        if(nread == 0)
        {

            //end of file
            break;
        }
        else if (nread == LIBSSH2_ERROR_EAGAIN) {
            //read again
            continue;
        }
        else if (nread < 0){
            //read file failed
            emit sg_errorInfo(filelist[indexOfFile]+" download failed.");
            emit sg_downloadFail(indexOfFile);
            fclose(hlocalfile);
            unlink((downloadPath+"/"+filelist[indexOfFile]).toLocal8Bit().constData());
            break;
        }
        ptr = mem;

        do
        {
            rc = fwrite(ptr,1,nread,hlocalfile);
            if(rc < 0)
            {
                qDebug()<<"ERROR"<<rc;
                break;
            }
            else {
                //rc indicates how many bytes were written this time
                ptr += rc;
                nread -= rc;
                tt_bytes += rc;
                recordProgress(tt_bytes,indexOfFile);
            }

        }while(nread);

    }while(1);


    //tell libssh2 we are blocking
    libssh2_session_set_blocking(my_session,1);
    libssh2_sftp_close(sftp_handle);
    fclose(hlocalfile);
    hlocalfile = NULL;
    return 0;
}

int DownloadThread::readDir(QString sftppath)
{
    int rc;
    filelist.clear();
    fileSizeList.clear();
    /* Request a dir listing via SFTP */
    LIBSSH2_SFTP_HANDLE *sftp_handle;
    do {
        sftp_handle = libssh2_sftp_opendir(sftp_session, sftppath.toLocal8Bit().constData());

        if((!sftp_handle) && (libssh2_session_last_errno(my_session) !=
                              LIBSSH2_ERROR_EAGAIN)) {
            qDebug("Unable to open dir with SFTP\n");
            return -1;
        }
    } while(!sftp_handle);
    //qDebug()<<"libssh2_sftp_opendir() is done, now receive listing!";
    do {
        char mem[512];
        LIBSSH2_SFTP_ATTRIBUTES attrs;

        /* loop until we fail */
        while((rc = libssh2_sftp_readdir(sftp_handle, mem, sizeof(mem),
                                         &attrs)) == LIBSSH2_ERROR_EAGAIN) {
            ;
        }
        if(rc > 0) {
            /* rc is the length of the file name in the mem
               buffer */

//            if(attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
//                /* this should check what permissions it
//                   is and print the output accordingly */
//                qDebug("--fix----- ");
//            }
//            else {
//                qDebug("---------- ");
//            }

//            if(attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID) {
//                qDebug()<<"%4d %4d "<<(int) attrs.uid<<"  "<<(int) attrs.gid;
//            }
//            else {
//                qDebug("   -    - ");
//            }

//            if(attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) {
//                qDebug()<<"file size: "<<attrs.filesize;
//            }

            qDebug("%s\n", mem);
            if(rc > 3)
            {
                filelist.append(mem);
                fileSizeList.push_back(attrs.filesize);
            }

        }
        else if(rc == LIBSSH2_ERROR_EAGAIN) {
            /* blocking */
            qDebug("Blocking\n");
        }
        else {
            break;
        }

    } while(1);

    libssh2_sftp_closedir(sftp_handle);
    emit updateFileList(filelist,fileSizeList);
    return 0;
}

void DownloadThread::recordProgress(unsigned long long bytes,int indexOfFile)
{
    int persentage = (double(bytes)/fileSizeList[indexOfFile])*100;
    emit sg_progress(persentage,indexOfFile);
}

void DownloadThread::readConfigure()
{
    QFileInfo file(configFileName);
    if(file.exists() == false)
    {
        qDebug()<<"Configure file doesn't exist.";
        hostIp = "10.42.0.1";
        hostName = "meshon";
        hostPassword = "meshon123";
        sftp_path = "/home/meshon/SavedVideos";
        //downloadPath = QCoreApplication::applicationDirPath();
        createConfigFile();
    }
    else {
        QSettings settings(configFileName,QSettings::IniFormat);
        settings.setIniCodec("UTF8");
        hostIp = settings.value("Configure/ip").toString();
        hostName = settings.value("Configure/hostname").toString();
        hostPassword = settings.value("Configure/password").toString();
        sftp_path = settings.value("Configure/sftppath").toString();
        //downloadPath = settings.value("Configure/downloadpath").toString();
    }
}

void DownloadThread::createConfigFile()
{
    QSettings settings(configFileName,QSettings::IniFormat);
    settings.setIniCodec("UTF8");
    settings.setValue("Configure/ip",hostIp);
    settings.setValue("Configure/hostname",hostName);
    settings.setValue("Configure/password",hostPassword);
    settings.setValue("Configure/sftppath",sftp_path);
    //settings.setValue("Configure/downloadpath",downloadPath);
}
