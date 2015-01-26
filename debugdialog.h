#ifndef DEBUGDIALOG_H
#define DEBUGDIALOG_H

#include <QtGui/QDialog>
#include "ui_debugdialog.h"
#include "qjsimple.h"

class QjSimple;

class DebugDialog : public QDialog {
Q_OBJECT

public:
	DebugDialog(QWidget * parent = 0);
	~DebugDialog();
	QString logFile;

private slots:
	void on_clearButton_clicked();

public slots:
	void log(QString);

private:
	Ui::DebugDialogClass ui;
};

#endif // DEBUGDIALOG_H
