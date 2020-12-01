//
// Created by fred.nicolson on 12/11/2019.
//

#ifndef CLIPUPLOAD_CLIPBOARD_H
#define CLIPUPLOAD_CLIPBOARD_H

#include <unordered_map>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

struct _XDisplay;
class Clipboard
{
public:

    struct Target
    {
        std::string name;
        unsigned long atom;
    };

    /*!
     * Constructor
     *
     * @param xa_priority Priority list when picking a suitable conversion type
     */
    explicit Clipboard(std::vector<std::pair<std::string, std::string>> xa_priority);
    ~Clipboard();
    Clipboard(const Clipboard&)=delete;
    Clipboard(Clipboard&&)=delete;
    void operator=(Clipboard&&)=delete;
    void operator=(const Clipboard&&)=delete;

    bool read_clipboard(const Target &target, const std::function<bool(const std::string &data)> &handler);

    std::vector<Target> list_available_conversions();

private:

    //Clipboard monitoring state
    unsigned long clipboard_atom;
    unsigned long xa_targets_atom;
    unsigned long our_window;
    unsigned long clipboard_type_atom;
    unsigned long incr_atom;
    unsigned long root_window;
    _XDisplay *display;
    std::vector<std::pair<std::string, std::string>> xa_priority;
};


#endif //CLIPUPLOAD_CLIPBOARD_H
