#include "debugdialog.h"
#include <QFile>

#ifdef WIN32
	#define LINEFEED "\r\n"
#else
	#define LINEFEED "\n"
#endif

DebugDialog::DebugDialog(QWidget *parent)
    : QDialog(parent)
{
	ui.setupUi(this);
}

DebugDialog::~DebugDialog()
{

}

void DebugDialog::on_clearButton_clicked() {
	ui.logEdit->clear();
}

void DebugDialog::log(QString text) {
	ui.logEdit->append(text);
	/* open log file, append text and then close it */
	if (!logFile.isEmpty()) {
		QFile file(logFile);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
			ui.logEdit->append("error opening log file '" + logFile + "'");
		} else {
			file.write((text + LINEFEED).toUtf8());
			file.close();
		}
	}
}
