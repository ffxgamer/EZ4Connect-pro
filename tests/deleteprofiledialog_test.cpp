#include <QApplication>
#include <QLineEdit>
#include <QPushButton>

#include "presentation/dialogs/deleteprofiledialog/deleteprofiledialog.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    DeleteProfileDialog dialog(QStringLiteral("常山 VPN"));
    auto *input = dialog.findChild<QLineEdit *>(QStringLiteral("profileNameConfirmation"));
    auto *confirm = dialog.findChild<QPushButton *>(QStringLiteral("confirmDeleteButton"));
    if (!input || !confirm || confirm->isEnabled()) return 1;

    for (const QString &value : {QStringLiteral("常山"), QStringLiteral("常山 VPN "), QStringLiteral("常山 vpn")})
    {
        input->setText(value);
        if (confirm->isEnabled()) return 1;
        dialog.accept();
        if (dialog.result() == QDialog::Accepted) return 1;
    }
    input->setText(QStringLiteral("常山 VPN"));
    if (!confirm->isEnabled()) return 1;
    dialog.accept();
    return dialog.result() == QDialog::Accepted ? 0 : 1;
}
