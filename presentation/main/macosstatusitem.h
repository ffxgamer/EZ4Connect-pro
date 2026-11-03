#pragma once

#include <QString>
#include <functional>

// Native status item avoids Qt 6.10's macOS 27 non-mouse-event crash.
class MacOSStatusItem
{
public:
    explicit MacOSStatusItem(std::function<void()> onActivated,
                             std::function<void()> onNotificationClicked);
    ~MacOSStatusItem();
    void update(bool connected, const QString &download, const QString &upload);
    void notify(const QString &title, const QString &message);

private:
    void *implementation = nullptr;
};
