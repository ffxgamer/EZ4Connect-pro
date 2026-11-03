#include "macosstatusitem.h"
#import <AppKit/AppKit.h>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

@interface EZ4StatusDelegate : NSObject <NSUserNotificationCenterDelegate> {
@public
    std::function<void()> onActivated;
    std::function<void()> onNotificationClicked;
}
- (void)activated:(id)sender;
@end

@implementation EZ4StatusDelegate
- (void)activated:(id)sender
{
    (void)sender;
    // The action may originate from a gesture, not a mouse event, on macOS 27.
    if (onActivated) onActivated();
}
- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification
{
    (void)center; (void)notification; return YES;
}
- (void)userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)notification
{
    [center removeDeliveredNotification:notification];
    if (onNotificationClicked) onNotificationClicked();
}
@end

struct StatusImplementation {
    NSStatusItem *item;
    EZ4StatusDelegate *delegate;
};

MacOSStatusItem::MacOSStatusItem(std::function<void()> onActivated,
                                 std::function<void()> onNotificationClicked)
{
    auto *impl = new StatusImplementation;
    impl->item = [[NSStatusBar.systemStatusBar statusItemWithLength:NSVariableStatusItemLength] retain];
    impl->delegate = [[EZ4StatusDelegate alloc] init];
    impl->delegate->onActivated = std::move(onActivated);
    impl->delegate->onNotificationClicked = std::move(onNotificationClicked);
    impl->item.button.target = impl->delegate;
    impl->item.button.action = @selector(activated:);
    [impl->item.button sendActionOn:NSEventMaskLeftMouseDown | NSEventMaskRightMouseDown];
    // Keep the application's original icon visible beside the traffic readout.
    NSImage *icon = [NSApp.applicationIconImage copy];
    icon.size = NSMakeSize(24, 24);
    impl->item.button.image = icon;
    impl->item.button.imagePosition = NSImageLeft;
    impl->item.button.imageScaling = NSImageScaleProportionallyDown;
    [icon release];
    impl->item.button.toolTip = @"EZ4Connect Pro · 点击打开菜单";
    implementation = impl;
    update(false, QStringLiteral("0 KB/s"), QStringLiteral("0 KB/s"));
}

MacOSStatusItem::~MacOSStatusItem()
{
    auto *impl = static_cast<StatusImplementation *>(implementation);
    if (NSUserNotificationCenter.defaultUserNotificationCenter.delegate == impl->delegate)
        NSUserNotificationCenter.defaultUserNotificationCenter.delegate = nil;
    [NSStatusBar.systemStatusBar removeStatusItem:impl->item];
    [impl->item release];
    [impl->delegate release];
    delete impl;
}

void MacOSStatusItem::update(bool connected, const QString &download, const QString &upload)
{
    auto *impl = static_cast<StatusImplementation *>(implementation);
    NSString *text = [NSString stringWithFormat:@"↓ %@\n↑ %@", download.toNSString(), upload.toNSString()];
    NSMutableParagraphStyle *style = [[[NSMutableParagraphStyle alloc] init] autorelease];
    style.minimumLineHeight = 10;
    style.maximumLineHeight = 10;
    NSDictionary *attributes = @{
        NSFontAttributeName: [NSFont monospacedDigitSystemFontOfSize:10 weight:NSFontWeightMedium],
        NSForegroundColorAttributeName: (connected ? NSColor.systemGreenColor : NSColor.systemRedColor),
        NSBaselineOffsetAttributeName: @(-4.0),
        NSParagraphStyleAttributeName: style
    };
    NSMutableAttributedString *title = [[[NSMutableAttributedString alloc] initWithString:text attributes:attributes] autorelease];
    impl->item.button.attributedTitle = title;
}

void MacOSStatusItem::notify(const QString &title, const QString &message)
{
    auto *impl = static_cast<StatusImplementation *>(implementation);
    NSUserNotification *notification = [[[NSUserNotification alloc] init] autorelease];
    notification.title = title.toNSString();
    notification.informativeText = message.toNSString();
    NSUserNotificationCenter *center = NSUserNotificationCenter.defaultUserNotificationCenter;
    center.delegate = impl->delegate;
    [center deliverNotification:notification];
}
