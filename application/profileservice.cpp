#include "profileservice.h"

#include <QFile>
#include <QFileInfo>
#include <QSettings>

#include <utility>

#include "application/profilebackend.h"

ProfileService::ProfileService(
    std::unique_ptr<ProfileBackend> backend,
    const QString &overrideConfigPath,
    QObject *parent
)
    : QObject(parent),
      profileBackend(std::move(backend)),
      currentSettings(nullptr),
      overrideConfiguration(!overrideConfigPath.isEmpty())
{
    if (!overrideConfiguration)
    {
        currentId = profileBackend->activeProfile();
        currentSettings = new QSettings(
            profileBackend->profilePath(currentId),
            QSettings::IniFormat
        );
    }
    else
    {
        currentId = "custom";
        currentSettings = new QSettings(overrideConfigPath, QSettings::IniFormat);
    }
}

ProfileService::~ProfileService()
{
    currentSettings->sync();
    delete currentSettings;
}

QSettings *ProfileService::settings() const
{
    return currentSettings;
}

QString ProfileService::currentProfileId() const
{
    return currentId;
}

QStringList ProfileService::profiles() const
{
    return profileBackend->listProfiles();
}

bool ProfileService::usesOverrideConfiguration() const
{
    return overrideConfiguration;
}

bool ProfileService::switchTo(const QString &profileId)
{
    if (!overrideConfiguration && profileId == currentId)
    {
        return true;
    }

    if (!QFileInfo::exists(profileBackend->profilePath(profileId)))
    {
        return false;
    }

    currentSettings->sync();
    if (!profileBackend->setActiveProfile(profileId))
    {
        return false;
    }

    delete currentSettings;
    currentSettings = new QSettings(
        profileBackend->profilePath(profileId),
        QSettings::IniFormat
    );
    currentId = profileId;
    overrideConfiguration = false;
    return true;
}

QString ProfileService::createAndSwitch(const QString &requestedName, bool copyCurrent)
{
    currentSettings->sync();
    const QString profileId = profileBackend->createProfile(
        requestedName,
        copyCurrent ? currentSettings->fileName() : QString()
    );
    if (profileId.isEmpty() || !switchTo(profileId))
    {
        if (!profileId.isEmpty())
        {
            profileBackend->removeProfile(profileId);
        }
        return {};
    }
    return profileId;
}

bool ProfileService::renameCurrent(const QString &newProfileId)
{
    if (overrideConfiguration || !hasSelectedProfile())
    {
        return false;
    }

    currentSettings->sync();
    if (!profileBackend->renameProfile(currentId, newProfileId))
    {
        return false;
    }

    const QString previousId = currentId;
    if (!profileBackend->setActiveProfile(newProfileId))
    {
        profileBackend->renameProfile(newProfileId, previousId);
        return false;
    }

    currentId = newProfileId;
    delete currentSettings;
    currentSettings = new QSettings(
        profileBackend->profilePath(currentId),
        QSettings::IniFormat
    );
    return true;
}

bool ProfileService::hasLegacyProfile() const
{
    return profileBackend->hasLegacyProfile();
}

bool ProfileService::hasSelectedProfile() const
{
    return overrideConfiguration || !currentId.isEmpty()
        || profileBackend->hasLegacyProfile();
}

bool ProfileService::removeCurrentAndSwitchToAnother()
{
    if (overrideConfiguration || !hasSelectedProfile()) return false;
    const QString removedId = currentId;
    QStringList remaining = profileBackend->listProfiles();
    remaining.removeAll(removedId);
    const QString nextId = remaining.isEmpty() ? QString() : remaining.first();
    currentSettings->sync();
    if (nextId.isEmpty() && removedId.isEmpty())
    {
        // The legacy file is the last profile. Remove it before opening the empty draft.
        if (!profileBackend->removeProfile(removedId)) return false;
        delete currentSettings;
        currentSettings = new QSettings(profileBackend->profilePath(""), QSettings::IniFormat);
        currentSettings->sync();
        currentId.clear();
        profileBackend->setActiveProfile("");
        return true;
    }
    // A draft is used only when no named profiles remain.
    if (nextId.isEmpty() && !QFileInfo::exists(profileBackend->profilePath("")))
    {
        QFile draft(profileBackend->profilePath(""));
        if (!draft.open(QIODevice::WriteOnly | QIODevice::NewOnly)) return false;
    }
    if (!profileBackend->setActiveProfile(nextId)) return false;
    if (!profileBackend->removeProfile(removedId))
    {
        profileBackend->setActiveProfile(removedId);
        return false;
    }
    delete currentSettings;
    currentSettings = new QSettings(profileBackend->profilePath(nextId), QSettings::IniFormat);
    currentId = nextId;
    return true;
}

QString ProfileService::normalizeProfileId(const QString &name) const
{
    return profileBackend->normalizeProfileId(name);
}

bool ProfileService::silentStartEnabled() const
{
    return profileBackend->silentStartEnabled();
}

void ProfileService::migrateAutoStartSetting(bool enabled)
{
    profileBackend->setAutoStartEnabled(enabled);
}
