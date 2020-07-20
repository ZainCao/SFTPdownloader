#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMetaType>
#include <QDebug>
#include <QProgressBar>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include <QAction>

Q_DECLARE_METATYPE(std::vector<unsigned long long>)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    qRegisterMetaType<std::vector<unsigned long long>>("std::vector<unsigned long long>");
    ui->setupUi(this);
    setWindowTitle("Meshon Videos Downloader");
    setFixedSize(this->width(), this->height());
    ui->listWidget->setSelectionRectVisible(true);
    ui->listWidget->setSelectionMode(QListView::NoSelection);
    ui->listWidget->setResizeMode(QListView::Adjust);
    ui->statusBar->setSizeGripEnabled(false);
    ui->statusBar->setStyleSheet("color: red");
//    QPushButton *btnRefresh = new QPushButton(this);
//    btnRefresh->setIcon(QIcon(":/Icon/image/refresh.png"));
//    btnRefresh->setFlat(true);
//    ui->statusBar->addPermanentWidget(btnRefresh);

    isCover = false;
    isWarning = true;
//    iStatus = -1;
    sftp_path = "/home/meshon/SavedVideos";
    my_downloader = new DownloadThread();
    //my_Configure = new Configure();

    connect(my_downloader,SIGNAL(updateFileList(QStringList,std::vector<unsigned long long>)),this,SLOT(loadFiles(QStringList,std::vector<unsigned long long>)));
    connect(my_downloader,SIGNAL(sg_progress(int,int)),this,SLOT(setProgressbar(int,int)));
    connect(my_downloader,SIGNAL(sg_errorInfo(QString)),this,SLOT(setErrorInfo(QString)));
    connect(my_downloader,SIGNAL(sg_downloadFail(int)),this,SLOT(downloadFail(int)));
    //connect(this,SIGNAL(readDir(QString)),my_downloader,SLOT(readDir(QString)));
    connect(this,SIGNAL(sg_downloadFile(QString,int)),my_downloader,SLOT(downloadFile(QString,int)));
    connect(ui->btnRefresh,SIGNAL(clicked()),my_downloader,SLOT(initSftpSession()));

    //connect(ui->actionConfig,SIGNAL(triggered()),this,SLOT(openConfigure()));

    readConfigure();
    my_downloader->start();
}


MainWindow::~MainWindow()
{
    //closeSftpSession();
    delete ui;
}

void MainWindow::refresh()
{
    my_downloader->start();
}

void MainWindow::setAllPickUnchecked(QListWidgetItem* item)
{
    if(item->checkState()==Qt::Unchecked)
    {
        ui->cBallPick->setChecked(false);
    }
    disconnect(ui->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(setAllPickUnchecked(QListWidgetItem*)));
}

void MainWindow::loadFiles(QStringList fileNameList, std::vector<unsigned long long> fileSizeList)
{
    ui->listWidget->clear();
   for(int i = 0; i < fileNameList.length(); i++)
   {
       addFileItem(fileNameList[i],fileSizeList[i]);
   }
   //ui->listWidget->addItems(strlist);
   //set each item checkable box
   QListWidgetItem* item = 0;
   for(int i = 0; i < ui->listWidget->count(); ++i){
       item = ui->listWidget->item(i);
       item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
       item->setCheckState(Qt::Unchecked);

   }
}

void MainWindow::addFileItem(QString filename, unsigned long long filesize)
{
    QWidget *WContainer = new QWidget(ui->listWidget);

    QLabel *name = new QLabel();
    name->setText(filename);
    name->setMinimumWidth(180);

    double temp;
    QLabel *size = new QLabel();
    const unsigned long long GigaToBytes = 1073741824;
    const unsigned long long MegaToBytes = 1048576;
    if(filesize>GigaToBytes)
    {
        temp = double(filesize)/GigaToBytes;
        size->setText(QString::number(temp,'f',2)+"Gb");
    }
    else {
        temp = double(filesize)/MegaToBytes;
        size->setText(QString::number(temp,'f',2)+"Mb");
    }

    QProgressBar *progressBar = new QProgressBar();
    progressBar->setVisible(false);

    QHBoxLayout *Hlayout = new QHBoxLayout();

    Hlayout->addWidget(name,0);
    Hlayout->addWidget(size,1);
    Hlayout->addWidget(progressBar,2);

    WContainer->setLayout(Hlayout);
    QListWidgetItem *WContainerItem = new QListWidgetItem(ui->listWidget);
    WContainerItem->setSizeHint(QSize(40,40));

    ui->listWidget->setItemWidget(WContainerItem,WContainer);
}

void MainWindow::downloadFail(int indexOfFile)
{
    ui->listWidget->itemWidget(ui->listWidget->item(indexOfFile))->findChild<QProgressBar*>()[0].setStyleSheet("color: red");
}

void MainWindow::setProgressbar(int percentage,int indexOfFile)
{
    //ui->progressBar->setValue(percentage);
    ui->listWidget->itemWidget(ui->listWidget->item(indexOfFile))->findChild<QProgressBar*>()[0].setVisible(true);
    ui->listWidget->itemWidget(ui->listWidget->item(indexOfFile))->findChild<QProgressBar*>()[0].setValue(percentage);
    //ui->listWidget->itemWidget(ui->listWidget->item(indexOfFile))->findChild<QProgressBar*>()[0].setStyleSheet("color: red");
}

void MainWindow::readConfigure()
{
    QSettings settings(configFileName,QSettings::IniFormat);
    settings.setIniCodec("UTF8");
    QString downloadPath = settings.value("Configure/downloadpath").toString();
    if(downloadPath == "")
    {
        ui->LEfilepath->setText(QCoreApplication::applicationDirPath());
    }
    else {
        ui->LEfilepath->setText(downloadPath);
    }

}

void MainWindow::setErrorInfo(QString errorMessage)
{
    ui->statusBar->showMessage(errorMessage);
}

void MainWindow::on_download_clicked()
{
    //search checked item.
    QListWidgetItem* item = 0;
    QString dirPath = ui->LEfilepath->text();
    for(int i = 0; i < ui->listWidget->count(); ++i){
        item = ui->listWidget->item(i);
        if(item->checkState()==Qt::Checked)
        {
            QString savedFilepath = dirPath+"/"+ui->listWidget->itemWidget(item)->findChild<QLabel*>()[0].text();
            QFileInfo file_info(savedFilepath);
            if(file_info.exists() == true)
            {
                if(isWarning)
                {
                    QMessageBox::StandardButton reply;
                    QString message = ui->listWidget->itemWidget(item)->findChild<QLabel*>()[0].text()+" is existed, do you want cover it?";
                    reply = QMessageBox::warning(NULL,"Warning",message,QMessageBox::Yes|QMessageBox::YesToAll|QMessageBox::No|QMessageBox::NoToAll|QMessageBox::Cancel);
                    switch (reply) {
                    case QMessageBox::Yes:
                        isCover = true;
                        break;
                    case QMessageBox::YesToAll:
                        isCover = true;
                        isWarning = false;
                        break;
                    case QMessageBox::No:
                        isCover = false;
                        break;
                    case QMessageBox::NoToAll:
                        isCover = false;
                        isWarning = false;
                        break;
                    case QMessageBox::Cancel:
                        isCover = false;
                        isWarning = true;
                        return;
                    default:
                        break;
                    }
                }
                if(!isCover)
                {
                    qDebug()<<savedFilepath<<"is existed.";
                    continue;
                }
                qDebug()<<savedFilepath<<"is covered.";
            }
            emit sg_downloadFile(ui->LEfilepath->text(),i);
            //qDebug()<<savedFilepath;
        }
    }

    isCover = false;
    isWarning = true;
}

void MainWindow::on_cBallPick_clicked(bool checked)
{
    //set each item check state
    QListWidgetItem* item = 0;
    for(int i = 0; i < ui->listWidget->count(); ++i){
        item = ui->listWidget->item(i);
        item->setCheckState(Qt::CheckState(int(checked)*2));//0:unchecked, 1:partially checked, 2:checked.
    }

    connect(ui->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(setAllPickUnchecked(QListWidgetItem*)));
}

void MainWindow::on_listWidget_itemClicked(QListWidgetItem *item)
{
    item->setCheckState(Qt::CheckState(2-item->checkState()));
}


void MainWindow::on_BtnFilepath_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(NULL,"Directory",".");
    if("" == dir)
    {
        return;
    }
    ui->LEfilepath->setText(dir);
    QSettings settings(configFileName,QSettings::IniFormat);
    settings.setIniCodec("UTF8");
    settings.setValue("Configure/downloadpath",dir);
}

void MainWindow::on_btnRefresh_clicked()
{
    ui->listWidget->clear();
    ui->statusBar->showMessage("Refreshing...",500);
    //emit sg_refresh();
}
