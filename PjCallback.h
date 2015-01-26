#ifndef PJCALLBACK_H_
#define PJCALLBACK_H_

#include <QObject>

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

class PjCallback : public QObject {
Q_OBJECT

public:
	PjCallback();
	virtual ~PjCallback();

	/* callback logger function which emmits the signal */
	void logger_cb(int level, const char *data, int len);
	/* callback logger function called by pjsip */
	static void logger_cb_wrapper(int level, const char *data, int len);

	/** callback function, called by wrapper */
	void on_pager(pjsua_call_id call_id, const pj_str_t *from,
			const pj_str_t *to, const pj_str_t *contact,
			const pj_str_t *mime_type, const pj_str_t *text);
	/** callback wrapper function called by pjsip
	 * Incoming IM message (i.e. MESSAGE request)!*/
	static void on_pager_wrapper(pjsua_call_id call_id, const pj_str_t *from,
			const pj_str_t *to, const pj_str_t *contact,
			const pj_str_t *mime_type, const pj_str_t *text);

	/* Notify application when invite state has changed. Application may 
	 * then query the call info to get the detail call states by calling 
	 * pjsua_call_get_info() function.callback wrapper function called by pjsip
	 */
	void on_call_state(pjsua_call_id call_id, pjsip_event *e);
	static void on_call_state_wrapper(pjsua_call_id call_id, pjsip_event *e);

	/* Notify application on incoming call.
	 */
	void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata);
	static void on_incoming_call_wrapper(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata);

	/* Notify application which NAT type was detected
	 */
	void on_nat_detect(const pj_stun_nat_detect_result *res);
	static void on_nat_detect_wrapper(const pj_stun_nat_detect_result *res);
	
	/* Notify application when media state in the call has changed. Normal 
	 * application would need to implement this callback, e.g. to connect 
	 * the call's media to sound device. When ICE is used, this callback 
	 * will also be called to report ICE negotiation failure.
	 */
	void on_call_media_state(pjsua_call_id call_id);
	static void on_call_media_state_wrapper(pjsua_call_id call_id);

	/** callback function, called by wrapper */
	void on_buddy_state(pjsua_buddy_id buddy_id);
	/** callback wrapper function called by pjsip
	 * Presence state of buddy was changed */
	static void on_buddy_state_wrapper(pjsua_buddy_id buddy_id);

	/** callback function, called by wrapper */
	void on_reg_state(pjsua_acc_id acc_id);
	/** callback wrapper function called by pjsip
	 * Registration state of account changed */
	static void on_reg_state_wrapper(pjsua_acc_id acc_id);


	//	/** callback function, called by wrapper */
//	void on_pager2(pjsua_call_id call_id, const pj_str_t *from,
//			const pj_str_t *to, const pj_str_t *contact,
//			const pj_str_t *mime_type, const pj_str_t *text,
//			pjsip_rx_data *rdata);
//	/** callback wrapper function called by pjsip
//	 * Incoming IM message (i.e. MESSAGE request)!*/
//	static void on_pager2_wrapper(pjsua_call_id call_id, const pj_str_t *from,
//			const pj_str_t *to, const pj_str_t *contact,
//			const pj_str_t *mime_type, const pj_str_t *text,
//			pjsip_rx_data *rdata);

signals:
	/* this signal forwards the log message a-synchronous to the GUI thread */
	void new_log_message(QString text);
	/* this signal forwards the instant message a-synchronous to the GUI thread */
	void new_im(QString from, QString text);
	/* this signal forwards the instant message a-synchronous to the GUI thread */
	void nat_detect(QString text, QString description);
	/* this signal forwards the call state synchronous to the GUI thread */
	void call_state_sync(int call_id);
	/* this signal forwards the call state a-synchronous BLOCKING to the GUI thread */
	void setCallState(QString);
	/* this signal forwards the text of the call button a-synchronous BLOCKING to the GUI thread */
	void setCallButtonText(QString);
//	/* this signal forwards incoming call event a-synchronous to the GUI thread */
//	void incoming_call();
	/* this signal forwards the buddy_id of the buddy whose status changed to the GUI thread */
	void buddy_state(int buddy_id);
	/* this signal forwards the acc_id of the SIP account whose registration status changed */
	void reg_state_signal(int acc_id);
};

#endif /*PJCALLBACK_H_*/
