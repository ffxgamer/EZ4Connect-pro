#include "deleteprofiledialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

DeleteProfileDialog::DeleteProfileDialog(const QString &profileName, QWidget *parent)
    : QDialog(parent), expectedName(profileName), nameInput(new QLineEdit(this))
{
    setWindowTitle(QStringLiteral("删除配置"));
    auto *layout = new QVBoxLayout(this);
    auto *message = new QLabel(
        QStringLiteral("配置“%1”及其保存的登录信息将被删除，此操作无法撤销。\n\n请输入完整配置名称以确认删除：").arg(profileName), this);
    message->setTextFormat(Qt::PlainText);
    message->setWordWrap(true);
    layout->addWidget(message);
    nameInput->setObjectName(QStringLiteral("profileNameConfirmation"));
    nameInput->setAccessibleName(QStringLiteral("输入配置名称以确认删除"));
    layout->addWidget(nameInput);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    auto *deleteButton = buttons->addButton(QStringLiteral("删除配置"), QDialogButtonBox::AcceptRole);
    deleteButton->setObjectName(QStringLiteral("confirmDeleteButton"));
    deleteButton->setEnabled(false);
    auto *cancelButton = buttons->button(QDialogButtonBox::Cancel);
    cancelButton->setText(QStringLiteral("取消"));
    cancelButton->setDefault(true);
    layout->addWidget(buttons);
    connect(nameInput, &QLineEdit::textChanged, this, [this, deleteButton](const QString &text)
    {
        deleteButton->setEnabled(!expectedName.isEmpty() && text == expectedName);
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &DeleteProfileDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    nameInput->setFocus();
}

void DeleteProfileDialog::accept()
{
    if (!expectedName.isEmpty() && nameInput->text() == expectedName)
        QDialog::accept();
}
