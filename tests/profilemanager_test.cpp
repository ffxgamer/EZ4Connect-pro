#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QSettings>
#include "application/profileservice.h"

#include "infrastructure/settings/profilemanager.h"

namespace
{
bool createsPhysicalFilesForEmptyProfiles()
{
    QTemporaryDir directory;
    ProfileManager manager(directory.path());

    if (!manager.activeProfile().isEmpty()
        || !QFileInfo::exists(manager.profilePath(""))
        || manager.hasLegacyProfile()
        || !manager.listProfiles().isEmpty()
        || !manager.setActiveProfile(""))
    {
        qCritical() << "empty draft was not created";
        return false;
    }

    const QString profileId = manager.createProfile("empty");
    if (profileId != "empty"
        || !QFileInfo::exists(manager.profilePath(profileId))
        || !manager.listProfiles().contains(profileId)
        || !manager.setActiveProfile(profileId))
    {
        qCritical() << "empty named profile file was not created";
        return false;
    }
    const QString chineseId = manager.createProfile(QStringLiteral("公司 VPN"));
    if (chineseId != QStringLiteral("公司 VPN") ||
        !manager.listProfiles().contains(chineseId) || !manager.setActiveProfile(chineseId) ||
        manager.activeProfile() != chineseId)
    {
        qCritical() << "Unicode profile name was not preserved";
        return false;
    }
    return true;
}

bool migratesAndRemovesLegacyProfile()
{
    QTemporaryDir directory;
    {
        QSettings legacy(directory.filePath("config.ini"), QSettings::IniFormat);
        legacy.setValue("ZJUConnect/ServerAddress", "192.0.2.1");
        legacy.setValue("Credential/Username", "legacy-user");
        legacy.setValue("Credential/Password", "legacy-password");
        legacy.sync();
    }
    {
        auto manager = std::make_unique<ProfileManager>(directory.path());
        ProfileService service(std::move(manager));
        if (service.currentProfileId() != "VPN"
            || service.settings()->value("Credential/Username") != "legacy-user"
            || service.settings()->value("Credential/Password") != "legacy-password"
            || QFileInfo::exists(directory.filePath("config.ini"))) return false;
        if (!service.renameCurrent(QStringLiteral("公司"))
            || service.settings()->value("ZJUConnect/ServerAddress") != "192.0.2.1") return false;
        if (service.createAndSwitch(QStringLiteral("学校"), false) != QStringLiteral("学校")) return false;
        if (!service.switchTo(QStringLiteral("公司")) || !service.removeCurrentAndSwitchToAnother()
            || service.currentProfileId() != QStringLiteral("学校")) return false;
        if (!service.removeCurrentAndSwitchToAnother() || service.hasSelectedProfile()
            || !service.profiles().isEmpty()) return false;
    }
    ProfileManager reloaded(directory.path());
    if (!reloaded.activeProfile().isEmpty() || !reloaded.listProfiles().isEmpty()
        || reloaded.hasLegacyProfile()) return false;
    return true;
}

}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    return createsPhysicalFilesForEmptyProfiles() && migratesAndRemovesLegacyProfile() ? 0 : 1;
}
