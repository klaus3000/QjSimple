#include <QSettings>
#include <QMessageBox>
#include <QCloseEvent>
#include <QIcon>
#include <QMetaType>
#include <QFile>
#include <QList>
#include <QMutex>
#include <QRegExp>
#include <QtNetwork/QHttp>
#include <QWaitCondition>

#ifdef WIN32
#include <Winsock2.h>
#include <iphlpapi.h>
#include <io.h>
#include <Windns.h>
#endif

#include "qjsimple.h"
#include "addbuddydialog.h"
#include "accountdialog.h"
//#include "PjCallback.h"

extern "C" {
#include <pjlib.h>
#include <pjlib-util.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>
#include <pjsip.h>
#include <pjsip_simple.h>
#include <pjsip_ua.h>
#include <pjsua-lib/pjsua.h>

}

#define THIS_FILE "qjsimple.cpp"
#define LOGPREFIX "QjSimple: "
#define USER_AGENT "QjSimple"
#define USER_AGENT_VERSION "0.6.6"


/* global callback/logger object */
extern void *globalPjCallback;

/* global structures for pjsip config */
pjsua_config cfg;
pjsua_logging_config log_cfg;
pjsua_media_config media_cfg;
pjsua_transport_config transport_cfg;
pjsua_transport_config rtp_cfg;
pjsua_buddy_config buddy_cfg;
pjsua_acc_config acc_cfg;
pjsua_acc_id acc_id;
pj_pool_t *pool;
//pjsua_call_id call_id;
//int sipStatus; /* -1=incoming, 0=idle, 1=outgoing */
QList<int> activeCalls;
QMutex activeCallsMutex;

/* Display error */
static void error(const char *title, pj_status_t status) {
	pjsua_perror(LOGPREFIX, title, status);
}

/* Notification on incoming request
 * Handle requests which are unhandled by pjsua, eg. incoming
 * PUBLISH, NOTIFY w/o SUBSCRIBE, ...
 */
static pj_bool_t default_mod_on_rx_request(pjsip_rx_data *rdata)
{
    pjsip_tx_data *tdata;
	pjsip_status_code status_code;
	pj_status_t status;

	/* Don't respond to ACK! */
	if (pjsip_method_cmp(&rdata->msg_info.msg->line.req.method,
			&pjsip_ack_method) == 0)
		return PJ_TRUE;

	/* Create basic response. */
	if (pjsip_method_cmp(&rdata->msg_info.msg->line.req.method,
			&pjsip_notify_method) == 0) {
		/* Unsolicited NOTIFY's, send with Bad Request */
		status_code = PJSIP_SC_BAD_REQUEST;
	} else {
		/* Probably unknown method */
		status_code = PJSIP_SC_METHOD_NOT_ALLOWED;
	}
	status = pjsip_endpt_create_response(pjsua_get_pjsip_endpt(), rdata,
			status_code, NULL, &tdata);
	if (status != PJ_SUCCESS) {
		error("Unable to create response", status);
		return PJ_TRUE;
	}

	/* Add Allow if we're responding with 405 */
	if (status_code == PJSIP_SC_METHOD_NOT_ALLOWED) {
		const pjsip_hdr *cap_hdr;
		cap_hdr = pjsip_endpt_get_capability(pjsua_get_pjsip_endpt(),
				PJSIP_H_ALLOW, NULL);
		if (cap_hdr) {
			pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr*)pjsip_hdr_clone(tdata->pool, cap_hdr));
		}
	}

	/* Add User-Agent header */
	{
		pj_str_t user_agent;
		char tmp[255];
		const pj_str_t USER_AGENT_HDR = {"User-Agent", 10};
		pjsip_hdr *h;

	    pj_ansi_snprintf(tmp, sizeof(tmp), USER_AGENT " " USER_AGENT_VERSION " (pjproject %s/%s)", 
	    		pj_get_version(), PJ_OS_NAME);
		pj_strdup2_with_null(tdata->pool, &user_agent, tmp);

		h = (pjsip_hdr*) pjsip_generic_string_hdr_create(tdata->pool,
				&USER_AGENT_HDR,
				&user_agent);
		pjsip_msg_add_hdr(tdata->msg, h);
	}

	pjsip_endpt_send_response2(pjsua_get_pjsip_endpt(), rdata, tdata, NULL,
			NULL);

	return PJ_TRUE;
}

static pjsip_module mod_default_handler = 
{
    NULL, NULL,				/* prev, next.		*/
    { "mod-default-handler", 19 },	/* Name.		*/
    -1,					/* Id			*/
    PJSIP_MOD_PRIORITY_APPLICATION+99,	/* Priority	        */
    NULL,				/* load()		*/
    NULL,				/* start()		*/
    NULL,				/* stop()		*/
    NULL,				/* unload()		*/
    default_mod_on_rx_request,		/* on_rx_request()	*/
    NULL,				/* on_rx_response()	*/
    NULL,				/* on_tx_request.	*/
    NULL,				/* on_tx_response()	*/
    NULL,				/* on_tsx_state()	*/

};

QjSimple::QjSimple(QWidget *parent) :
	QWidget(parent) {
	/* initialize variables and members */
	globalPjCallback = pjCallback = 0;
	caor=creguri=cdomain=cusername=cpassword=cstun=coutbound=0;
	/* start GUI */
	ui.setupUi(this);
	/* read buddy list from QSettings */
	readSettings();
	/* add new metatypes to Qt */
//funzt leider nicht
//	qRegisterMetaType<pjsua_acc_id>("pjsua_acc_id");
//	qRegisterMetaType<pjsua_buddy_id>("pjsua_buddy_id");
	/* SIP stack status */
	sipOn = false;
	onHold = false;
	/* create Logger/Callback object */
	pjCallback = new PjCallback();
	QObject::connect((PjCallback*)globalPjCallback, SIGNAL(new_log_message(QString)),
			this, SLOT(dump_log_message(QString)), Qt::QueuedConnection);
	QObject::connect((PjCallback*)globalPjCallback, SIGNAL(new_im(QString,QString)),
			this, SLOT(new_incoming_im(QString,QString)), Qt::QueuedConnection);
	QObject::connect((PjCallback*)globalPjCallback, SIGNAL(setCallState(QString)),
			this, SLOT(setCallState(QString)), Qt::QueuedConnection);
	QObject::connect((PjCallback*)globalPjCallback, SIGNAL(setCallButtonText(QString)),
			this, SLOT(setCallButtonText(QString)), Qt::QueuedConnection);
	QObject::connect((PjCallback*)globalPjCallback, SIGNAL(buddy_state(int)),
			this, SLOT(buddy_state(int)), Qt::QueuedConnection);
	QObject::connect((PjCallback*)globalPjCallback, SIGNAL(reg_state_signal(int)),
			this, SLOT(reg_state_slot(int)), Qt::QueuedConnection);
	QObject::connect((PjCallback*)globalPjCallback, SIGNAL(nat_detect(QString,QString)),
			this, SLOT(nat_detect_slot(QString,QString)), Qt::QueuedConnection);
	/* initialize debug window */
	dbgWindow = new DebugDialog();
	if (dbgWindow) {
		dbgWindow->logFile = logFile;
	}
	/* connect log signal to log slot */
	QObject::connect(this, SIGNAL(log(QString)), dbgWindow, SLOT(log(QString)));
	QObject::connect(this, SIGNAL(showDebug()), dbgWindow, SLOT(show()));
	QObject::connect(this, SIGNAL(shuttingDown()), dbgWindow, SLOT(close()));
	/* connect signals and slots */
	QObject::connect(ui.buddyList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
			this, SLOT(buddy_double_clicked(QListWidgetItem*)));
	QObject::connect(ui.comboBox, SIGNAL(activated(QString)),
			this, SLOT(local_status_changed(QString)));
	/* fill out combobox */
	ui.comboBox->addItem (QIcon(":/icons/online"), "Online");
	ui.comboBox->addItem (QIcon(":/icons/offline"), "Offline");
	ui.directCallButton->setIcon(QIcon(":/icons/directCall"));
	ui.directImButton->setIcon(QIcon(":/icons/directIm"));
	/* set Window title */
	this->setWindowTitle(USER_AGENT " " USER_AGENT_VERSION);
	/* HTTP Requests */
	xcapPut = new QHttp(this); /* will be deleted automatically with parent */
	xcapGet = new QHttp(this);
    connect(xcapGet, SIGNAL(requestFinished(int, bool)),
            this, SLOT(xcapGetRequestFinished(int, bool)));
    connect(xcapGet, SIGNAL(dataReadProgress(int, int)),
            this, SLOT(xcapGetUpdateDataReadProgress(int, int)));
    connect(xcapGet, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
            this, SLOT(xcapGetReadResponseHeader(const QHttpResponseHeader &)));
    connect(xcapGet, SIGNAL(authenticationRequired(const QString &, QAuthenticator *)),
            this, SLOT(xcapGetSlotAuthenticationRequired(const QString &, QAuthenticator *)));
//	appLog("Test");
//	if (QFile::exists(":/icons/offline")) {
//		appLog(":/icons/offline gefunden");
//	} else {
//		appLog(":/icons/offline nicht gefunden");
//	}
//	if (QFile::exists(":/icons/foobar")) {
//		appLog(":/icons/foobar gefunden");
//	} else {
//		appLog(":/icons/foobar nicht gefunden");
//	}
}

QjSimple::~QjSimple() {
//	shutdownPjSip();
//	pjCallback->disconnect();
//	globalPjCallback=0;
//	delete pjCallback;
//	pjCallback = 0;
	dbgWindow->disconnect();
	delete dbgWindow;
	dbgWindow=0;

	ImWidget *imWindow = 0;
	while (!imWindowList.isEmpty()) {
		imWindow = imWindowList.takeFirst();
		if (imWindow) {
			delete imWindow;
		}
 	}	
}

void QjSimple::closeEvent(QCloseEvent *event) {
	if (reallyExit()) {
		writeSettings();
		shutdownPjSip();
		pjCallback->disconnect();
		globalPjCallback=0;
		delete pjCallback;
		pjCallback = 0;
		emit shuttingDown();
		event->accept();
	} else {
		event->ignore();
	}
}

void QjSimple::readSettings() {
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, "IPCom",
			"QjSimple");
	settings.setFallbacksEnabled(false);

	settings.beginGroup("SIPAccount");
	domain = settings.value("domain").toString();
	username = settings.value("username").toString();
	password = settings.value("password").toString();
	stun = settings.value("stun").toString();
	outbound = settings.value("outbound").toString();
	transport = settings.value("transport").toString();
	srtp =	settings.value("srtp").toString();
	srtpReq = settings.value("srtpReq").toString();
	caFile = settings.value("caFile").toString();
	privKeyFile = settings.value("privKeyFile").toString();
	certFile = settings.value("certFile").toString();
	logFile = settings.value("logFile").toString();
	xcapUrl = settings.value("xcapUrl").toString();
	logLevel = settings.value("logLevel").toString();
	sipPort = settings.value("sipPort").toInt();
	tlsVerifyServer = settings.value("tlsVerifyServer").toBool();
	subscribe = settings.value("subscribe").toBool();
	subscribe_done = false;
	publish = settings.value("publish").toBool();
	noregistration = settings.value("noregistration").toBool();

	settings.endGroup();

	settings.beginGroup("BuddyList");
	int count = settings.beginReadArray("buddy");
	for (int i = 0; i < count; i++) {
		settings.setArrayIndex(i);
		addNewBuddy(settings.value("Name").toString(),
				settings.value("URI").toString(),
				settings.value("Presence").toBool());
	}
	settings.endArray();
	settings.endGroup();
}

void QjSimple::writeSettings() {
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, "IPCom",
			"QjSimple");

	settings.beginGroup("SIPAccount");
	settings.setValue("domain", domain);
	settings.setValue("username", username);
	settings.setValue("password", password);
	settings.setValue("stun", stun);
	settings.setValue("outbound", outbound);
	settings.setValue("transport", transport);
	settings.setValue("srtp", srtp);
	settings.setValue("srtpReq", srtpReq);
	settings.setValue("caFile", caFile);
	settings.setValue("privKeyFile", privKeyFile);
	settings.setValue("certFile", certFile);
	settings.setValue("logFile", logFile);
	settings.setValue("xcapUrl", xcapUrl);
	settings.setValue("logLevel", logLevel);
	settings.setValue("sipPort", sipPort);
	settings.setValue("tlsVerifyServer", tlsVerifyServer);
	settings.setValue("subscribe", subscribe);
	settings.setValue("publish", publish);
	settings.setValue("noregistration", noregistration);
	settings.endGroup();

	settings.beginGroup("BuddyList");
	settings.beginWriteArray("buddy");
	
	
	Buddy * buddy;
	int i=0;
	while (buddies.size()) {
		buddy = buddies.takeFirst();
		settings.setArrayIndex(i);
		settings.setValue("Name", buddy->name);
		settings.setValue("Presence", buddy->presence);
		settings.setValue("URI", buddy->uri);
		i++;
		delete buddy;
	}
	settings.endArray();
	settings.endGroup();

	settings.sync();
}

bool QjSimple::reallyExit() {
	QMessageBox::StandardButton ret;
	ret = QMessageBox::question(this, tr("QjSimple"),
			tr("Do you really want to exit?"), QMessageBox::Yes
					| QMessageBox::No);
	if (ret == QMessageBox::Yes) {
		return true;
	} else {
		return false;
	}
}

void QjSimple::on_addButton_clicked() {
	AddBuddyDialog dialog(this);

	if (dialog.exec()) {
		addNewBuddy(dialog.getName(), dialog.getUri(), dialog.getPresence());
	}
}

void QjSimple::on_editButton_clicked() {
	QListWidgetItem *curItem = ui.buddyList->currentItem();
	if (curItem) {
		Buddy *buddy;
		buddy = getBuddy(curItem->data(Qt::UserRole).toString());
		if (buddy) {
			AddBuddyDialog dialog(this);
			dialog.setName(buddy->name);
			dialog.setUri(buddy->uri);
			dialog.setPresence(buddy->presence);
			if (dialog.exec()) {
				QString name = dialog.getName().trimmed();
				QString uri = dialog.getUri().trimmed();
				bool presence = dialog.getPresence();
				if ((name!=buddy->name) || (uri!=buddy->uri) || (presence!=buddy->presence)) {
					/* buddy was changed */
					if (subscribe) {
						unsubscribeBuddy(buddy);
					}
					deleteBuddy(buddy);
					buddy = addNewBuddy(name, uri, presence);
					if (buddy && subscribe) {
						subscribeBuddy(buddy);
					}
				} 
//				else if (presence!=buddy->presence) {
//					/* only presence was changed */
//					buddy->presence = presence;
//					if (presence) {
//						
//						subscribeBuddy(buddy);
//					} else {
//						unsubscribeBuddy(buddy);
//					}
//				}
			}
		}
	}
}

void QjSimple::on_deleteButton_clicked() {

	QListWidgetItem *curItem = ui.buddyList->currentItem();
	QMessageBox::StandardButton ret;
	ret = QMessageBox::question(this, tr("QjSimple"),
			tr("Do you really want to delete ") + curItem->text() + "?", 
			QMessageBox::Yes | QMessageBox::No);
	if (ret != QMessageBox::Yes) {
		return;
	}
	if (curItem) {
		Buddy *buddy;
		buddy = getBuddy(curItem->data(Qt::UserRole).toString());
		if (buddy) {
			deleteBuddy(buddy);
		}
	}
}

void QjSimple::on_debugButton_clicked() {
emit 				showDebug();
}

void QjSimple::on_callButton_clicked() {
	PJ_LOG(3,(THIS_FILE, "on_callButton_clicked() ...."));
	if (!sipOn) {
		QMessageBox::StandardButton ret;
		ret	= QMessageBox::warning( this, tr("QjSimple"),
				tr("You are offline! Please REGISTER first!"),
				QMessageBox::Ok);
		return;
	}
	PJ_LOG(3,(THIS_FILE, "on_callButton_clicked() .... sipOn"));
	
	if(ui.callButton->text() == "call buddy") {
		PJ_LOG(3,(THIS_FILE, "on_callButton_clicked() .... call buddy"));

		QList<QListWidgetItem *> list = ui.buddyList->selectedItems();
		if (list.isEmpty()) {
			return;
		}
		QListWidgetItem *curItem = ui.buddyList->currentItem();
		if (!curItem) {
			return;
		}
		PJ_LOG(3,(THIS_FILE, "trying to lock ...."));
		activeCallsMutex.lock();
		PJ_LOG(3,(THIS_FILE, "trying to lock .... locked"));
		if (!activeCalls.empty()) {
			PJ_LOG(3,(THIS_FILE, "ERROR: we already have an active call"));
			activeCallsMutex.unlock();
			return;
		}
		// add additional headers
		QString tempstring = ui.hname1->text().trimmed();
		QByteArray temparray1 =tempstring.toLatin1();
		pj_str_t hname1 = pj_str(temparray1.data());
		tempstring = ui.hname2->text().trimmed();
		QByteArray temparray2 =tempstring.toLatin1();
		pj_str_t hname2 = pj_str(temparray2.data());
		tempstring = ui.hval1->text().trimmed();
		QByteArray temparray3 =tempstring.toLatin1();
		pj_str_t hval1 = pj_str(temparray3.data());
		tempstring = ui.hval2->text().trimmed();
		QByteArray temparray4 =tempstring.toLatin1();
		pj_str_t hval2 = pj_str(temparray4.data());
		pjsua_msg_data msg_data;
		pjsip_generic_string_hdr header1;
		pjsip_generic_string_hdr header2;
		pjsua_msg_data_init(&msg_data);
		if (!(ui.hname1->text().trimmed().isEmpty())) {
			pjsip_generic_string_hdr_init2(&header1, &hname1, &hval1);
			pj_list_push_back(&msg_data.hdr_list, &header1);
		}
		if (!(ui.hname2->text().trimmed().isEmpty())) {
			pjsip_generic_string_hdr_init2(&header2, &hname2, &hval2);
			pj_list_push_back(&msg_data.hdr_list, &header2);
		}

		int call_id;
		QString uri = curItem->data(Qt::UserRole).toString();
		QByteArray temp = uri.toLatin1();
		pj_str_t pjuri = pj_str(temp.data());
		PJ_LOG(3,(THIS_FILE, "make call ..."));
		pj_status_t status = pjsua_call_make_call(acc_id, &pjuri, 0, 0, &msg_data, &call_id);
		if (status != PJ_SUCCESS) {
			error("Error calling buddy", status);
		} else {
			activeCalls << call_id;
			ui.callButton->setText("hang up");
		}
		activeCallsMutex.unlock();
	} else if(ui.callButton->text() == "answer call") {

		PJ_LOG(3,(THIS_FILE, "on_callButton_clicked() .... answer call"));
		PJ_LOG(3,(THIS_FILE, "trying to lock ...."));
		activeCallsMutex.lock();
		PJ_LOG(3,(THIS_FILE, "trying to lock .... locked"));
		if (activeCalls.empty()) {
			appLog("WARNING: no call to answer");
			ui.callButton->setText("call buddy");
			activeCallsMutex.unlock();
			return;
		}
		pjsua_call_answer(activeCalls.at(0),200,0,0);
		ui.callButton->setText("hang up");
		activeCallsMutex.unlock();

	} else if(ui.callButton->text() == "hang up") {
		
		PJ_LOG(3,(THIS_FILE, "on_callButton_clicked() .... hang up"));
		PJ_LOG(3,(THIS_FILE, "trying to lock ...."));
		activeCallsMutex.lock();
		PJ_LOG(3,(THIS_FILE, "trying to lock .... locked"));
		ui.holdButton->setText("hold");
		onHold = false;
		if (activeCalls.empty()) {
			appLog("WARNING: no calls to hang up");
			ui.callButton->setText("call buddy");
			activeCallsMutex.unlock();
			return;
		}
		pjsua_call_hangup(activeCalls.at(0),0,0,0);
		activeCallsMutex.unlock();
		// note: button text change will be triggered by call function
	}
	PJ_LOG(3,(THIS_FILE, "leaving on_callButton_clicked ..."));
}

void QjSimple::on_directCallButton_clicked() {
	if (!sipOn) {
		QMessageBox::StandardButton ret;
		ret	= QMessageBox::warning( this, tr("QjSimple"),
				tr("You are offline! Please REGISTER first!"),
				QMessageBox::Ok);
		return;
	}
	PJ_LOG(3,(THIS_FILE, "on_directCallButton_clicked() ...."));
	QString directCallUri = ui.directCallComboBox->currentText().trimmed();
	if (directCallUri.isEmpty()) {
		return;
	}
	if (ui.directCallComboBox->findText(directCallUri) == -1 ) {
		ui.directCallComboBox->insertItem(0,directCallUri);
	}
	if (!directCallUri.startsWith("urn:",Qt::CaseInsensitive)) {
		directCallUri.remove(QRegExp("^sip:",Qt::CaseInsensitive));
		directCallUri = "sip:" + directCallUri;
		if (!directCallUri.contains("@")) {
			directCallUri = directCallUri + "@" + domain;
		}
	}
	appLog("using as RURI: " + directCallUri);

	PJ_LOG(3,(THIS_FILE, "trying to lock ...."));
	activeCallsMutex.lock();
	PJ_LOG(3,(THIS_FILE, "trying to lock .... locked"));
	if (!activeCalls.empty()) {
		PJ_LOG(3,(THIS_FILE, "ERROR: we already have an active call"));
		activeCallsMutex.unlock();
		return;
	}
	// add additional headers
	QString tempstring = ui.hname1->text().trimmed();
	QByteArray temparray1 =tempstring.toLatin1();
	pj_str_t hname1 = pj_str(temparray1.data());
	tempstring = ui.hname2->text().trimmed();
	QByteArray temparray2 =tempstring.toLatin1();
	pj_str_t hname2 = pj_str(temparray2.data());
	tempstring = ui.hval1->text().trimmed();
	QByteArray temparray3 =tempstring.toLatin1();
	pj_str_t hval1 = pj_str(temparray3.data());
	tempstring = ui.hval2->text().trimmed();
	QByteArray temparray4 =tempstring.toLatin1();
	pj_str_t hval2 = pj_str(temparray4.data());
	pjsua_msg_data msg_data;
	pjsip_generic_string_hdr header1;
	pjsip_generic_string_hdr header2;
	pjsua_msg_data_init(&msg_data);
	if (!(ui.hname1->text().trimmed().isEmpty())) {
		pjsip_generic_string_hdr_init2(&header1, &hname1, &hval1);
		pj_list_push_back(&msg_data.hdr_list, &header1);
	}
	if (!(ui.hname2->text().trimmed().isEmpty())) {
		pjsip_generic_string_hdr_init2(&header2, &hname2, &hval2);
		pj_list_push_back(&msg_data.hdr_list, &header2);
	}

	int call_id;
	QByteArray temp = directCallUri.toLatin1();
	pj_str_t pjuri = pj_str(temp.data());
	PJ_LOG(3,(THIS_FILE, "make call ..."));
	pj_status_t status = pjsua_call_make_call(acc_id, &pjuri, 0, 0, &msg_data, &call_id);
	if (status != PJ_SUCCESS) {
		error("Error calling buddy", status);
	} else {
		activeCalls << call_id;
		ui.callButton->setText("hang up");
	}
	activeCallsMutex.unlock();
}

void QjSimple::on_directImButton_clicked() {
	PJ_LOG(3,(THIS_FILE, "on_sendButton_clicked() ...."));
	QString directCallUri = ui.directCallComboBox->currentText().trimmed();
	if (directCallUri.isEmpty()) {
		return;
	}
	if (ui.directCallComboBox->findText(directCallUri) == -1 ) {
		ui.directCallComboBox->insertItem(0,directCallUri);
	}
	directCallUri.remove(QRegExp("^sip:",Qt::CaseInsensitive));
	directCallUri = "sip:" + directCallUri;
	if (!directCallUri.contains("@")) {
		directCallUri = directCallUri + "@" + domain;
	}
	sendIm(directCallUri, "");
}

void QjSimple::sendIm(QString uri, QString name) {
	if (!sipOn) {
		QMessageBox::StandardButton ret;
		ret	= QMessageBox::warning( this, tr("QjSimple"),
				tr("You are offline! Please REGISTER first!"),
				QMessageBox::Ok);
		return;
	}

	new_incoming_im(QString("\"") + name + QString("\"<") + 
				uri + QString(">"),
				QString(""));
}

void QjSimple::on_accountButton_clicked() {
	AccountDialog dialog(this);
	int changed=0, itemp;
	bool btemp;
	QString temp;

	dialog.setDomain(domain);
	dialog.setUsername(username);
	dialog.setPassword(password);
	dialog.setStun(stun);
	dialog.setOutbound(outbound);
	dialog.setTransport(transport);
	dialog.setSrtp(srtp);
	dialog.setSrtpReq(srtpReq);
	dialog.setCaFile(caFile);
	dialog.setPrivKeyFile(privKeyFile);
	dialog.setCertFile(certFile);
	dialog.setLogFile(logFile);
	dialog.setXcapUrl(xcapUrl);
	dialog.setLogLevel(logLevel);
	dialog.setSipPort(sipPort);
	dialog.setTlsVerifyServer(tlsVerifyServer);
	dialog.setSubscribe(subscribe);
	dialog.setPublish(publish);
	dialog.setNoregistration(noregistration);
	if (dialog.exec()) {
		temp = dialog.getDomain().trimmed();
		if (temp != domain) {
			domain = temp;
			changed++;
		}
		temp = dialog.getUsername().trimmed();
		if (temp != username) {
			username = temp;
			changed++;
		}
		temp = dialog.getPassword().trimmed();
		if (temp != password) {
			password = temp;
			changed++;
		}
		temp = dialog.getStun().trimmed();
		if (temp != stun) {
			stun = temp;
			changed++;
		}
		temp = dialog.getTransport();
		if (temp != transport) {
			transport = temp;
			changed++;
		}
		temp = dialog.getOutbound().trimmed();
//		if (temp.startsWith("sip:",Qt::CaseInsensitive)) {
//			temp.remove(0,4);
//		}
		if (!temp.isEmpty()) {
			temp.remove(QRegExp("^sip:",Qt::CaseInsensitive));
			temp.remove(QRegExp(";transport=[^;]*",Qt::CaseInsensitive));
			temp.remove(QRegExp(";lr=[^;]*",Qt::CaseInsensitive));
			temp.remove(QRegExp(";lr$",Qt::CaseInsensitive));
			temp.replace(QRegExp(";lr;",Qt::CaseInsensitive),";");
			temp = temp + ";lr";
			if ( (transport == "UDP") || (transport == "UDP6") )  {
				temp = temp + ";transport=udp";
			} else if ( (transport == "TCP") || (transport == "TCP6") )  {
				temp = temp + ";transport=tcp";
			} else if ( (transport == "TLS") )  {
				temp = temp + ";transport=tls";
			}
		}
		if (temp != outbound) {
			outbound = temp;
			changed++;
		}
		temp = dialog.getSrtp();
		if (temp != srtp) {
			srtp = temp;
			changed++;
		}
		temp = dialog.getSrtpReq();
		if (temp != srtpReq) {
			srtpReq = temp;
			changed++;
		}
		temp = dialog.getCaFile();
		if (temp != caFile) {
			caFile = temp;
			changed++;
		}
		temp = dialog.getPrivKeyFile();
		if (temp != privKeyFile) {
			privKeyFile = temp;
			changed++;
		}
		temp = dialog.getCertFile();
		if (temp != certFile) {
			certFile = temp;
			changed++;
		}
		temp = dialog.getLogFile();
		if (temp != logFile) {
			logFile = temp;
			changed++;
			if (dbgWindow) {
				dbgWindow->logFile = logFile;
			}
		}
		temp = dialog.getXcapUrl();
		if (temp != xcapUrl) {
			xcapUrl = temp;
			changed++;
		}
		temp = dialog.getLogLevel();
		if (temp != logLevel) {
			logLevel = temp;
			//changed++;
			pj_log_set_level(logLevel.toInt());
		}
		itemp = dialog.getSipPort();
		if (itemp != sipPort) {
			sipPort = itemp;
			changed++;
		}
		btemp = dialog.getTlsVerifyServer();
		if (btemp != tlsVerifyServer) {
			tlsVerifyServer = btemp;
			changed++;
		}
		btemp = dialog.getSubscribe();
		if (btemp != subscribe) {
			subscribe = btemp;
			changed++;
		}
		btemp = dialog.getPublish();
		if (btemp != publish) {
			publish = btemp;
			changed++;
		}
		btemp = dialog.getNoregistration();
		if (btemp != noregistration) {
			noregistration = btemp;
			changed++;
			if (!sipOn) {
				if (noregistration)
					ui.registerButton->setText("initialize");
				else
					ui.registerButton->setText("REGISTER");
			} else {
				if (noregistration)
					ui.registerButton->setText("deinitialize");
				else
					ui.registerButton->setText("unREGISTER");
			}
		}
		if (changed) {
			if (sipOn) {
				this->reInitializePjSip();
			}
		}
	}
}

void QjSimple::on_registerButton_clicked() {
	appLog("starting initialization/registration ...");
	if (sipOn) {
		shutdownPjSip();
		if (noregistration)
			ui.registerButton->setText("initialize");
		else
			ui.registerButton->setText("REGISTER");
	} else {
		if (!initializePjSip()) {
			if (noregistration)
				ui.registerButton->setText("deinitialize");
			else
				ui.registerButton->setText("unREGISTER");
		}
	}
}

void QjSimple::new_incoming_im(QString from, QString text) {
	ImWidget *imWindow = 0;
	int i;
	
	QString fromdisplay, fromuri;
	from = from.trimmed();
	
	if ((i=from.indexOf("<")) != -1) {
		/* name-addr spec */
	appLog("name-addr spec");
		fromdisplay = from.left(i);
	appLog(fromdisplay);
		fromdisplay = fromdisplay.trimmed();
	appLog(fromdisplay);
		if (fromdisplay.contains("\"")) {
		appLog(fromdisplay);
			fromdisplay.remove(0,1);
		appLog(fromdisplay);
			fromdisplay.chop(1);
		appLog(fromdisplay);
		}
		fromuri = from.mid(i+1); /* strip display name and < */
	appLog(fromuri);
		fromuri.chop(1); /* strip > */
	appLog(fromuri);
	} else {
		/* addr spec */
	appLog("addr spec");
		fromuri = from;
	}
	
	appLog(QString("new incoming IM: from=")+from+QString(" text=")+text);
	for (int i = 0; i < imWindowList.size(); i++) {
		if (imWindowList.at(i)->getHandle() == fromuri) {
			imWindow = imWindowList.at(i);
			break;
		}
 	}
	if (!imWindow) {
		/* we do nat have a windows yet for this caller, 
		 * create a new and add it to the list */
		imWindow = new ImWidget();
		imWindow->setHandle(fromuri);
		imWindowList << imWindow;
		QObject::connect(this, SIGNAL(shuttingDown()),
				imWindow, SLOT(close()));
		QObject::connect(imWindow, SIGNAL(new_outgoing_im(QString, QString)), 
				this, SLOT(new_outgoing_im(QString, QString)));
		imWindow->setWindowTitle(from);
	}
	/* show window (if hidden) and show incoming message */
	imWindow->show();
	if (fromdisplay.isEmpty()) {
		imWindow->new_incoming_im(fromuri, text);		
	} else {
		imWindow->new_incoming_im(fromdisplay, text);		
	}
}

void QjSimple::nat_detect_slot(QString text, QString description) {
	ui.natTypeEdit->setText(text);
	ui.natTypeEdit->setToolTip(description);
}

void QjSimple::new_outgoing_im(QString to, QString text) {
	pj_status_t status;
	QByteArray tempto, temptext;
	pj_str_t pjto, pjtext;
	
	tempto   = to.toLatin1();
	temptext = text.toLatin1();
	
	pjto = pj_str(tempto.data());
	pjtext = pj_str(temptext.data());

	status = pjsua_im_send(acc_id, &pjto,
			NULL, &pjtext,
			NULL, NULL);
	if (status != PJ_SUCCESS)
		error("Error sending IM", status);
}

void QjSimple::buddy_double_clicked(QListWidgetItem *item) {
	//	new_incoming_im(QString("\"") + curItem->text() + QString("\"<") + 
	//				curItem->data(Qt::UserRole).toString() + QString(">"),
	//				QString(""));
	sendIm(item->data(Qt::UserRole).toString(), item->text());
}

void QjSimple::appLog(QString text) {
	emit log(QString(LOGPREFIX) + text);
}

void QjSimple::dump_log_message(QString text) {
	emit log(text);
}

int QjSimple::initializePjSip() {
    char tmp[256];
	QByteArray tempStun;
	pj_status_t status;

    if (sipOn) {
		appLog("SIP stack already initialized");
		return 0;
	}
	if (domain.isEmpty() || username.isEmpty() ) {
		QMessageBox::StandardButton ret;
		ret = QMessageBox::warning(
						this,
						tr("QjSimple"),
						tr("SIP account not configured! Please configure at least username and domain!"),
						QMessageBox::Ok);
		return -1;
	}
	appLog("initializing SIP stack ...");

	/* Create pjsua first! */
	status = pjsua_create();
	if (status != PJ_SUCCESS) {
		error("Error in pjsua_create()", status);
		pjsua_destroy();
		return -1;
	}
	appLog("pjsua_create succeeded ...");

	/* Create pool for application */
    pool = pjsua_pool_create("qjsimple", 1000, 1000);

	appLog("\"qjsimple\" memory pool created ...");

	pjsua_config_default(&cfg);
	/* Configure the Name Servers */
	/* We try to fetch the nameservers from the OS, because
	   pjsip supports SRV lookups only with the own resolver */
	unsigned int count = 0;
	cfg.nameserver_count=0;
	for (count=0;count<4;count++) {
		if (cfg.nameserver[count].ptr) {
			free(cfg.nameserver[count].ptr);
			cfg.nameserver[count].ptr = 0;
			cfg.nameserver[count].slen = 0;
		}
	}

#ifdef WIN32
	/* searching for nameserver on WIN32 (code from resiprocate/ares/ares_init.c) */
	/* see also http://msdn2.microsoft.com/en-us/library/aa366068.aspx */
	/*
     * Way of getting nameservers that should work on all Windows from 98 on.
     */
	FIXED_INFO *     FixedInfo;
	ULONG            ulOutBufLen;
	DWORD            dwRetVal;
	IP_ADDR_STRING * pIPAddr;
	HANDLE           hLib;
	typedef DWORD (*GNP) (FIXED_INFO*, DWORD*);
	GNP gnp;

	while(1) {
		hLib = LoadLibrary(TEXT("iphlpapi.dll"));
		if (!hLib) {
			break;
		}
		appLog("DLL loaded ...");

		gnp = (GNP) GetProcAddress((HINSTANCE__*) hLib, "GetNetworkParams");

		if (!gnp) {
			FreeLibrary((HINSTANCE__*) hLib);
			break;
		}
		appLog("GetNetworkParams found ...");
		FixedInfo = (FIXED_INFO *) GlobalAlloc(GPTR, sizeof(FIXED_INFO));
		ulOutBufLen = sizeof(FIXED_INFO);

		if (ERROR_BUFFER_OVERFLOW == gnp(FixedInfo, &ulOutBufLen) ) {
			GlobalFree(FixedInfo);
			FixedInfo = (FIXED_INFO *)GlobalAlloc(GPTR, ulOutBufLen);
		}

                dwRetVal = gnp( FixedInfo, &ulOutBufLen );
                if ( dwRetVal ) {
			GlobalFree( FixedInfo );
			FreeLibrary((HINSTANCE__*) hLib);
			break;
		}
		appLog("GetNetworkParams done ...");
		pIPAddr = &FixedInfo->DnsServerList;
		while ( pIPAddr && strlen(pIPAddr->IpAddress.String) > 0)
		{
			char *dns;
			appLog("found name server: " + QString(pIPAddr->IpAddress.String));
			dns = strdup(pIPAddr->IpAddress.String);
			cfg.nameserver[cfg.nameserver_count] = pj_str(dns);
			pIPAddr = pIPAddr ->Next;
			cfg.nameserver_count++;
			if (cfg.nameserver_count == 4) {
				break;	/* pjsua allows configuration of 4 name servers */
			}
		}
		GlobalFree( FixedInfo );
		FreeLibrary((HINSTANCE__*) hLib);
		break;
	}
#else
	FILE *f = fopen("/etc/resolv.conf", "r");
	char line[128], server[64];
	int err = 0;

	if (f) {
		while (1) {
			err = fscanf(f, "%126[^\n]\n", line);
			if (err == EOF) {
				break;
			}
			if (err !=1) {
				break;
			}

			if (1 == sscanf(line, "nameserver %62s\n", &(server[0]))) {
				cfg.nameserver[cfg.nameserver_count] = pj_str(&(server[0]));
				cfg.nameserver_count++;
				if (cfg.nameserver_count == 4) {
					break;	/* pjsua allows configuration of 4 name servers */
				}
			}
			break;
		}

		(void)fclose(f);
	}
#endif //WIN32
	appLog("cfg.nameserver_count=" + QString::number(cfg.nameserver_count));
	for (count=0;count<cfg.nameserver_count;count++) {
		appLog("cfg.nameserver[" + QString::number(count) + "]=" 
				+ QString::fromAscii(cfg.nameserver[count].ptr,cfg.nameserver[count].slen));
	}

	/* initialize pjsua callbacks */
	cfg.cb.on_incoming_call = PjCallback::on_incoming_call_wrapper;
	cfg.cb.on_call_media_state = PjCallback::on_call_media_state_wrapper;
	cfg.cb.on_call_state = PjCallback::on_call_state_wrapper;
	cfg.cb.on_pager = PjCallback::on_pager_wrapper;
	cfg.cb.on_reg_state = PjCallback::on_reg_state_wrapper;
	cfg.cb.on_buddy_state = PjCallback::on_buddy_state_wrapper;
	cfg.cb.on_nat_detect = PjCallback::on_nat_detect_wrapper;
	//cfg.cb.on_pager2 = PjCallback::on_pager2_wrapper;;

	/* configure STUN server (activates NAT traversal)*/
	if (!stun.isEmpty()) {
		tempStun = stun.toLatin1();
		cfg.stun_srv_cnt=1;
		cfg.stun_srv[0] = pj_str(tempStun.data());
		ui.natTypeEdit->setText("NAT detection in progress ...");
		ui.natTypeEdit->setToolTip("");
	} else {
		ui.natTypeEdit->setText("n/a (no STUN server configured)");
		ui.natTypeEdit->setToolTip("Please configure a STUN server to \r\nenable NAT detection");
	}
	pj_ansi_snprintf(tmp, sizeof(tmp), USER_AGENT " " USER_AGENT_VERSION " (pjproject %s/%s)", pj_get_version(), PJ_OS_NAME);
	pj_strdup2_with_null(pool, &(cfg.user_agent), tmp);
	
	pjsua_logging_config_default(&log_cfg);
	log_cfg.msg_logging = true;
	log_cfg.console_level = logLevel.toUInt();
	log_cfg.cb = PjCallback::logger_cb_wrapper;
	log_cfg.decor = log_cfg.decor & ~PJ_LOG_HAS_NEWLINE;
			
	pjsua_media_config_default(&media_cfg);
	media_cfg.no_vad = true;
	status = pjsua_init(&cfg, &log_cfg, &media_cfg);
	if (status != PJ_SUCCESS) {
		error("Error in pjsua_init()", status);
		pjsua_destroy();
		return -1;
	}

	appLog("pjsua_init succeeded ...");

	/* Initialize our module to handle otherwise unhandled request */
	status = pjsip_endpt_register_module(pjsua_get_pjsip_endpt(),
					 &mod_default_handler);
    
	appLog("pjsip_endpt_register_module succeeded ...");

	/* add transport to pjsua */
	pjsua_transport_id transport_id;
	pjsua_transport_config_default(&transport_cfg);
	transport_cfg.port = sipPort;
	if (transport == "TCP") {
		appLog("adding TCP transport ...");
		status = pjsua_transport_create(PJSIP_TRANSPORT_TCP, &transport_cfg, &transport_id);
#if defined(PJ_HAS_IPV6) && PJ_HAS_IPV6
	} else if (transport == "UDP6") {
		appLog("adding UDP6 transport ...");
		status = pjsua_transport_create(PJSIP_TRANSPORT_UDP6, &transport_cfg, &transport_id);
	} else if (transport == "TCP6") {
		appLog("adding TCP6 transport ...");
		status = pjsua_transport_create(PJSIP_TRANSPORT_TCP6, &transport_cfg, &transport_id);
#endif
	} else if (transport == "TLS") {
		transport_cfg.tls_setting.method = PJSIP_TLSV1_METHOD;
		transport_cfg.tls_setting.verify_server = tlsVerifyServer;
//		transport_cfg.tls_setting.ca_list_file = pj_str("c:\\cacert.pem");
		QByteArray tempca, tempkey, tempcert;
		tempca=caFile.toLatin1();
		transport_cfg.tls_setting.ca_list_file = pj_str(tempca.data());
		tempkey=privKeyFile.toLatin1();
		transport_cfg.tls_setting.privkey_file = pj_str(tempkey.data());
		tempcert=certFile.toLatin1();
		transport_cfg.tls_setting.cert_file = pj_str(tempcert.data());
		appLog("adding TLS transport ...");
		status = pjsua_transport_create(PJSIP_TRANSPORT_TLS, &transport_cfg, &transport_id);
	} else {
		appLog("adding UDP transport ...");
		status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &transport_cfg, &transport_id);
	}
	if (status != PJ_SUCCESS) {
		error("Error creating transport", status);
		pjsua_destroy();
		return -1;
	}

	appLog("pjsip_endpt_register_module succeeded ...");
	
	/* register to SIP proxy */
	pjsua_acc_config_default(&acc_cfg);
	QString sipAoR, regUri, outboundUri;
	sipAoR = QString("sip:") + username + QString("@") + domain;
	regUri = QString("sip:") + domain;
	//regUri = QString("sip:") + domain + ";transport=tcp";
	if (!outbound.isEmpty()) {
		outboundUri = QString("sip:") + outbound;
	} else {
		/* if no explicit OBP use the domain as OBP (to send requests always via home proxy) */
		outboundUri = QString("sip:") + domain + ";lr";
		if ( (transport == "UDP") || (transport == "UDP6") )  {
			outboundUri = outboundUri + ";transport=udp";
		} else if ( (transport == "TCP") || (transport == "TCP6") )  {
			outboundUri = outboundUri + ";transport=tcp";
		} else if ( (transport == "TLS") )  {
			outboundUri = outboundUri + ";transport=tls";
		}
	}

	appLog("outboundUri=" + outboundUri);

	QByteArray temp;
	free(caor);      temp=sipAoR.toLatin1();        caor = strdup(temp.data());
	free(creguri);   temp = regUri.toLatin1();      creguri = strdup(temp.data());
	free(cdomain);   temp = domain.toLatin1();      cdomain = strdup(temp.data());
	free(cusername); temp = username.toLatin1();    cusername = strdup(temp.data());
	free(cpassword); temp = password.toLatin1();    cpassword = strdup(temp.data());
	free(coutbound); temp = outboundUri.toLatin1(); coutbound = strdup(temp.data());

	acc_cfg.id = pj_str(caor);
	if (!noregistration) {
		acc_cfg.reg_uri = pj_str(creguri);
		appLog("registration_enabled=TRUE");
	} else {
		appLog("registration_enabled=FALSE");
	}
	acc_cfg.cred_count = 1;
	acc_cfg.cred_info[0].realm = pj_str(cdomain);
	acc_cfg.cred_info[0].scheme = pj_str("digest");
	acc_cfg.cred_info[0].username = pj_str(cusername);
	acc_cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
	acc_cfg.cred_info[0].data = pj_str(cpassword);
	/* do not specify the transport id as this will cause pjsip to open
	 * a new TCP connection for every request. IMO this is a bug. See also
	 * http://lists.pjsip.org/pipermail/pjsip_lists.pjsip.org/2009-July/008183.html
	 */
	//acc_cfg.transport_id = transport_id;
	acc_cfg.proxy_cnt = 1;
	acc_cfg.proxy[0] = pj_str(coutbound);
	if (publish) {
		acc_cfg.publish_enabled = PJ_TRUE;
		appLog("publish_enabled=TRUE");
	} else {
		acc_cfg.publish_enabled = PJ_FALSE;
		appLog("publish_enabled=FALSE");
	}
	/* SRTP account settings */
	if (srtp == "disabled") {
		acc_cfg.use_srtp = PJMEDIA_SRTP_DISABLED;
	} else if (srtp == "optional") {
		acc_cfg.use_srtp = PJMEDIA_SRTP_OPTIONAL;
	} else if (srtp == "mandatory") {
		acc_cfg.use_srtp = PJMEDIA_SRTP_MANDATORY;
	}
	if (srtpReq == "none") {
		acc_cfg.srtp_secure_signaling = 0;
	} else if (srtpReq == "TLS") {
		acc_cfg.srtp_secure_signaling = 1;
	} else if (srtpReq == "SIPS") {
		acc_cfg.srtp_secure_signaling = 2;
	}

	if (stun.isEmpty()) {
		// STUN server not configured, disable internal NAT traversal
		acc_cfg.allow_contact_rewrite = 0;
	} else {
		// STUN server configured, enable internal NAT traversal
		acc_cfg.allow_contact_rewrite = 1;
	}

	status = pjsua_acc_add(&acc_cfg, PJ_TRUE, &acc_id);
	if (status != PJ_SUCCESS) {
		QMessageBox::StandardButton ret;
		ret	= QMessageBox::warning( this, tr("QjSimple"),
					tr("Failure adding the account! Inspect the debug window!"),
					QMessageBox::Ok);
		shutdownPjSip();
		return -1;
	}
	
	appLog("pjsua_acc_add succeeded ...");

	/* tell the applicaton that the SIP stack is ready now */
	sipOn = true;
		
	appLog("finished intializing SIP stack, start pjsua ...");

	/* finished intializing SIP stack */
	/* start pjsua */
	status = pjsua_start();
	if (status != PJ_SUCCESS) {
		error("Error starting pjsua", status);
		pjsua_destroy();
		return -1;
	}

	//uh, ah, QjSimple crashes if we have too many log messages - buffer overflow in signal/slot????
	//very very strange!!!
	//appLog("initializing SIP stack ... done");

	if (noregistration) {
		appLog("registration disabled, simulating callback");
		reg_state_slot(acc_id);
	}

	return 0;
}

int QjSimple::shutdownPjSip() {
	/* shut down */
	pj_status_t status;
	
	sipOn = false;
//	ui.statusBox->setCheckState(Qt::Unchecked);
	
	appLog("shuting down SIP stack ...");
    if (pool) {
    	pj_pool_release(pool);
    	pool = NULL;
    }

	status = pjsua_destroy();
	if (status != PJ_SUCCESS) {
		error("Error destroying pjsua", status);
	}
	appLog("shuting down SIP stack ... done");
	/* initialize */
	return 0;
}

int QjSimple::reInitializePjSip() {
	/* shut down */
	shutdownPjSip();
	/* initialize */
	return initializePjSip();
}

QListWidgetItem* QjSimple::getBuddyItem(QString uri) {
	QListWidgetItem *item;
	for (int i=0; i<ui.buddyList->count(); i++) {
		item = ui.buddyList->item(i);
		if (uri == item->data(Qt::UserRole).toString()) {
			return item;
		}
	}
	return 0;
}

Buddy* QjSimple::getBuddy(QString uri) {
	Buddy *buddy;
	for (int i=0; i<buddies.size(); i++) {
		buddy = buddies.at(i);
		if (uri == buddy->uri) {
			return buddy;
		}
	}
	return 0;
}

Buddy* QjSimple::addNewBuddy(QString name, QString uri, bool presence) {
	if ((!name.isEmpty()) && (!uri.isEmpty())) {
		Buddy *buddy = new Buddy();
		buddy->name = name.trimmed();
		uri = uri.trimmed();
		if (!uri.startsWith("sip:",Qt::CaseInsensitive)) {
			uri = "sip:" + uri;
		}
		buddy->uri = uri;
		buddy->presence = presence;
		QListWidgetItem *item = new QListWidgetItem(buddy->name, ui.buddyList);
		item->setData(Qt::UserRole, buddy->uri);
		if (presence) {
			item->setIcon(QIcon(":/icons/unknown"));
		}
		item->setToolTip(buddy->uri);
		buddies << buddy;
		return buddy;
	}
	return 0;
}

void QjSimple::deleteBuddy(Buddy * buddy) {
	QListWidgetItem *item;
	QList<Buddy*>::iterator i;
	if (!buddy) return;
	for (i = buddies.begin(); i != buddies.end(); i++) {
		if ((*i) == buddy) {
			buddies.erase(i);

			for (int i=0; i<ui.buddyList->count(); i++) {
				item = ui.buddyList->item(i);
				if (buddy->uri == item->data(Qt::UserRole).toString()) {
					/* test if deleting automatically removes it from the list */
					/* if yes, 			item=getBuddyItem(buddy->uri); can be used too */
					delete item;
				}
			}
						
			delete buddy;
			break;
		}
	 }
}

void QjSimple::subscribeBuddy(Buddy *buddy) {
	pj_status_t status;
	if (!buddy || !sipOn) return;
	pjsua_buddy_config_default(&buddy_cfg);
	buddy_cfg.subscribe = PJ_TRUE;
	QByteArray temp=buddy->uri.toLatin1();
	buddy_cfg.uri = pj_str(temp.data());
	status = pjsua_buddy_add(&buddy_cfg, &(buddy->buddy_id));
	appLog("subscribeBuddy: Buddy id for " + buddy->name + "(" + buddy->uri + ")=" + QString::number(buddy->buddy_id));
	if (status != PJ_SUCCESS) {
		error("Error adding buddy", status);
	}
	buddy->buddy_id_valid = TRUE;
}

void QjSimple::unsubscribeBuddy(Buddy *buddy) {
	pj_status_t status;
	if (!buddy || !sipOn) return;
	appLog("unsubscribeBuddy: Buddy id for " + buddy->name + "(" + buddy->uri + ")=" + QString::number(buddy->buddy_id));
	status = pjsua_buddy_del(buddy->buddy_id);
	if (status != PJ_SUCCESS) {
		error("Error deleting buddy", status);
	}
}

void QjSimple::buddy_state(pjsua_buddy_id buddy_id) {
	pj_status_t status;
	pjsua_buddy_info info;

	appLog("slot buddy_state called for buddy_id " + QString::number(buddy_id));
	
	if (pjsua_buddy_is_valid(buddy_id)) {
		status = pjsua_buddy_get_info(buddy_id, &info);
		if (status != PJ_SUCCESS) {
			error("Error getting buddy info", status);
		}
		/* find buddy object */
		Buddy *buddy;
		QString uri = QString::fromAscii(info.uri.ptr,info.uri.slen);
		QString status_text = QString::fromAscii(info.status_text.ptr,info.status_text.slen);
		QString rpid_status_text = QString::fromAscii(info.rpid.note.ptr,info.rpid.note.slen);
		buddy = getBuddy(uri);
		if (!buddy) {
			appLog("buddy URI " + uri + " not found!");
			return;
		}
		/* check if buddy id matches, otherwise this could be a race condition between deleted and added buddy */
		if (buddy_id != buddy->buddy_id) {
			appLog("callback buddy_id "+QString::number(buddy_id)+" does not match our buddy_id "+QString::number(buddy->buddy_id)+" !!!");
			return;
		}
		buddy->status = info.status;
		appLog(uri + " status is " + status_text);
		appLog(uri + " RPID status is " + rpid_status_text);
		
		/* find buddy item and update presence state */
		QListWidgetItem *item;
		item=getBuddyItem(uri);
		if (!item) {
			appLog("item for buddy " + uri + " not found");
			return;
		}
		switch (buddy->status){
		case PJSUA_BUDDY_STATUS_UNKNOWN: 
			item->setIcon(QIcon(":/icons/unknown"));
			break;
		case PJSUA_BUDDY_STATUS_ONLINE: 
			item->setIcon(QIcon(":/icons/online"));
			break;
		case PJSUA_BUDDY_STATUS_OFFLINE: 
			item->setIcon(QIcon(":/icons/offline"));
			break;
		default:
			appLog("unknown presence state!");
		}
	} else {
		appLog("Warning: invalid buddy_id");
	}
}

void QjSimple::reg_state_slot(int acc_id_cb) {
	pj_status_t status;
	pjsua_acc_info info;
	appLog("Info: on_reg_state_slot called");

	if (acc_id != acc_id_cb) {
		appLog("Warning: acc_id != acc_id from callback");
		return;
	}

	if (pjsua_acc_is_valid(acc_id_cb)) {
		status = pjsua_acc_get_info(acc_id, &info);
		if (status != PJ_SUCCESS) {
			error("Error getting account info", status);
			return;
		}
		appLog("Info: on_reg_state_slot: status=" + QString::number(info.status));
		appLog("Info: on_reg_state_slot: statustext=" + QString::fromAscii(info.status_text.ptr,info.status_text.slen));
		if (info.status == 200) {
			ui.statusBox->setCheckState(Qt::Checked);
		} else {
			ui.statusBox->setCheckState(Qt::Unchecked);
		}
		if (info.status == 200 || noregistration) {
			if (subscribe && !subscribe_done) {
				/* set local presence state - triggers publish */
				local_status_changed(ui.comboBox->currentText());
				/* register buddies in pjsua */
				QList<Buddy*>::iterator i;
				for (i = buddies.begin(); i != buddies.end(); i++) {
					if ((*i)->presence) {
						subscribeBuddy(*i);
					}
				}
			}
		} else {
			ui.statusBox->setCheckState(Qt::Unchecked);
		}
	} else {
		appLog("Warning: invalid acc_id");
		ui.statusBox->setCheckState(Qt::Unchecked);
	}
}

void QjSimple::local_status_changed(QString text) {
	pj_status_t status;

	if (publish && sipOn) {
		/* set local presence state - triggers publish */
		
		if (text=="Online") {
			status = pjsua_acc_set_online_status(acc_id, PJ_TRUE);
		//} else if (text=="Offline") {
		} else {
			status = pjsua_acc_set_online_status(acc_id, PJ_FALSE);
		}
		//else create our own pdif element 

		if (status != PJ_SUCCESS) {
			error("Error setting online status", status);
		}
	}
}

void QjSimple::setCallState(QString text) {
		ui.callStateEdit->setText(text);
}

void QjSimple::setCallButtonText(QString text) {
		ui.callButton->setText(text);
		if (text == "call buddy") { // set by callback on DISCONNECTED
			ui.holdButton->setText("hold");
			onHold = false;
		}
}

//void QjSimple::test(QString name) {
//	name="bbbbbbb";
//}



void QjSimple::on_xcapGetBuddyButton_clicked() {
	PJ_LOG(3,(THIS_FILE, "on_xcapGetBuddyButton_clicked() ...."));
	//QHttp("www.orf.at", QHttp::ConnectionModeHttps);
//	appLog("currentID=" + QString::number(xcapGet->currentId()));
//	xcapGet->setHost("www.orf.at", QHttp::ConnectionModeHttp, 0);
//	appLog("currentID=" + QString::number(xcapGet->currentId()));
//	xcapGet->clearPendingRequests();
//	appLog("after clear: currentID=" + QString::number(xcapGet->currentId()));
//	xcapGetId = xcapGet->get("/");
//	appLog("xcapGetID=" + QString::number(xcapGetId));
//	appLog("currentID=" + QString::number(xcapGet->currentId()));
	
//	QUrl url(xcapUrl);
//    QHttp::ConnectionMode mode = url.scheme().toLower() == "https" ? QHttp::ConnectionModeHttps : QHttp::ConnectionModeHttp;
//    xcapGet->setHost(url.host(), mode, url.port() == -1 ? 0 : url.port());
//    xcapGet->clearPendingRequests();
//    xcapGetId = xcapGet->get(url.path());
}
void QjSimple::xcapGetRequestFinished(int requestId, bool error) {
	PJ_LOG(3,(THIS_FILE, "xcapGetRequestFinished() ...."));
	if (requestId != xcapGetId) {
//		appLog("xcapGetRequestFinished: wrong request id "  + QString::number(requestId));
		PJ_LOG(3,(THIS_FILE, "xcapGetRequestFinished: wrong request id %d... ignoring",requestId));
    	return;
	}
//	appLog("xcapGetRequestFinished: correct request id "  + QString::number(requestId));
	PJ_LOG(3,(THIS_FILE, "xcapGetRequestFinished: correct request id %d...",requestId));
	if (error) {
		PJ_LOG(3,(THIS_FILE, "xcapGetRequestFinished: error GETing request"));
		QMessageBox::information(this, tr("HTTP"),
				tr("xcapGet failed: %1.").arg(xcapGet->errorString()));
	} else {
		PJ_LOG(3,(THIS_FILE, "xcapGetRequestFinished: GET request succeeded"));
		QByteArray response=xcapGet->readAll();
		QString text(response.data());
		appLog(text);
	}
}
void QjSimple::xcapGetReadResponseHeader(const QHttpResponseHeader &responseHeader) {
	
}
void QjSimple::xcapGetUpdateDataReadProgress(int bytesRead, int totalBytes) {
	
}
void QjSimple::xcapGetSlotAuthenticationRequired(const QString &hostName, QAuthenticator *authenticator) {
//	PJ_LOG(3,(THIS_FILE, "xcapGetSlotAuthenticationRequired() ...."));
//	authenticator->setUser(username);
//    authenticator->setPassword(password);	
}
void QjSimple::on_xcapPutBuddyButton_clicked() {
}

void QjSimple::on_holdButton_clicked() {
	PJ_LOG(3,(THIS_FILE, "on_holdButton_clicked() ...."));
	if (onHold) {
		PJ_LOG(3,(THIS_FILE, "on_callButton_clicked() .... putting call off hold"));
		pj_status_t status = pjsua_call_reinvite(activeCalls.at(0), true, 0);
		if (status != PJ_SUCCESS) {
			PJ_LOG(3,(THIS_FILE, "on_callButton_clicked() .... ERROR putting call off hold"));
		} else {
			ui.holdButton->setText("hold");
			onHold = false;
		}
	} else {
		PJ_LOG(3,(THIS_FILE, "on_callButton_clicked() .... putting call on hold"));
		pj_status_t status = pjsua_call_set_hold(activeCalls.at(0), 0);
		if (status != PJ_SUCCESS) {
			PJ_LOG(3,(THIS_FILE, "on_callButton_clicked() .... ERROR putting call on hold"));
		} else {
			ui.holdButton->setText("unhold");
			onHold = true;
		}
	}
	PJ_LOG(3,(THIS_FILE, "leaving on_holdButton_clicked ..."));
}
