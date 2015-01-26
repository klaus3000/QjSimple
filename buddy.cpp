#include "buddy.h"

Buddy::Buddy(QObject *parent) : QObject(parent)
{
	presence = false;
	status = PJSUA_BUDDY_STATUS_UNKNOWN;
	buddy_id_valid = false;
	buddy_id = -1;
}

Buddy::~Buddy()
{
}
