#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QThread>
#include <QWidget>

#include <atomic>
#include <functional>
#include <memory>

#include "application/applicationlogger.h"
#include "application/connectionsession.h"
#include "application/systemproxysession.h"
#include "presentation/coordinators/authdialogcoordinator.h"
#include "presentation/coordinators/connectionuicontroller.h"
#include "presentation/dialogs/graphcaptchawindow/graphcaptchawindow.h"

namespace
{
class FakeCoreProcess : public CoreProcess
{
public:
    int startCalls = 0;
    int stopCalls = 0;
    ConnectionProfile lastProfile;

    void start(const ConnectionProfile &profile) override
    {
        ++startCalls;
        lastProfile = profile;
        emit started();
        emit connectionEstablished();
    }
    void stop() override { ++stopCalls; }
    void writeInput(const QByteArray &) override {}
    void finish() { emit finished(); }
};

class FakeProxyBackend : public SystemProxyBackend
{
public:
    std::atomic<int> appliedHttpPort{0};
    std::atomic<int> clearCalls{0};

    bool hasConflict(const SystemProxyConfig &) override { return false; }
    bool apply(const SystemProxyConfig &config) override
    {
        appliedHttpPort = config.httpPort;
        return true;
    }
    bool clear() override
    {
        ++clearCalls;
        return true;
    }
};

bool waitFor(const std::function<bool()> &condition)
{
    QElapsedTimer timer;
    timer.start();
    while (!condition() && timer.elapsed() < 5000)
    {
        QApplication::processEvents();
        QThread::msleep(5);
    }
    return condition();
}

void configure(QSettings &settings, const QString &server, int httpPort)
{
    settings.setValue("ZJUConnect/Protocol", "easyconnect");
    settings.setValue("ZJUConnect/EasyConnectAuthType", "password");
    settings.setValue("ZJUConnect/ServerAddress", server);
    settings.setValue("ZJUConnect/ServerPort", 443);
    settings.setValue("ZJUConnect/HTTPPort", httpPort);
    settings.setValue("Credential/Username", "alice");
    settings.setValue("Credential/Password", QByteArray("secret").toBase64());
    settings.sync();
}

bool selectionKeepsConnectionUntilConnect()
{
    QTemporaryDir directory;
    if (!directory.isValid()) return false;
    QSettings first(directory.filePath("first.ini"), QSettings::IniFormat);
    QSettings second(directory.filePath("second.ini"), QSettings::IniFormat);
    configure(first, "first.example", 11081);
    configure(second, "second.example", 22081);
    second.setValue("ZJUConnect/Protocol", "atrust");
    second.setValue("ZJUConnect/AuthType", "psw");
    second.sync();
    QSettings *selected = &first;
    QString selectedId = "first";

    QWidget window;
    QPushButton connectButton;
    QPushButton proxyButton;
    QAction trayAction(&window);
    auto *core = new FakeCoreProcess;
    ConnectionSession session(core);
    auto backend = std::make_unique<FakeProxyBackend>();
    auto *proxyBackend = backend.get();
    SystemProxySession proxy(std::move(backend));
    AuthDialogCoordinator auth(&window, selected);
    ApplicationLogger logger;
    ConnectionUiController controller(
        &window, &connectButton, &proxyButton, &trayAction, &session, &proxy,
        &auth, &logger, [&]() { return selected; }, [&]() { return selectedId; },
        [](const QString &, const QString &, QSystemTrayIcon::MessageIcon) {}
    );
    QObject::connect(&session, &ConnectionSession::finished, &proxy, [&]()
    {
        if (proxy.isEnabled()) proxy.disable();
    });

    connectButton.click();
    if (core->startCalls != 1 || core->lastProfile.endpoint.server != "first.example"
        || connectButton.text() != "断开服务器") return false;

    selected = &second;
    selectedId = "second";
    controller.profileSelectionChanged();
    if (core->stopCalls != 0 || session.state() != ConnectionState::Running
        || connectButton.text() != "连接服务器"
        || controller.connectedProfileName() != "first"
        || controller.connectedProfileId() != "first"
        || controller.connectedConfigPath() != first.fileName()) return false;

    auth.requestGraphCaptcha(QString());
    auto *captcha = window.findChild<GraphCaptchaWindow *>();
    auto *captchaInput = captcha ? captcha->findChild<QLineEdit *>("codeEdit") : nullptr;
    if (!captchaInput || captchaInput->isHidden()) return false;
    delete captcha;

    selected = &first;
    selectedId = "first";
    controller.profileSelectionChanged();
    if (core->stopCalls != 0 || connectButton.text() != "断开服务器") return false;
    selected = &second;
    selectedId = "second";
    controller.profileSelectionChanged();

    proxyButton.click();
    if (!waitFor([&]() { return proxy.isEnabled(); })
        || proxyBackend->appliedHttpPort != 11081) return false;

    connectButton.click();
    if (core->stopCalls != 1 || core->startCalls != 1) return false;
    core->finish();
    if (!waitFor([&]() { return core->startCalls == 2; })
        || core->lastProfile.endpoint.server != "second.example"
        || proxyBackend->clearCalls != 1 || proxy.isEnabled()
        || connectButton.text() != "断开服务器") return false;

    connectButton.click();
    if (core->stopCalls != 2) return false;
    core->finish();
    return !session.isActive() && connectButton.text() == "连接服务器";
}

bool restoresProxyStateAfterUpdate()
{
    QTemporaryDir directory;
    if (!directory.isValid()) return false;
    QSettings settings(directory.filePath("vpn.ini"), QSettings::IniFormat);
    configure(settings, "vpn.example", 11081);
    settings.setValue("Common/AutoSetProxy", true);
    QWidget window;
    QPushButton connectButton;
    QPushButton proxyButton;
    QAction trayAction(&window);
    auto *core = new FakeCoreProcess;
    ConnectionSession session(core);
    auto backend = std::make_unique<FakeProxyBackend>();
    auto *proxyBackend = backend.get();
    SystemProxySession proxy(std::move(backend));
    AuthDialogCoordinator auth(&window, &settings);
    ApplicationLogger logger;
    ConnectionUiController controller(
        &window, &connectButton, &proxyButton, &trayAction, &session, &proxy,
        &auth, &logger, [&]() { return &settings; }, []() { return QStringLiteral("vpn"); },
        [](const QString &, const QString &, QSystemTrayIcon::MessageIcon) {}
    );

    controller.restoreProxyOnNextConnection(false);
    connectButton.click();
    if (core->startCalls != 1 || proxy.isEnabled() || proxyBackend->appliedHttpPort != 0)
        return false;
    connectButton.click();
    core->finish();

    settings.setValue("Common/AutoSetProxy", false);
    controller.restoreProxyOnNextConnection(true);
    connectButton.click();
    if (!waitFor([&]() { return proxy.isEnabled(); })
        || core->startCalls != 2 || proxyBackend->appliedHttpPort != 11081)
        return false;
    proxy.disable();
    if (!waitFor([&]() { return !proxy.isBusy(); })) return false;
    connectButton.click();
    core->finish();
    return !session.isActive();
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    return selectionKeepsConnectionUntilConnect() && restoresProxyStateAfterUpdate() ? 0 : 1;
}
