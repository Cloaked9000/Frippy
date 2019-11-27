//
// Created by fred.nicolson on 13/11/2019.
//

#include <libnotifymm.h>
#include "Notifier.h"

void Notifier::notify(const std::string &title, const std::string &message, const std::chrono::seconds &timeout)
{
    auto init = []() {
        return Notify::init("Frippy");
    };
    static bool ready = init();
    if(!ready)
        throw std::runtime_error("Failed to initialise libnotify!");

    Notify::Notification notification(title, message, "dialog-information");
    if(timeout.count())
        notification.set_timeout(timeout.count());
    notification.show();
}
