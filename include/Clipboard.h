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

struct _XDisplay;
class Clipboard
{
public:
    struct ClipboardRead
    {
        std::string data; //raw clipboard data
        std::string type; //mime type of data
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

    /*!
     * Reads the current clipboard then returns its mimetype/data
     *
     * @return The clipboard
     */
    ClipboardRead read_clipboard();

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
