#include "imwidget.h"

ImWidget::ImWidget(QWidget *parent)
    : QWidget(parent)
{
	ui.setupUi(this);
}

ImWidget::~ImWidget()
{

}

void ImWidget::new_incoming_im(QString from, QString text)
{
	if (!text.isEmpty()) {
		ui.historyEdit->append(from + QString(": ") + text);
	}
	ui.textEdit->setFocus();
}

void ImWidget::setHandle(QString newhandle)
{
	this->imHandle = newhandle;
}

QString ImWidget::getHandle()
{
	return this->imHandle;
}

void ImWidget::on_sendButton_clicked() {
	emit new_outgoing_im(this->imHandle, ui.textEdit->toPlainText());
	ui.historyEdit->append(QString("<i>you: </i>") + ui.textEdit->toPlainText());
	ui.textEdit->clear();
	ui.textEdit->setFocus();
}


//void ImWidget::closeEvent(QCloseEvent *event) {
//	ui.historyEdit->append(QString("closeEvent"));
//}
