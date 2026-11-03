#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <optional>

// Carries only the identity of the active connection across an application restart.
struct UpdateRestartState
{
    QString profileId;
    QString configPath;
    bool externalConfig = false;
    bool restoreSystemProxy = false;

    QString encode() const
    {
        const QJsonObject object{{"profile", profileId}, {"config", configPath},
                                 {"external", externalConfig}, {"proxy", restoreSystemProxy}};
        return QString::fromLatin1(QJsonDocument(object).toJson(QJsonDocument::Compact)
            .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    }

    static std::optional<UpdateRestartState> decode(const QString &token)
    {
        if (token.isEmpty() || token.size() > 16384) return std::nullopt;
        const auto decoded = QByteArray::fromBase64Encoding(token.toLatin1(),
            QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
        if (!decoded) return std::nullopt;
        const QJsonDocument document = QJsonDocument::fromJson(decoded.decoded);
        if (!document.isObject()) return std::nullopt;
        const QJsonObject object = document.object();
        if (!object.value("profile").isString() || !object.value("config").isString()
            || !object.value("external").isBool() || !object.value("proxy").isBool())
            return std::nullopt;
        UpdateRestartState state{object.value("profile").toString(), object.value("config").toString(),
                                 object.value("external").toBool(), object.value("proxy").toBool()};
        if (state.configPath.isEmpty()) return std::nullopt;
        return state;
    }
};
