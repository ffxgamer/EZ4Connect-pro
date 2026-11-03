#ifndef CONNECTIONUICONTROLLER_H
#define CONNECTIONUICONTROLLER_H

#include <QObject>
#include <QSettings>
#include <optional>
#include <QSystemTrayIcon>

#include <functional>
#include <memory>

#include "core/connectionerror.h"

class QAction;
class ApplicationLogger;
class AuthDialogCoordinator;
class ConnectionSession;
class QPushButton;
class SystemProxySession;
class QWidget;

class ConnectionUiController : public QObject
{
    Q_OBJECT

public:
    using SettingsProvider = std::function<QSettings *()>;
    using ProfileIdProvider = std::function<QString()>;
    using NotificationHandler = std::function<void(
        const QString &title,
        const QString &content,
        QSystemTrayIcon::MessageIcon icon
    )>;

    ConnectionUiController(
        QWidget *parentWidget,
        QPushButton *connectButton,
        QPushButton *proxyButton,
        QAction *trayConnectAction,
        ConnectionSession *connectionSession,
        SystemProxySession *systemProxySession,
        AuthDialogCoordinator *authenticationDialogs,
        ApplicationLogger *applicationLogger,
        SettingsProvider settingsProvider,
        ProfileIdProvider profileIdProvider,
        NotificationHandler notificationHandler,
        QObject *parent = nullptr
    );

    void profileSelectionChanged();
    bool selectedProfileIsConnected() const;
    QString connectedProfileName() const;
    QString connectedProfileId() const;
    QString connectedConfigPath() const;
    void restoreProxyOnNextConnection(bool enabled);

private:
    void handleConnectClicked();
    void handleProxyClicked();
    void startConnection(
        const QString &username,
        const QString &password,
        const QString &phone = QString()
    );
    void showConnectionError(ZJU_ERROR error);
    QSettings *settings() const;
    QSettings *connectionSettings() const;
    void finishProfileSwitch(const QString &requestedPath);

    QWidget *parentWidget;
    QPushButton *connectButton;
    QPushButton *proxyButton;
    QAction *trayConnectAction;
    ConnectionSession *connectionSession;
    SystemProxySession *systemProxySession;
    AuthDialogCoordinator *authenticationDialogs;
    SettingsProvider settingsProvider;
    ProfileIdProvider profileIdProvider;
    NotificationHandler notificationHandler;
    std::unique_ptr<QSettings> activeSettings;
    QString activeProfileId;
    std::optional<bool> restoreProxyAfterUpdate;
    QString restoreProxyConfigPath;
    quint64 connectionGeneration = 0;
    bool switchingConnection = false;
    bool proxyClearAttempted = false;
};

#endif // CONNECTIONUICONTROLLER_H
