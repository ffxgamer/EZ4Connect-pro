#include "updatemanifest.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

std::optional<UpdateAsset> UpdateManifest::parse(const QByteArray &payload, const QString &platform)
{
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) return std::nullopt;
    const QJsonObject root = doc.object();
    const QString version = root.value("version").toString();
    const QRegularExpression versionPattern(QStringLiteral("^v?[0-9]+\\.[0-9]+\\.[0-9]+(?:-[A-Za-z0-9.-]+)?$"));
    if (!versionPattern.match(version).hasMatch()) return std::nullopt;
    const QJsonObject entry = root.value("assets").toObject().value(platform).toObject();
    const QUrl url(entry.value("url").toString(), QUrl::StrictMode);
    const QString digest = entry.value("sha256").toString().toLower();
    const qint64 size = static_cast<qint64>(entry.value("size").toDouble());
    const QString tag = version.startsWith('v') ? version : "v" + version;
    const QString expectedPath = "/ffxgamer/EZ4Connect-pro/releases/download/" + tag
        + "/EZ4Connect-pro-" + tag + "-" + platform + ".dmg";
    if (!url.isValid() || url.scheme() != "https" || url.host() != "github.com"
        || url.path() != expectedPath || url.hasQuery() || url.hasFragment()
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(digest).hasMatch()
        || size <= 0 || size > 1024LL * 1024 * 1024)
        return std::nullopt;
    return UpdateAsset{version.startsWith('v') ? version.mid(1) : version, url, digest.toLatin1(), size};
}

std::optional<QUrl> UpdateManifest::proxyUrl(const QString &prefix, const QUrl &source)
{
    if (prefix.trimmed().isEmpty()) return source;
    QString clean = prefix.trimmed();
    const QUrl proxy(clean, QUrl::StrictMode);
    if (!proxy.isValid() || proxy.scheme() != "https" || proxy.host().isEmpty()
        || !proxy.userInfo().isEmpty() || proxy.hasQuery() || proxy.hasFragment())
        return std::nullopt;
    while (clean.endsWith('/')) clean.chop(1);
    return QUrl(clean + '/' + source.toString(QUrl::FullyEncoded), QUrl::StrictMode);
}
