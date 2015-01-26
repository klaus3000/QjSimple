#ifndef IMWIDGET_H
#define IMWIDGET_H

#include <QtGui/QWidget>
#include "ui_imwidget.h"

class ImWidget : public QWidget
{
    Q_OBJECT

//protected:
//	void closeEvent(QCloseEvent *event);

public:
    ImWidget(QWidget *parent = 0);
    ~ImWidget();

	void new_incoming_im(QString from, QString text);
	void setHandle(QString);	
	QString getHandle();	

private:
    Ui::ImWidgetClass ui;
    QString imHandle;

signals:
    	void new_outgoing_im(QString to, QString text);
private slots:
	void on_sendButton_clicked();

};

#endif // IMWIDGET_H
