#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;

class DeleteProfileDialog : public QDialog
{
public:
    explicit DeleteProfileDialog(const QString &profileName, QWidget *parent = nullptr);
    void accept() override;

private:
    const QString expectedName;
    QLineEdit *nameInput;
};
