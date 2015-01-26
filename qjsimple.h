#ifndef QJSIMPLE_H
#define QJSIMPLE_H

#include <QtGui/QWidget>
#include <QList>
#include <QtNetwork/QHttp>

#include "ui_qjsimple.h"

#include "debugdialog.h"
#include "PjCallback.h"
#include "imwidget.h"
#include "buddy.h"

class DebugDialog;

class QjSimple : public QWidget {
Q_OBJECT

protected:
	void closeEvent(QCloseEvent *event);

public:
	QjSimple(QWidget *parent = 0);
	~QjSimple();
	
private:
	Ui::QjSimpleClass ui;
	void readSettings();
	void writeSettings();
	bool reallyExit();
	QString domain;
	QString username;
	QString password;
	QString stun;
	QString outbound;
	QString transport;
	QString srtp;
	QString srtpReq;
	QString caFile;
	QString privKeyFile;
	QString certFile;
	QString logFile;
	QString xcapUrl;
	QString logLevel;
	int sipPort;
	QHttp *xcapPut;
	QHttp *xcapGet;
	int xcapPutId;
	int xcapGetId;
	bool tlsVerifyServer;
	bool subscribe;
	bool subscribe_done;
	bool publish;
	bool noregistration;
	PjCallback *pjCallback;
	int initializePjSip();
	int shutdownPjSip();
	int reInitializePjSip();
	bool sipOn;
	bool onHold;
	void sendIm(QString, QString);
	void appLog(QString);
	DebugDialog *dbgWindow;
	QListWidgetItem* getBuddyItem(QString uri);
	Buddy* getBuddy(QString uri);
	Buddy* addNewBuddy(QString name, QString uri, bool presence);
	void deleteBuddy(Buddy *buddy);
	void subscribeBuddy(Buddy *buddy);
	void unsubscribeBuddy(Buddy *buddy);
	/* list which holds references to all created ImWidget (hidden or shown) */
	QList<ImWidget*> imWindowList; 
	
	/* list which holds pointers to all buddies */
	QList<Buddy*> buddies;
	
	/* pointers to char for account settings */
	char *caor, *creguri, *cdomain, *cusername, *cpassword, *cstun, *coutbound;
	
//	void test (QString);

signals:
	/* this signal forwards the log message to the debug window */
	void log(QString text);
	void showDebug();
	void shuttingDown();

public slots:
	void dump_log_message(QString text);
	void new_incoming_im(QString from, QString text);
	void nat_detect_slot(QString text, QString description);
	void new_outgoing_im(QString to, QString text);
	void buddy_double_clicked(QListWidgetItem *item);
	void buddy_state(int buddy_id);
	void reg_state_slot(int acc_id);
	void local_status_changed(QString text);
	void setCallState(QString);
	void setCallButtonText(QString);
	
private slots:
	void on_addButton_clicked();
	void on_editButton_clicked();
	void on_deleteButton_clicked();
	void on_debugButton_clicked();
	void on_callButton_clicked();
	void on_holdButton_clicked();
	void on_accountButton_clicked();
	void on_registerButton_clicked();
	void on_directCallButton_clicked();
	void on_directImButton_clicked();
	void on_xcapGetBuddyButton_clicked();
	void on_xcapPutBuddyButton_clicked();
    void xcapGetRequestFinished(int requestId, bool error);
    void xcapGetReadResponseHeader(const QHttpResponseHeader &responseHeader);
    void xcapGetUpdateDataReadProgress(int bytesRead, int totalBytes);
    void xcapGetSlotAuthenticationRequired(const QString &, QAuthenticator *);
	
};

#endif // QJSIMPLE_H
