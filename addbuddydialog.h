#ifndef ADDBUDDYDIALOG_H
#define ADDBUDDYDIALOG_H

#include <QtGui/QDialog>
#include "ui_addbuddydialog.h"

class AddBuddyDialog : public QDialog
{
    Q_OBJECT

public:
    AddBuddyDialog(QWidget *parent = 0);
    ~AddBuddyDialog();


    QString getName();
    QString getUri();
    bool getPresence();
    void setName(QString);
    void setUri(QString);
    void setPresence(bool);

    
private:
    Ui::AddBuddyDialogClass ui;
};

#endif // ADDBUDDYDIALOG_H
