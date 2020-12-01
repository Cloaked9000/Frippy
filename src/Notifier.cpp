//
// Created by fred.nicolson on 13/11/2019.
//

#include <libnotifymm.h>
#include <iostream>
#include "Notifier.h"
#define MS_PER_SEC 1000

void Notifier::notify(const std::string &title, const std::string &message, const std::chrono::seconds &timeout)
{
    std::cout << "[" << title << "]: " << message << std::endl;
    auto init = []() {
        return Notify::init("Frippy");
    };
    static bool ready = init();
    if(!ready)
    {
        throw std::runtime_error("Failed to initialise libnotify!");
    }

    Notify::Notification notification(title, message, "dialog-information");
    if(timeout.count())
    {
        notification.set_timeout((int)timeout.count() * MS_PER_SEC);
    }
    notification.set_category("transfer.complete");
    notification.set_urgency(Notify::URGENCY_CRITICAL);
    notification.show();
}
