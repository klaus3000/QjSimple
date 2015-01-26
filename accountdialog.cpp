#include <QFileDialog>

#include "accountdialog.h"
#include <pjsua-lib/pjsua.h>

AccountDialog::AccountDialog(QWidget *parent) :
	QDialog(parent) {
	ui.setupUi(this);
	
	/* fill out combobox */
	ui.transportComboBox->addItem("UDP");
	ui.transportComboBox->addItem("TCP");
	ui.transportComboBox->addItem("TLS");
#if defined(PJ_HAS_IPV6) && PJ_HAS_IPV6
	ui.transportComboBox->addItem("UDP6");
//	ui.transportComboBox->addItem("TCP6");
#endif

	ui.srtpComboBox->addItem("disabled");
	ui.srtpComboBox->addItem("optional");
	ui.srtpComboBox->addItem("mandatory");

	ui.srtpReqComboBox->addItem("none");
	ui.srtpReqComboBox->addItem("TLS");
	ui.srtpReqComboBox->addItem("SIPS");

	ui.logLevelComboBox->addItem("0");
	ui.logLevelComboBox->addItem("1");
	ui.logLevelComboBox->addItem("2");
	ui.logLevelComboBox->addItem("3");
	ui.logLevelComboBox->addItem("4");
	ui.logLevelComboBox->addItem("5");
	ui.logLevelComboBox->addItem("6");
}

AccountDialog::~AccountDialog() {

}

QString AccountDialog::getDomain() {
	return ui.domainEdit->text();
}
QString AccountDialog::getUsername() {
	return ui.usernameEdit->text();
}
QString AccountDialog::getPassword() {
	return ui.passwordEdit->text();
}
QString AccountDialog::getStun() {
	return ui.stunEdit->text();
}
QString AccountDialog::getOutbound() {
	return ui.outboundproxyEdit->text();
}
QString AccountDialog::getTransport() {
	return ui.transportComboBox->currentText();
}
QString AccountDialog::getSrtp() {
	return ui.srtpComboBox->currentText();
}
QString AccountDialog::getSrtpReq() {
	return ui.srtpReqComboBox->currentText();
}
QString AccountDialog::getCaFile() {
	return ui.tlsCaFileEdit->text();
}
QString AccountDialog::getPrivKeyFile() {
	return ui.tlsPrivKeyFileEdit->text();
}
QString AccountDialog::getCertFile() {
	return ui.tlsCertFileEdit->text();
}
QString AccountDialog::getLogFile() {
	return ui.logFileEdit->text();
}
QString AccountDialog::getXcapUrl() {
	return ui.xcapUrlEdit->text();
}
QString AccountDialog::getLogLevel() {
	return ui.logLevelComboBox->currentText();
}
int AccountDialog::getSipPort() {
	return ui.sipPortSpinBox->value();
}
bool AccountDialog::getTlsVerifyServer() {
	if (ui.tlsVerifyServerBox->checkState() == Qt::Unchecked) {
		return false;
	} else {
		return true;
	}
}
bool AccountDialog::getSubscribe() {
	if (ui.subscribeBox->checkState() == Qt::Unchecked) {
		return false;
	} else {
		return true;
	}
}
bool AccountDialog::getPublish() {
	if (ui.publishBox->checkState() == Qt::Unchecked) {
		return false;
	} else {
		return true;
	}
}
bool AccountDialog::getNoregistration() {
	if (ui.noregistrationBox->checkState() == Qt::Unchecked) {
		return false;
	} else {
		return true;
	}
}

void AccountDialog::setDomain(QString text) {
	ui.domainEdit->setText(text);
}
void AccountDialog::setUsername(QString text) {
	ui.usernameEdit->setText(text);
}
void AccountDialog::setPassword(QString text) {
	ui.passwordEdit->setText(text);
}
void AccountDialog::setStun(QString text) {
	ui.stunEdit->setText(text);
}
void AccountDialog::setOutbound(QString text) {
	ui.outboundproxyEdit->setText(text);
}
void AccountDialog::setTransport(QString text) {
	ui.transportComboBox->setCurrentIndex(ui.transportComboBox->findText(text));
}
void AccountDialog::setSrtp(QString text) {
	ui.srtpComboBox->setCurrentIndex(ui.srtpComboBox->findText(text));
}
void AccountDialog::setSrtpReq(QString text) {
	ui.srtpReqComboBox->setCurrentIndex(ui.srtpReqComboBox->findText(text));
}
void AccountDialog::setCaFile(QString text) {
	ui.tlsCaFileEdit->setText(text);
}
void AccountDialog::setPrivKeyFile(QString text) {
	ui.tlsPrivKeyFileEdit->setText(text);
}
void AccountDialog::setCertFile(QString text) {
	ui.tlsCertFileEdit->setText(text);
}
void AccountDialog::setLogFile(QString text) {
	ui.logFileEdit->setText(text);
}
void AccountDialog::setXcapUrl(QString text) {
	ui.xcapUrlEdit->setText(text);
}
void AccountDialog::setTlsVerifyServer(bool verify) {
	if (verify) {
		ui.tlsVerifyServerBox->setCheckState(Qt::Checked);
	} else {
		ui.tlsVerifyServerBox->setCheckState(Qt::Unchecked);
	}
}
void AccountDialog::setLogLevel(QString text) {
	ui.logLevelComboBox->setCurrentIndex(ui.logLevelComboBox->findText(text));
}
void AccountDialog::setSipPort(int port) {
	ui.sipPortSpinBox->setValue(port);
}
void AccountDialog::setSubscribe(bool subscribe) {
	if (subscribe) {
		ui.subscribeBox->setCheckState(Qt::Checked);
	} else {
		ui.subscribeBox->setCheckState(Qt::Unchecked);
	}
}
void AccountDialog::setPublish(bool publish) {
	if (publish) {
		ui.publishBox->setCheckState(Qt::Checked);
	} else {
		ui.publishBox->setCheckState(Qt::Unchecked);
	}
}
void AccountDialog::setNoregistration(bool noregistration) {
	if (noregistration) {
		ui.noregistrationBox->setCheckState(Qt::Checked);
	} else {
		ui.noregistrationBox->setCheckState(Qt::Unchecked);
	}
}

void AccountDialog::on_caPushButton_clicked() {
	QString  filename = QFileDialog::getOpenFileName(this,
			tr("Select CA file"), 
			this->getCaFile(), 
			tr("Certificate Files (*.der *.pem)"));
	if (!filename.isEmpty()) {
		ui.tlsCaFileEdit->setText(filename);
	}
}
void AccountDialog::on_privKeyPushButton_clicked() {
	QString  filename = QFileDialog::getOpenFileName(this,
			tr("Select private key file"), 
			this->getPrivKeyFile(), 
			tr("Certificate Files (*.der *.pem)"));
	if (!filename.isEmpty()) {
		ui.tlsPrivKeyFileEdit->setText(filename);
	}
}
void AccountDialog::on_certPushButton_clicked() {
	QString  filename = QFileDialog::getOpenFileName(this,
			tr("Select personal certificate file"), 
			this->getCertFile(), 
			tr("Certificate Files (*.der *.pem)"));
	if (!filename.isEmpty()) {
		ui.tlsCertFileEdit->setText(filename);
	}
}
void AccountDialog::on_logFilePushButton_clicked() {
	QString  filename = QFileDialog::getSaveFileName(this,
			tr("Select log file"), 
			this->getLogFile(), 
			tr("Log Files (*.txt)"));
	if (!filename.isEmpty()) {
		ui.logFileEdit->setText(filename);
	}
}
