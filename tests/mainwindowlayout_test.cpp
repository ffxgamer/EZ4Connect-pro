#include <QApplication>
#include <QDebug>
#include <QMainWindow>

#include "ui_mainwindow.h"
#include "presentation/main/sidebarlayout.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    reserveSidebarTextSpace(ui.statusDetailLabel, ui.profileNameLabel, ui.profileDetailLabel);
    window.resize(840, 650);
    window.show();
    app.processEvents();

    const int originalWidth = ui.controlPanel->width();
    const int originalButtonY = ui.pushButton1->y();
    const int originalListY = ui.profileList->y();
    ui.statusDetailLabel->setText(
        QStringLiteral("当前连接：常山；已选择：默认。点击连接服务器后切换。"));
    ui.profileNameLabel->setText(QStringLiteral("常山"));
    ui.profileDetailLabel->setText(QStringLiteral("EasyConnect · 127.0.0.1:443"));
    window.layout()->activate();
    app.processEvents();

    if (ui.controlPanel->width() != originalWidth
        || ui.statusDetailLabel->height() < ui.statusDetailLabel->heightForWidth(ui.statusDetailLabel->width())
        || ui.pushButton1->y() != originalButtonY || ui.profileList->y() != originalListY
        || !ui.deleteProfileButton->isVisible())
    {
        qCritical() << "Profile text changed the panel width, clipped, or hid the delete button";
        return 1;
    }
    return 0;
}
