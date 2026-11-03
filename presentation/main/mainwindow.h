#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QSettings>
#include <QPointer>
#include <QElapsedTimer>

#include "application/connectionsession.h"
#include "application/profileservice.h"
#include "application/systemproxysession.h"
#include "presentation/dialogs/settingwindow/settingwindow.h"
#include "presentation/coordinators/authdialogcoordinator.h"
#include "presentation/main/mainwindowcoordinator.h"

namespace Ui
{
    class MainWindow;
}

class ApplicationLogger;
class ApplicationLogFile;
class ConnectionUiController;
class UpdateChecker;
class MacOSStatusItem;

class MainWindow : public QMainWindow
{
Q_OBJECT

public:
    explicit MainWindow(
        ApplicationLogger *logger,
        ApplicationLogFile *logFile,
        QWidget *parent = nullptr
    );

    ~MainWindow() override;

public slots:

    void cleanUpWhenQuit();

protected:
    void closeEvent(QCloseEvent *e) override;

    void changeEvent(QEvent *event) override;

private:
    void upgradeSettings();

    void clearLog();

    void resetZjuConnectUi();

    void applyColorScheme(Qt::ColorScheme scheme);

    void updateConnectionState(ConnectionState state);

    void updateProfileSummary();

    void showNotification(
        const QString &title,
        const QString &content,
        QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::MessageIcon::Information
    );

    void updateVersionInfo();

    void openUpdateDialog();

    void setupTrayIcon();

    void showMainWindow();

#ifdef Q_OS_MACOS
    void updateTrafficTotals(quint64 received, quint64 sent);
    void resetTrafficStatus(bool connected);
#endif

    void setupProfileMenu();

    void refreshProfileMenu();

    void refreshProfileList();

    void requestProfileSelection(
        const QString &profileId, bool edit = false, bool overrideTarget = false
    );

    bool switchProfile(const QString &profileId);

    void createProfile();

    void openConfigurationGuide();

    void promptConfigurationGuide(
        const QString &windowTitle,
        const QString &text
    );

    void promptFirstLaunchGuide();

    void renameCurrentProfile();

    void deleteCurrentProfile();

    void gracefullyQuit();

    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;
#ifdef Q_OS_MACOS
    MacOSStatusItem *macStatusItem = nullptr;
    bool trafficConnected = false;
    quint64 lastReceived = 0;
    quint64 lastSent = 0;
    QElapsedTimer trafficSampleTimer;
    QElapsedTimer trafficLastSeen;
#endif
    QMenu *trayMenu;
    QMenu *trayProfileMenu;
    QAction *trayConnectAction;
    QAction *trayShowAction;
    QAction *trayCloseAction;
    QAction *newProfileAction;
    QAction *renameProfileAction;
    QAction *deleteProfileAction;
    ConnectionSession *connectionSession = nullptr;
    SystemProxySession *systemProxySession = nullptr;
    AuthDialogCoordinator *authenticationDialogs = nullptr;
    ConnectionUiController *connectionUiController = nullptr;
    MainWindowCoordinator *coordinator;
    ApplicationLogger *applicationLogger;
    ApplicationLogFile *applicationLogFile;
    UpdateChecker *updateChecker;
    QSettings *settings;
    ProfileService *profileService;
    QString currentProfileId;
    QPointer<SettingWindow> settingWindow;

};

#endif //MAINWINDOW_H
