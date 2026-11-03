#pragma once

#include <QLabel>
#include <QSizePolicy>
#include <QWidget>

inline void reserveSidebarTextSpace(QLabel *status, QLabel *name, QLabel *details)
{
    status->setFixedHeight(status->fontMetrics().lineSpacing() * 3);
    status->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    name->setFixedHeight(name->fontMetrics().lineSpacing());
    details->setFixedHeight(details->fontMetrics().lineSpacing());
}

inline void reserveProxyButtonSpace(QWidget *button)
{
    QSizePolicy policy = button->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    button->setSizePolicy(policy);
}
