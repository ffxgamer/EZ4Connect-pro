#include "updatedialog.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QVBoxLayout>

#include "infrastructure/update/updatechecker.h"

namespace
{
const QString manifestUrl = QStringLiteral("https://raw.githubusercontent.com/ffxgamer/EZ4Connect-pro/master/updates/latest.json");

QString settingsPath()
{
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(root);
    return QDir(root).filePath(QStringLiteral("updater.ini"));
}

struct FetchResult
{
    bool ok = false;
    bool cancelled = false;
    QByteArray data;
    QByteArray sha256;
    qint64 size = 0;
    QString error;
};

FetchResult fetch(QNetworkAccessManager &network, const QUrl &url, QFile *file, qint64 limit, QWidget *parent)
{
    FetchResult result;
    QProgressDialog progress(file ? QStringLiteral("正在下载更新…") : QStringLiteral("正在检查更新…"),
                             QStringLiteral("取消"), 0, 0, parent);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(300);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("EZ4Connect-Pro-Updater"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(45000);
    QNetworkReply *reply = network.get(request);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    QEventLoop loop;
    bool writeError = false;
    QObject::connect(reply, &QIODevice::readyRead, &loop, [&]()
    {
        const QByteArray chunk = reply->readAll();
        result.size += chunk.size();
        if (result.size > limit || (file && file->write(chunk) != chunk.size()))
        {
            writeError = true;
            reply->abort();
            return;
        }
        hash.addData(chunk);
        if (!file) result.data.append(chunk);
    });
    QObject::connect(reply, &QNetworkReply::downloadProgress, &progress, [&](qint64 received, qint64 total)
    {
        if (total > 0) { progress.setMaximum(100); progress.setValue(int(received * 100 / total)); }
    });
    QObject::connect(&progress, &QProgressDialog::canceled, &loop, [&]() { result.cancelled = true; reply->abort(); });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.ok = !writeError && !result.cancelled && reply->error() == QNetworkReply::NoError
        && status >= 200 && status < 300;
    result.error = writeError ? QStringLiteral("下载文件过大或无法写入") : reply->errorString();
    result.sha256 = hash.result().toHex();
    reply->deleteLater();
    return result;
}

QList<QUrl> routes(const QString &proxy, const QUrl &source)
{
    QList<QUrl> result;
    const auto proxied = UpdateManifest::proxyUrl(proxy, source);
    if (proxied && *proxied != source) result.append(*proxied);
    result.append(source);
    return result;
}
}

UpdateDialog::UpdateDialog(std::function<bool()> canInstall, QWidget *parent)
    : QDialog(parent), canInstall(std::move(canInstall))
{
    setWindowTitle(QStringLiteral("应用更新"));
    resize(520, 220);
    auto *layout = new QVBoxLayout(this);
    auto *description = new QLabel(QStringLiteral("GitHub 下载代理（留空为直连）："), this);
    layout->addWidget(description);
    QSettings settings(settingsPath(), QSettings::IniFormat);
    proxyInput = new QLineEdit(settings.value("Update/Proxy", "https://ghfast.top").toString(), this);
    proxyInput->setPlaceholderText(QStringLiteral("https://ghfast.top"));
    layout->addWidget(proxyInput);
    auto *hint = new QLabel(QStringLiteral("仅使用可信代理。代理需支持拼接完整 GitHub 链接；代理失败时会自动尝试直连。"), this);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    statusLabel = new QLabel(QStringLiteral("正在准备检查更新。"), this);
    statusLabel->setWordWrap(true);
    layout->addWidget(statusLabel);
    checkButton = new QPushButton(QStringLiteral("检查更新"), this);
    installButton = new QPushButton(QStringLiteral("下载并安装"), this);
    installButton->setEnabled(false);
    layout->addWidget(checkButton);
    layout->addWidget(installButton);
    connect(checkButton, &QPushButton::clicked, this, &UpdateDialog::checkForUpdates);
    connect(installButton, &QPushButton::clicked, this, &UpdateDialog::installUpdate);
    connect(proxyInput, &QLineEdit::textChanged, this, [this]() { asset.reset(); installButton->setEnabled(false); });
}

void UpdateDialog::checkForUpdates()
{
    const QString proxy = proxyInput->text().trimmed();
    if (!UpdateManifest::proxyUrl(proxy, QUrl(manifestUrl)))
    {
        QMessageBox::warning(this, QStringLiteral("代理地址无效"), QStringLiteral("请输入以 https:// 开头的代理地址，或留空直连。"));
        return;
    }
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue("Update/Proxy", proxy);
    settings.sync();
    checkButton->setEnabled(false);
    proxyInput->setEnabled(false);
    installButton->setEnabled(false);
    asset.reset();
    statusLabel->setText(QStringLiteral("正在检查更新…"));
    QString lastError;
    for (const QUrl &url : routes(proxy, QUrl(manifestUrl)))
    {
        const FetchResult response = fetch(network, url, nullptr, 1024 * 1024, this);
        if (response.cancelled) { lastError = QStringLiteral("已取消检查。"); break; }
        if (response.ok) asset = UpdateManifest::parse(response.data, QStringLiteral("macos-arm64"));
        if (asset) break;
        lastError = response.ok ? QStringLiteral("更新信息无效或没有适用于本机的安装包。") : response.error;
    }
    if (!asset) statusLabel->setText(QStringLiteral("检查更新失败：") + lastError);
    else if (!UpdateChecker::isNewerVersion(QApplication::applicationVersion(), asset->version))
    {
        statusLabel->setText(QStringLiteral("当前已是最新版本（%1）。").arg(QApplication::applicationVersion()));
        asset.reset();
    }
    else
    {
        statusLabel->setText(QStringLiteral("发现新版本 %1，下载并安装后应用将重新启动。").arg(asset->version));
        installButton->setEnabled(true);
    }
    proxyInput->setEnabled(true);
    checkButton->setEnabled(true);
}

void UpdateDialog::installUpdate()
{
#if !defined(Q_OS_MACOS) || !defined(Q_PROCESSOR_ARM_64)
    QMessageBox::information(this, QStringLiteral("暂不支持"), QStringLiteral("应用内更新目前仅支持 macOS Apple 芯片版本。"));
    return;
#else
    if (!asset) return;
    if (!canInstall())
    {
        QMessageBox::warning(this, "无法更新", "请先断开 VPN 并关闭系统代理，再安装更新。");
        return;
    }
    const QString appPath = QFileInfo(QCoreApplication::applicationDirPath() + "/../..").canonicalFilePath();
    if (!appPath.endsWith(".app") || !QFileInfo(QFileInfo(appPath).absolutePath()).isWritable())
    {
        QMessageBox::warning(this, QStringLiteral("无法更新"), QStringLiteral("请先将应用安装到可写入的“应用程序”文件夹，再运行更新。"));
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("安装更新"),
        QStringLiteral("将下载并安装 %1，应用会退出并重新启动。请先断开 VPN。是否继续？").arg(asset->version),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes) return;

    QTemporaryDir work(QDir::tempPath() + "/ez4connect-update-XXXXXX");
    if (!work.isValid()) { QMessageBox::warning(this, "更新失败", "无法创建临时目录。"); return; }
    checkButton->setEnabled(false);
    installButton->setEnabled(false);
    proxyInput->setEnabled(false);
    QString lastError;
    bool downloaded = false;
    for (const QUrl &url : routes(proxyInput->text().trimmed(), asset->url))
    {
        QFile file(work.filePath("update.dmg"));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) { lastError = file.errorString(); break; }
        const FetchResult response = fetch(network, url, &file, asset->size, this);
        const bool flushed = file.flush();
        file.close();
        if (response.cancelled) { lastError = QStringLiteral("已取消下载。"); break; }
        if (response.ok && flushed && response.size == asset->size && response.sha256 == asset->sha256)
        { downloaded = true; break; }
        lastError = response.ok ? QStringLiteral("安装包大小或 SHA-256 校验不匹配。") : response.error;
    }
    if (!downloaded)
    {
        statusLabel->setText(QStringLiteral("更新失败：") + lastError);
        proxyInput->setEnabled(true); checkButton->setEnabled(true); installButton->setEnabled(true);
        return;
    }
    QFile source(":/resource/update-macos.sh");
    const QString script = work.filePath("install.sh");
    if (!canInstall())
    {
        QMessageBox::warning(this, "无法更新", "VPN 或系统代理状态已变化，请先断开后重试。");
        proxyInput->setEnabled(true); checkButton->setEnabled(true); installButton->setEnabled(true);
        return;
    }
    if (!source.copy(script))
    {
        QMessageBox::warning(this, "更新失败", "无法准备安装程序。");
        proxyInput->setEnabled(true); checkButton->setEnabled(true); installButton->setEnabled(true);
        return;
    }
    const bool started = QProcess::startDetached("/bin/bash", {script, work.path(), appPath,
        QString::number(QCoreApplication::applicationPid()), asset->version});
    if (!started)
    {
        QMessageBox::warning(this, "更新失败", "无法启动安装程序。");
        proxyInput->setEnabled(true); checkButton->setEnabled(true); installButton->setEnabled(true);
        return;
    }
    work.setAutoRemove(false);
    QCoreApplication::quit();
#endif
}
