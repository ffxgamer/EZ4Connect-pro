#include <QCoreApplication>
#include <QDebug>

#include "application/updaterestartstate.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const UpdateRestartState state{QStringLiteral("学校"), QStringLiteral("/tmp/VPN 配置.ini"), true, true};
    const auto restored = UpdateRestartState::decode(state.encode());
    if (!restored || restored->profileId != state.profileId || restored->configPath != state.configPath
        || !restored->externalConfig || !restored->restoreSystemProxy)
    {
        qCritical() << "Update restart state did not round trip";
        return 1;
    }
    const UpdateRestartState legacy{QString(), QStringLiteral("/tmp/legacy.ini"), false, false};
    const auto legacyRestored = UpdateRestartState::decode(legacy.encode());
    if (!legacyRestored || !legacyRestored->profileId.isEmpty() || legacyRestored->externalConfig
        || legacyRestored->restoreSystemProxy || UpdateRestartState::decode(QStringLiteral("!invalid!"))
        || UpdateRestartState::decode(QString(16385, 'a'))
        || UpdateRestartState::decode(UpdateRestartState{}.encode()))
    {
        qCritical() << "Invalid update restart state was accepted or legacy state was lost";
        return 1;
    }
    return 0;
}
