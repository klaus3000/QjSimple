#ifndef BUDDY_H_
#define BUDDY_H_

#include <QObject>
#include <QString>
extern "C" {
	#include <pjsua-lib/pjsua.h>
}

class Buddy : public QObject {
	Q_OBJECT
public:
	Buddy(QObject *parent = 0);
	virtual ~Buddy();
	
	QString name;
	QString uri;
	bool presence;
	pjsua_buddy_status status;
	QString status_text;
	bool buddy_id_valid;
	pjsua_buddy_id buddy_id;
};

#endif /*BUDDY_H_*/
