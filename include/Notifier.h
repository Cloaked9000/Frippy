//
// Created by fred.nicolson on 13/11/2019.
//

#ifndef CLIPUPLOAD_NOTIFIER_H
#define CLIPUPLOAD_NOTIFIER_H
#include <string>

class Notifier
{
public:

    /*!
     * Creates a desktop notification
     *
     * @param title Notification title
     * @param message Notification message
     * @param timeout Number of seconds to display for
     */
    static void notify(const std::string &title, const std::string &message, const std::chrono::seconds &timeout = std::chrono::seconds(0));

};


#endif //CLIPUPLOAD_NOTIFIER_H
