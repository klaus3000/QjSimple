#include "addbuddydialog.h"

AddBuddyDialog::AddBuddyDialog(QWidget *parent) :
	QDialog(parent) {
	ui.setupUi(this);
}

AddBuddyDialog::~AddBuddyDialog() {

}

QString AddBuddyDialog::AddBuddyDialog::getName() {
	return ui.nameEdit->text();
}
QString AddBuddyDialog::AddBuddyDialog::getUri() {
	return ui.uriEdit->text();
}
bool AddBuddyDialog::AddBuddyDialog::getPresence() {
	if (ui.presenceBox->checkState() == Qt::Unchecked) {
		return false;
	} else {
		return true;
	}
}
void AddBuddyDialog::AddBuddyDialog::setName(QString name) {
	ui.nameEdit->setText(name);
}
void AddBuddyDialog::AddBuddyDialog::setUri(QString uri) {
	ui.uriEdit->setText(uri);
}
void AddBuddyDialog::AddBuddyDialog::setPresence(bool presence) {
	if (presence) {
		ui.presenceBox->setCheckState(Qt::Checked);
	} else {
		ui.presenceBox->setCheckState(Qt::Unchecked);
	}
}
