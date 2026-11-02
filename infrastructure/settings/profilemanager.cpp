#include "profilemanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>

namespace
{
bool createEmptyFile(const QString &path)
{
    const QFileInfo fileInfo(path);
    if (fileInfo.exists())
    {
        return fileInfo.isFile();
    }

    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::NewOnly);
}
}

ProfileManager::ProfileManager(const QString &storageRoot)
{
    configRootPath = storageRoot.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        : storageRoot;
    profilesPath = QDir(configRootPath).filePath("profiles");
    statePath = QDir(configRootPath).filePath("state.ini");
    defaultProfilePath = QDir(configRootPath).filePath("config.ini");
    draftProfilePath = QDir(configRootPath).filePath("draft.ini");

    ensureStorage();
    migrateLegacyProfile();
}

QStringList ProfileManager::listProfiles() const
{
    QDir profilesDir(profilesPath);
    QStringList profiles;
    for (const QFileInfo &entry : profilesDir.entryInfoList(QStringList() << "*.ini", QDir::Files, QDir::Name))
    {
        if (entry.fileName() == "state.ini")
        {
            continue;
        }
        profiles << entry.baseName();
    }
    return profiles;
}

void ProfileManager::migrateLegacyProfile() const
{
    if (!QFileInfo::exists(defaultProfilePath)) return;
    const QString id = ensureUniqueProfileId(QStringLiteral("VPN"));
    if (!QFile::rename(defaultProfilePath, profilePath(id))) return;
    QSettings state(statePath, QSettings::IniFormat);
    if (state.value("Profile/Active").toString().isEmpty())
    {
        state.setValue("Profile/Active", id);
        state.sync();
    }
}

bool ProfileManager::hasLegacyProfile() const
{
    return QFileInfo::exists(defaultProfilePath);
}

QString ProfileManager::activeProfile() const
{
    QSettings state(statePath, QSettings::IniFormat);
    QString profileId = normalizeProfileId(state.value("Profile/Active").toString());
    const QStringList profiles = listProfiles();
    if (profileId.isEmpty() || !profiles.contains(profileId))
    {
        if (QFileInfo::exists(defaultProfilePath)) profileId.clear();
        else profileId = profiles.isEmpty() ? QString() : profiles.first();
        state.setValue("Profile/Active", profileId);
        state.sync();
    }
    if (profileId.isEmpty() && !QFileInfo::exists(profilePath(profileId)))
        createEmptyFile(profilePath(profileId));
    return profileId;
}

bool ProfileManager::setActiveProfile(const QString &profileId) const
{
    const QString normalizedId = normalizeProfileId(profileId);
    if (!QFileInfo::exists(profilePath(normalizedId)))
    {
        return false;
    }

    QSettings state(statePath, QSettings::IniFormat);
    state.setValue("Profile/Active", normalizedId);
    state.sync();
    return state.status() == QSettings::NoError;
}

QString ProfileManager::profilePath(const QString &profileId) const
{
    QString normalizedId = normalizeProfileId(profileId);
    if (normalizedId.isEmpty())
    {
        return QFileInfo::exists(defaultProfilePath) ? defaultProfilePath : draftProfilePath;
    }
    return QDir(profilesPath).filePath(normalizedId + ".ini");
}

QString ProfileManager::createProfile(const QString &requestedName, const QString &sourcePath) const
{
    const QString normalizedId = ensureUniqueProfileId(normalizeProfileId(requestedName));
    const QString path = profilePath(normalizedId);

    if (!sourcePath.isEmpty() && QFileInfo::exists(sourcePath))
    {
        if (QFile::exists(path))
        {
            QFile::remove(path);
        }
        if (!QFile::copy(sourcePath, path))
        {
            return QString();
        }
    }
    else
    {
        if (!createEmptyFile(path))
        {
            return {};
        }
    }

    return normalizedId;
}

bool ProfileManager::renameProfile(const QString &oldId, const QString &newId) const
{
    const QString sourceId = normalizeProfileId(oldId);
    const QString targetId = normalizeProfileId(newId);
    if (sourceId == targetId || (targetId.isEmpty() && QFileInfo::exists(defaultProfilePath)))
    {
        return false;
    }

    const QString sourcePath = profilePath(sourceId);
    const QString targetPath = targetId.isEmpty() ? defaultProfilePath : profilePath(targetId);
    if (!QFileInfo::exists(sourcePath) || QFileInfo::exists(targetPath))
    {
        return false;
    }

    return QFile::rename(sourcePath, targetPath);
}

bool ProfileManager::removeProfile(const QString &profileId) const
{
    const QString normalizedId = normalizeProfileId(profileId);
    if (normalizedId.isEmpty() && !QFileInfo::exists(defaultProfilePath)) return false;
    return QFile::remove(profilePath(normalizedId));
}

QString ProfileManager::normalizeProfileId(const QString &name) const
{
    QString normalized = name.trimmed();
    normalized.replace(QRegularExpression("[^\\p{L}\\p{N}_ -]"), "_");
    normalized.replace(QRegularExpression("_+"), "_");
    if (normalized.startsWith('_'))
    {
        normalized.remove(0, 1);
    }
    if (normalized.endsWith('_'))
    {
        normalized.chop(1);
    }
    return normalized;
}

bool ProfileManager::autoStartEnabled() const
{
    QSettings state(statePath, QSettings::IniFormat);
    return state.value("Global/AutoStart", false).toBool();
}

void ProfileManager::setAutoStartEnabled(bool enabled) const
{
    QSettings state(statePath, QSettings::IniFormat);
    state.setValue("Global/AutoStart", enabled);
    state.sync();
}

bool ProfileManager::silentStartEnabled() const
{
    QSettings state(statePath, QSettings::IniFormat);
    return state.value("Global/SilentStart", false).toBool();
}

void ProfileManager::setSilentStartEnabled(bool enabled) const
{
    QSettings state(statePath, QSettings::IniFormat);
    state.setValue("Global/SilentStart", enabled);
    state.sync();
}

QString ProfileManager::ensureUniqueProfileId(const QString &baseId) const
{
    QString candidate = baseId;
    if (candidate.isEmpty())
    {
        candidate = "profile";
    }

    if (!QFileInfo::exists(profilePath(candidate)))
    {
        return candidate;
    }

    int suffix = 2;
    while (QFileInfo::exists(profilePath(candidate + "_" + QString::number(suffix))))
    {
        suffix++;
    }
    return candidate + "_" + QString::number(suffix);
}

void ProfileManager::ensureStorage() const
{
    QDir configRoot(configRootPath);
    if (!configRoot.exists())
    {
        configRoot.mkpath(".");
    }

    QDir profilesDir(profilesPath);
    if (!profilesDir.exists())
    {
        profilesDir.mkpath(".");
    }
}
