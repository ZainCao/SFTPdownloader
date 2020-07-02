#ifndef CONFIGURE_H
#define CONFIGURE_H

#include <QDialog>

namespace Ui {
class Configure;
}

class Configure : public QDialog
{
    Q_OBJECT

public:
    explicit Configure(QWidget *parent = nullptr);
    ~Configure();

private:
    Ui::Configure *ui;
    QString hostIp;
    QString hostName;
    QString hostPassword;

    const QString configFileName = "./Configure.ini";

private:

    void readConfigure();
    void createConfigFile();
    void showHostInfo();

public:

    void setHostIp(QString ip);
    void setHostName(QString name);
    void setPassword(QString password);
    QString getHostIp();
    QString getHostName();
    QString getPassword();

};

#endif // CONFIGURE_H
