#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>
#include <optional>

struct UpdateAsset
{
    QString version;
    QUrl url;
    QByteArray sha256;
    qint64 size = 0;
};

namespace UpdateManifest
{
std::optional<UpdateAsset> parse(const QByteArray &payload, const QString &platform);
std::optional<QUrl> proxyUrl(const QString &prefix, const QUrl &source);
}
