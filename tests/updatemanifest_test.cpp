#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>

#include "infrastructure/update/updatemanifest.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QString url = QStringLiteral("https://github.com/ffxgamer/EZ4Connect-pro/releases/download/v0.0.9/EZ4Connect-pro-v0.0.9-macos-arm64.dmg");
    QJsonObject entry{{"url", url}, {"sha256", QString(64, 'a')}, {"size", 123}};
    auto manifest = [&entry]() {
        return QJsonDocument(QJsonObject{{"version", "v0.0.9"}, {"assets", QJsonObject{{"macos-arm64", entry}}}}).toJson();
    };
    const auto parsed = UpdateManifest::parse(manifest(), "macos-arm64");
    if (!parsed || parsed->version != "0.0.9" || parsed->size != 123) return 1;
    if (UpdateManifest::parse(manifest(), "windows-x64")) return 1;
    entry["sha256"] = "bad";
    if (UpdateManifest::parse(manifest(), "macos-arm64")) return 1;
    entry["sha256"] = QString(64, 'a');
    entry["url"] = "https://evil.example/update.dmg";
    if (UpdateManifest::parse(manifest(), "macos-arm64")) return 1;
    entry["url"] = url;
    entry["size"] = 0;
    if (UpdateManifest::parse(manifest(), "macos-arm64")) return 1;
    const QUrl source(url);
    const auto proxy = UpdateManifest::proxyUrl("https://ghfast.top/", source);
    if (!proxy || proxy->toString() != "https://ghfast.top/" + url) return 1;
    if (UpdateManifest::proxyUrl("", source) != std::optional<QUrl>(source)) return 1;
    if (UpdateManifest::proxyUrl("http://invalid.example", source)) return 1;
    if (UpdateManifest::proxyUrl("https://user:pass@example.com/", source)) return 1;
    return 0;
}
