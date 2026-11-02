#pragma once

#include <QDialog>
#include <QNetworkAccessManager>
#include <optional>
#include <functional>

#include "infrastructure/update/updatemanifest.h"

class QLabel;
class QLineEdit;
class QPushButton;

class UpdateDialog : public QDialog
{
public:
    explicit UpdateDialog(std::function<bool()> canInstall, QWidget *parent = nullptr);
    void checkForUpdates();

private:
    void installUpdate();
    QNetworkAccessManager network;
    QLineEdit *proxyInput;
    QLabel *statusLabel;
    QPushButton *checkButton;
    QPushButton *installButton;
    std::optional<UpdateAsset> asset;
    std::function<bool()> canInstall;
};
