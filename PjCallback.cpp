#include <QList>
#include <QMutex>

#include "PjCallback.h"

#define THIS_FILE "QjCallback"

extern QList<int> activeCalls;
extern QMutex activeCallsMutex;

/* global callback/logger object */
PjCallback *globalPjCallback;

PjCallback::PjCallback() {
	globalPjCallback = this;
}

PjCallback::~PjCallback() {
}

void PjCallback::logger_cb(int level, const char *data, int len) {
	PJ_UNUSED_ARG(level);
	PJ_UNUSED_ARG(len);
	/* optional dump to stdout */
	//puts(data);
	/* emit signal with log message */
	/* paramter will be converted to QString which makes a deep copy */
emit 		new_log_message(data);
}

void PjCallback::logger_cb_wrapper(int level, const char *data, int len) {
	/* call the non-static member */
	if (globalPjCallback) {
		PjCallback *myCb = (PjCallback*) globalPjCallback;
		myCb->logger_cb(level, data, len);
	}
}

void PjCallback::on_pager(pjsua_call_id call_id, const pj_str_t *from,
		const pj_str_t *to, const pj_str_t *contact, const pj_str_t *mime_type,
		const pj_str_t *text) {
	/* Note: call index may be -1 */
	PJ_UNUSED_ARG(call_id);
	PJ_UNUSED_ARG(to);
	PJ_UNUSED_ARG(contact);
	PJ_UNUSED_ARG(mime_type);
	
	emit new_im(QString::fromAscii(from->ptr,from->slen), 
			QString::fromAscii(text->ptr,text->slen));
}

/** callback wrapper function called by pjsip
 * Incoming IM message (i.e. MESSAGE request)!*/
void PjCallback::on_pager_wrapper(pjsua_call_id call_id, const pj_str_t *from,
		const pj_str_t *to, const pj_str_t *contact, const pj_str_t *mime_type,
		const pj_str_t *text) {
	/* call the non-static member */
	if (globalPjCallback) {
		PjCallback *myCb = (PjCallback*) globalPjCallback;
		myCb->on_pager(call_id, from, to, contact, mime_type, text);
	}
}

/* Notify application which NAT type was detected
 */
void PjCallback::on_nat_detect(const pj_stun_nat_detect_result *res) {
	QString description;
	switch (res->nat_type) {
	case PJ_STUN_NAT_TYPE_UNKNOWN:
		description="PJ_STUN_NAT_TYPE_UNKNOWN:\r\n\r\n"
			"NAT type is unknown because the detection has not been performed.";
		break;
	case PJ_STUN_NAT_TYPE_ERR_UNKNOWN:
		description="PJ_STUN_NAT_TYPE_ERR_UNKNOWN:\r\n\r\n"
			"NAT type is unknown because there is failure in the detection process, \r\n"
			"possibly because server does not support RFC 3489.";
		break;
	case PJ_STUN_NAT_TYPE_OPEN:
		description="PJ_STUN_NAT_TYPE_OPEN:\r\n\r\n"
			"This specifies that the client has open access to Internet (or at \r\n"
			"least, its behind a firewall that behaves like a full-cone NAT, but \r\n"
			"without the translation)";
		break;
	case PJ_STUN_NAT_TYPE_BLOCKED:
		description="PJ_STUN_NAT_TYPE_BLOCKED:\r\n\r\n"
			"This specifies that communication with server has failed, probably \r\n"
			"because UDP packets are blocked.";
		break;
	case PJ_STUN_NAT_TYPE_SYMMETRIC_UDP:
		description="PJ_STUN_NAT_TYPE_SYMMETRIC_UDP:\r\n\r\n"
			"Firewall that allows UDP out, and responses have to come back to the \r\n"
			"source of the request (like a symmetric NAT, but no translation.";
		break;
	case PJ_STUN_NAT_TYPE_FULL_CONE:
		description="PJ_STUN_NAT_TYPE_FULL_CONE:\r\n\r\n"
			"A full cone NAT is one where all requests from the same internal IP \r\n"
			"address and port are mapped to the same external IP address and port. \r\n"
			"Furthermore, any external host can send a packet to the internal host, \r\n"
			"by sending a packet to the mapped external address.";
		break;
	case PJ_STUN_NAT_TYPE_SYMMETRIC:
		description="PJ_STUN_NAT_TYPE_SYMMETRIC:\r\n\r\n"
			"A symmetric NAT is one where all requests from the same internal IP \r\n"
			"address and port, to a specific destination IP address and port, are \r\n"
			"mapped to the same external IP address and port. If the same host \r\n"
			"sends a packet with the same source address and port, but to a different \r\n"
			"destination, a different mapping is used. Furthermore, only the external \r\n"
			"host that receives a packet can send a UDP packet back to the internal host.";
		break;
	case PJ_STUN_NAT_TYPE_RESTRICTED:
		description="PJ_STUN_NAT_TYPE_RESTRICTED:\r\n\r\n"
			"A restricted cone NAT is one where all requests from the same internal \r\n"
			"IP address and port are mapped to the same external IP address and port. \r\n"
			"Unlike a full cone NAT, an external host (with IP address X) can send a \r\n"
			"packet to the internal host only if the internal host had previously \r\n"
			"sent a packet to IP address X.";
		break;
	case PJ_STUN_NAT_TYPE_PORT_RESTRICTED:
		description="PJ_STUN_NAT_TYPE_PORT_RESTRICTED:\r\n\r\n"
			"A port restricted cone NAT is like a restricted cone NAT, but the \r\n"
			"restriction includes port numbers. Specifically, an external host \r\n"
			"can send a packet, with source IP address X and source port P, to \r\n"
			"the internal host only if the internal host had previously sent a \r\n"
			"packet to IP address X and port P.: ";
		break;
	default:
		description="Error: unknown type detected!";
	}
	emit nat_detect(QString(res->nat_type_name), description);
}

void PjCallback::on_nat_detect_wrapper(const pj_stun_nat_detect_result *res) {
	/* call the non-static member */
	if (globalPjCallback) {
		PjCallback *myCb = (PjCallback*) globalPjCallback;
		myCb->on_nat_detect(res);
	}
}

void PjCallback::on_call_state(pjsua_call_id call_id, pjsip_event *e) {
	PJ_UNUSED_ARG(e);

	PJ_LOG(3,(THIS_FILE, "trying to lock ...."));
	PJ_LOG(3,(THIS_FILE, "trying to lock .... locked"));
	if (activeCalls.empty()) {
		PJ_LOG(3,(THIS_FILE, "Call %d not found as callList is empty; new incoming call? ... ignoring", call_id));
		return;	
	}
	if (!activeCalls.contains(call_id)) {
		PJ_LOG(3,(THIS_FILE, "Call %d not found in callList; new incoming call? ... ignoring", call_id));
		return;	
	}

	pj_status_t status;
	pjsua_call_info ci;
	status = pjsua_call_get_info(call_id, &ci);
	if (status != PJ_SUCCESS) {
		PJ_LOG(3,(THIS_FILE, "ERROR retrieveing info for Call %d ... ignoring", call_id));
		return;
	}
	PJ_LOG(3,(THIS_FILE, "Call %d state=%.*s", call_id, 
			(int)ci.state_text.slen, ci.state_text.ptr));

	QString state_text = QString::fromAscii(ci.state_text.ptr,(int)ci.state_text.slen);
	emit setCallState(state_text);

	switch(ci.state) {
	case PJSIP_INV_STATE_DISCONNECTED:
		activeCalls.removeAt(activeCalls.indexOf(call_id));
		emit setCallButtonText("call buddy");
		break;
	default:
		;
	}
}

void PjCallback::on_call_state_wrapper(pjsua_call_id call_id, pjsip_event *e) {
	/* call the non-static member */
	if (globalPjCallback) {
		PjCallback *myCb = (PjCallback*) globalPjCallback;
		myCb->on_call_state(call_id, e);
	}	
}

void PjCallback::on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata) {

	PJ_UNUSED_ARG(acc_id);
	PJ_UNUSED_ARG(rdata);

	activeCallsMutex.lock();
	if (!activeCalls.empty()) {
		PJ_LOG(3,(THIS_FILE, "new incoming Call %d, but we already have a call ... reject", call_id));
		activeCallsMutex.unlock();
		pjsua_call_hangup(call_id, 486, NULL, NULL);
		return;	
	}
	
	pj_status_t status;
	pjsua_call_info ci;
	status = pjsua_call_get_info(call_id, &ci);
	if (status != PJ_SUCCESS) {
		PJ_LOG(3,(THIS_FILE, "ERROR retrieveing info for Call %d ... ignoring", call_id));
		activeCallsMutex.unlock();
		return;
	}
	PJ_LOG(3,(THIS_FILE, "Call %d state=%.*s", call_id, 
			(int)ci.state_text.slen, ci.state_text.ptr));

	QString state_text = QString::fromAscii(ci.state_text.ptr,(int)ci.state_text.slen);
	emit setCallState(state_text);

	emit setCallState(state_text);
	activeCalls << call_id;
	emit setCallButtonText("answer call");
	activeCallsMutex.unlock();
}

void PjCallback::on_incoming_call_wrapper(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata) {
	/* call the non-static member */
	if (globalPjCallback) {
		PjCallback *myCb = (PjCallback*) globalPjCallback;
		myCb->on_incoming_call(acc_id, call_id, rdata);
	}	
}

void PjCallback::on_call_media_state(pjsua_call_id call_id) {
	pjsua_call_info ci;

	pjsua_call_get_info(call_id, &ci);
	switch (ci.media_status) {
	case PJSUA_CALL_MEDIA_NONE: 
		PJ_LOG(3,(THIS_FILE, "on_call_media_state: call_id %d: "
				"PJSUA_CALL_MEDIA_NONE: Call currently has no media", call_id));
		break;
	case PJSUA_CALL_MEDIA_ACTIVE: 
		PJ_LOG(3,(THIS_FILE, "on_call_media_state: call_id %d: "
				"PJSUA_CALL_MEDIA_ACTIVE: The media is active", call_id));
		// When media is active, connect call to sound device.
		pjsua_conf_connect(ci.conf_slot, 0);
		pjsua_conf_connect(0, ci.conf_slot);
		break;
	case PJSUA_CALL_MEDIA_LOCAL_HOLD:
		PJ_LOG(3,(THIS_FILE, "on_call_media_state: call_id %d: "
				"PJSUA_CALL_MEDIA_LOCAL_HOLD: The media is currently put on hold by local endpoint", call_id));
		break;
	case PJSUA_CALL_MEDIA_REMOTE_HOLD:
		PJ_LOG(3,(THIS_FILE, "on_call_media_state: call_id %d: "
				"PJSUA_CALL_MEDIA_REMOTE_HOLD: The media is currently put on hold by remote endpoint", call_id));
		break;
	case PJSUA_CALL_MEDIA_ERROR:
		PJ_LOG(3,(THIS_FILE, "on_call_media_state: call_id %d: "
				"PJSUA_CALL_MEDIA_ERROR: The media has reported error (e.g. ICE negotiation)", call_id));
		break;
	}
}

void PjCallback::on_call_media_state_wrapper(pjsua_call_id call_id) {
	/* call the non-static member */
	if (globalPjCallback) {
		PjCallback *myCb = (PjCallback*) globalPjCallback;
		myCb->on_call_media_state(call_id);
	}	
}

void PjCallback::on_buddy_state(pjsua_buddy_id buddy_id) {
	emit new_log_message("on_buddy_state called for buddy_id " + QString::number(buddy_id));
	emit buddy_state(buddy_id);
}

void PjCallback::on_buddy_state_wrapper(pjsua_buddy_id buddy_id) {
	/* call the non-static member */
	if (globalPjCallback) {
		PjCallback *myCb = (PjCallback*) globalPjCallback;
		myCb->on_buddy_state(buddy_id);
	}
}

void PjCallback::on_reg_state(pjsua_acc_id acc_id) {
	emit reg_state_signal(acc_id);
}

void PjCallback::on_reg_state_wrapper(pjsua_acc_id acc_id) {
	/* call the non-static member */
	if (globalPjCallback) {
		PjCallback *myCb = (PjCallback*) globalPjCallback;
		myCb->on_reg_state(acc_id);
	}
}

//void PjCallback::on_pager2(pjsua_call_id call_id, const pj_str_t *from,
//		const pj_str_t *to, const pj_str_t *contact, const pj_str_t *mime_type,
//		const pj_str_t *text, pjsip_rx_data *rdata) {
//	/* Note: call index may be -1 */
//	PJ_UNUSED_ARG(call_id);
//	PJ_UNUSED_ARG(to);
//	PJ_UNUSED_ARG(contact);
//	PJ_UNUSED_ARG(mime_type);
//	PJ_UNUSED_ARG(rdata);
//	
//	char *cfrom, *ctext, *cfromuri;
//	cfrom = (char*) malloc((from->slen)+1);
//	if (!cfrom) {
//		puts("cfrom memory allocation error");
//		return;
//	}
//	memcpy(cfrom, from->ptr, from->slen);
//	memcpy(cfrom+(from->slen), "", 1);
//	ctext = (char*) malloc((text->slen)+1);
//	if (!ctext) {
//		puts("ctext memory allocation error");
//		return;
//	}
//	memcpy(ctext, text->ptr, text->slen);
//	memcpy(ctext+(text->slen), "", 1);
//
////rdata->msg_info.from->uri->vptr->
////	cfromuri = (char*) malloc((from->slen)+1);
////	if (!cfromuri) {
////		puts("cfrom memory allocation error");
////		return;
////	}
////	memcpy(cfromuri, from->ptr, from->slen);
////	memcpy(cfromuri+(from->slen), "", 1);
//
////	emit new_im(QString(cfrom), QString(cfromuri), QString(ctext));
//	emit new_im(QString(cfrom), QString(ctext));
//}
//
///** callback wrapper function called by pjsip
// * Incoming IM message (i.e. MESSAGE request)!*/
//void PjCallback::on_pager2_wrapper(pjsua_call_id call_id, const pj_str_t *from,
//		const pj_str_t *to, const pj_str_t *contact, const pj_str_t *mime_type,
//		const pj_str_t *text, pjsip_rx_data *rdata) {
//	/* call the non-static member */
//	if (globalPjCallback) {
//		PjCallback *myCb = (PjCallback*) globalPjCallback;
//		myCb->on_pager2(call_id, from, to, contact, mime_type, text, rdata);
//	}
//}
