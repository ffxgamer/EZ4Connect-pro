#include "coreoutputparser.h"
#include <QList>

CoreOutputEvent CoreOutputParser::parse(const QString &output)
{
    if (output.contains("SUDO_ASK_PASS"))
    {
        return CoreOutputEvent::AskSudoPassword;
    }
    if (output.contains("Graph check code saved to "))
    {
        return CoreOutputEvent::GraphCaptcha;
    }
    if (output.contains("Please enter the SMS verification code: "))
    {
        return CoreOutputEvent::SmsCodeWithSkipOption;
    }
    if (output.contains("Please enter your SMS code:"))
    {
        return CoreOutputEvent::SmsCode;
    }
    if (output.contains("Please enter your TOTP code:"))
    {
        return CoreOutputEvent::TotpCode;
    }
    if (output.contains("Please enter rand code:"))
    {
        return CoreOutputEvent::RandCode;
    }
    // aTrust RADIUS Access-Challenge：核心在密码认证后输出此提示并等待用户输入短信验证码。
    if (output.contains("Please enter the RADIUS token:"))
    {
        return CoreOutputEvent::RadiusCodeWithSkipOption;
    }
    if (output.contains("Please enter the callback url:"))
    {
        return CoreOutputEvent::SsoCallback;
    }
    if (output.contains("VPN client started"))
    {
        return CoreOutputEvent::ClientStarted;
    }
    if (output.contains("graph check code still required after second login attempt") ||
        output.contains("The characters are incorrect or has expired!"))
    {
        return CoreOutputEvent::CaptchaFailed;
    }
    if (output.contains("Access is denied."))
    {
        return CoreOutputEvent::AccessDenied;
    }
    if (output.contains("listen failed"))
    {
        return CoreOutputEvent::ListenFailed;
    }
    if (output.contains("Invalid username or password!") || output.contains("ticket is empty"))
    {
        return CoreOutputEvent::InvalidCredentials;
    }
    if (output.contains("You are trying brute-force login on this IP address."))
    {
        return CoreOutputEvent::BruteForceBlocked;
    }
    if (output.contains("Login failed") || output.contains("too many login failures"))
    {
        return CoreOutputEvent::LoginFailed;
    }
    if (output.contains("unexpected newline"))
    {
        return CoreOutputEvent::InteractiveError;
    }
    if (output.contains("auth type/login domain combination not found"))
    {
        return CoreOutputEvent::AuthNotAvailable;
    }
    if (output.contains("invalid SID") || output.contains("l3-tunnel tunnel auth failed:"))
    {
        return CoreOutputEvent::AuthExpired;
    }
    if (output.contains("server returned SHUTDOWN")
        || output.contains("server SHUTDOWN; running cleanup hooks"))
    {
        return CoreOutputEvent::ServerShutdown;
    }
    if (output.contains("client setup error"))
    {
        return CoreOutputEvent::ClientFailed;
    }
    if (output.contains("panic"))
    {
        return CoreOutputEvent::CorePanic;
    }
    return CoreOutputEvent::None;
}

QString CoreOutputParser::graphCaptchaFile(const QString &output)
{
    const QString prefix = "Graph check code saved to ";
    const qsizetype markerIndex = output.indexOf(prefix);
    if (markerIndex < 0)
    {
        return {};
    }

    return output.mid(markerIndex + prefix.size()).trimmed();
}

bool CoreOutputParser::trafficTotals(const QByteArray &line, quint64 &received, quint64 &sent)
{
    const QList<QByteArray> fields = line.split(' ');
    if (fields.size() != 3 || fields[0] != "EZ4PRO_TRAFFIC" || fields[1].isEmpty() || fields[2].isEmpty())
        return false;
    for (const QByteArray &number : {fields[1], fields[2]})
        for (char c : number)
            if (c < '0' || c > '9') return false;
    bool receivedOk = false;
    bool sentOk = false;
    const quint64 parsedReceived = fields[1].toULongLong(&receivedOk);
    const quint64 parsedSent = fields[2].toULongLong(&sentOk);
    if (!receivedOk || !sentOk) return false;
    received = parsedReceived;
    sent = parsedSent;
    return true;
}

bool CoreOutputParser::hasInteractivePrompt(const QByteArray &output)
{
    return output.contains("SUDO_ASK_PASS")
           || output.contains("Please enter the SMS verification code: ")
           || output.contains("Please enter your SMS code:")
           || output.contains("Please enter your TOTP code:")
           || output.contains("Please enter rand code:")
           || output.contains("Please enter the RADIUS token:")
           || output.contains("Please enter the callback url:");
}
