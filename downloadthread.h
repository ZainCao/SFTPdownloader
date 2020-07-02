#ifndef DOWNLOADTHREAD_H
#define DOWNLOADTHREAD_H

#include <libssh2.h>
#include <libssh2_sftp.h>
#include <QTcpSocket>
#include <QListWidgetItem>

#include<QObject>
#include<QThread>

class DownloadThread : public QObject
{
    Q_OBJECT
public:
    explicit DownloadThread(QObject *parent = nullptr);
    ~DownloadThread();

    void start();

signals:

    void updateFileList(QStringList fileNameList, std::vector<unsigned long long> fileSizeList);
    void sg_init();
    void sg_close();
    void sg_progress(int percentage,int indexOfFile);
    void sg_errorInfo(QString errorMessage);
    void sg_readDir(QString sftppath);
    void sg_downloadFail(int indexOfFile);

public slots:

    int uploadFile(QString filepath);
    int downloadFile(QString downloadPath,int indexOfFile);
    int readDir(QString sftppath);
private:
    QTcpSocket *my_socket;
    LIBSSH2_SESSION *my_session;
    LIBSSH2_SFTP *sftp_session;
    const char *fingerprint;
    int iStatus;
    QString sftp_path;
    QString hostIp;
    QString hostName;
    QString hostPassword;
    //QString downloadPath;
    QStringList filelist;
    unsigned long long *filesize;
    std::vector<unsigned long long> fileSizeList;
    QThread *my_thread;

    const QString configFileName = "./Configure.ini";
private:

    void readConfigure();
    void createConfigFile();

private slots:
    int initSftpSession();
    int closeSftpSession();
    void recordProgress(unsigned long long bytes,int indexOfFile);

};

#endif // DOWNLOADTHREAD_H
