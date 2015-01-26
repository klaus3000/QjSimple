#ifndef ACCOUNTDIALOG_H
#define ACCOUNTDIALOG_H

#include <QtGui/QDialog>
#include "ui_accountdialog.h"

class AccountDialog : public QDialog {
Q_OBJECT

public:
	AccountDialog(QWidget *parent = 0);
	~AccountDialog();

	QString getDomain();
	QString getUsername();
	QString getPassword();
	QString getStun();
	QString getOutbound();
	QString getTransport();
	QString getSrtp();
	QString getSrtpReq();
	QString getCaFile();
	QString getPrivKeyFile();
	QString getCertFile();
	QString getLogFile();
	QString getXcapUrl();
	QString getLogLevel();
	int getSipPort();
	bool getSubscribe();
	bool getPublish();
	bool getNoregistration();
	bool getTlsVerifyServer();
	
	void setDomain(QString);
	void setUsername(QString);
	void setPassword(QString);
	void setStun(QString);
	void setOutbound(QString);
	void setTransport(QString);
	void setSrtp(QString);
	void setSrtpReq(QString);
	void setCaFile(QString);
	void setPrivKeyFile(QString);
	void setCertFile(QString);
	void setLogFile(QString);
	void setXcapUrl(QString);
	void setLogLevel(QString);
	void setSipPort(int);
	void setSubscribe(bool);
	void setPublish(bool);
	void setNoregistration(bool);
	void setTlsVerifyServer(bool);
		
private:
	Ui::AccountDialogClass ui;

private slots:
	void on_caPushButton_clicked();
	void on_privKeyPushButton_clicked();
	void on_certPushButton_clicked();
	void on_logFilePushButton_clicked();
};

#endif // ACCOUNTDIALOG_H
