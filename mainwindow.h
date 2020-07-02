#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>

#include "downloadthread.h"
#include "configure.h"

using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:

    //void readDir(QString sftppath);
    void sg_downloadFile(QString downloadPath,int indexOfFile);
    void sg_refresh();

private slots:

    void setAllPickUnchecked(QListWidgetItem* item);

    void loadFiles(QStringList fileNameList,std::vector<unsigned long long> fileSizeList);

    void setProgressbar(int percentage,int indexOfFile);

    void setErrorInfo(QString errorMessage);

    void refresh();

    void downloadFail(int indexOfFile);
private:
    Ui::MainWindow *ui;

//    QTcpSocket *my_socket;
//    LIBSSH2_SESSION *my_session;
//    LIBSSH2_SFTP *sftp_session;

//    const char *fingerprint;
//    int iStatus;

    QString sftp_path;

    DownloadThread *my_downloader;
//    Configure *my_Configure;
    const QString configFileName = "./Configure.ini";
    bool isCover;
    bool isWarning;

private:
    int initSftpSession();
    int closeSftpSession();
    int uploadFile(QString filepath);
    int downloadFile(QString filepath, QString filename);
    //int readDir(QString filepath);
    void addFileItem(QString filename, unsigned long long filesize);
    void readConfigure();

private slots:
    void on_download_clicked();
    void on_cBallPick_clicked(bool checked);
    void on_listWidget_itemClicked(QListWidgetItem *item);
    void on_BtnFilepath_clicked();
    void on_btnRefresh_clicked();
};

#endif // MAINWINDOW_H
