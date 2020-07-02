#include "configure.h"
#include "ui_configure.h"
#include <QFileInfo>
#include <QDebug>
#include <QSettings>

Configure::Configure(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Configure)
{
    ui->setupUi(this);
    setWindowTitle("Configure");

    readConfigure();
}

Configure::~Configure()
{
    delete ui;
}

void Configure::readConfigure()
{
    QFileInfo file(configFileName);
    if(file.exists() == false)
    {
        qDebug()<<"Configure file doesn't exist.";
        hostIp = "192.168.0.104";
        hostName = "meshon";
        hostPassword = "meshon123";
        createConfigFile();
    }
    else {
        QSettings settings(configFileName,QSettings::IniFormat);
        settings.setIniCodec("UTF8");
        hostIp = settings.value("Configure/ip").toString();
        hostName = settings.value("Configure/hostname").toString();
        hostPassword = settings.value("Configure/password").toString();
    }
    showHostInfo();
}

void Configure::createConfigFile()
{
    QSettings settings(configFileName,QSettings::IniFormat);
    settings.setIniCodec("UTF8");
    settings.setValue("Configure/ip",hostIp);
    settings.setValue("Configure/hostname",hostName);
    settings.setValue("Configure/password",hostPassword);
}

void Configure::showHostInfo()
{
    qDebug()<<"show host info:\nip: "<<hostIp<<"\nname: "<<hostName<<"\npassword: "<<hostPassword;
    ui->LEdIP->setText(hostIp);
    ui->LEdName->setText(hostName);
    ui->LEdPass->setText(hostPassword);
}

void Configure::setHostIp(QString ip)
{
    hostIp = ip;
}

void Configure::setHostName(QString name)
{
    hostName = name;
}

void Configure::setPassword(QString password)
{
    hostPassword = password;
}

QString Configure::getHostIp()
{
    return hostIp;
}

QString Configure::getHostName()
{
    return hostName;
}

QString Configure::getPassword()
{
    return hostPassword;
}
