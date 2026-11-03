#ifndef COREOUTPUTPARSER_H
#define COREOUTPUTPARSER_H

#include <QByteArray>
#include <QString>

enum class CoreOutputEvent
{
    None,
    AskSudoPassword,
    GraphCaptcha,
    SmsCodeWithSkipOption,
    SmsCode,
    TotpCode,
    RandCode,
    RadiusCodeWithSkipOption,
    SsoCallback,
    ClientStarted,
    CaptchaFailed,
    AccessDenied,
    ListenFailed,
    InvalidCredentials,
    BruteForceBlocked,
    LoginFailed,
    InteractiveError,
    AuthNotAvailable,
    AuthExpired,
    ServerShutdown,
    ClientFailed,
    CorePanic,
};

class CoreOutputParser
{
public:
    static CoreOutputEvent parse(const QString &output);

    static bool trafficTotals(const QByteArray &line, quint64 &received, quint64 &sent);

    static bool hasInteractivePrompt(const QByteArray &output);

    static QString graphCaptchaFile(const QString &output);
};

#endif // COREOUTPUTPARSER_H
