#include <QApplication>
#include <QDebug>
#include <QMainWindow>

#include "ui_mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    window.resize(840, 650);
    window.show();
    app.processEvents();

    const int originalWidth = ui.controlPanel->width();
    ui.statusDetailLabel->setText(
        QStringLiteral("当前连接：常山；已选择：默认。点击连接服务器后切换。"));
    ui.profileNameLabel->setText(QStringLiteral("非常长的配置名称用于检查窗口布局"));
    ui.profileDetailLabel->setText(QStringLiteral("EasyConnect · a-very-long-vpn-hostname.example.com:443"));
    window.layout()->activate();
    app.processEvents();

    if (ui.controlPanel->width() != originalWidth
        || ui.statusDetailLabel->height() < ui.statusDetailLabel->heightForWidth(ui.statusDetailLabel->width())
        || ui.profileNameLabel->height() < ui.profileNameLabel->heightForWidth(ui.profileNameLabel->width())
        || !ui.deleteProfileButton->isVisible())
    {
        qCritical() << "Profile text changed the panel width, clipped, or hid the delete button";
        return 1;
    }
    return 0;
}
