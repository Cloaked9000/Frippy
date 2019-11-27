//
// Created by fred.nicolson on 25/11/2019.
//

#ifndef CLIPUPLOAD_KEYBOARD_H
#define CLIPUPLOAD_KEYBOARD_H


#include <cstdint>

class Keyboard
{
public:

    //List of possible modifiers
    struct KeyModifier
    {
        uint8_t ctrl_pressed : 1;
        uint8_t shift_pressed : 1;
        uint8_t key_pressed : 1;
        uint8_t key_released : 1;
    };

    Keyboard();
    ~Keyboard();

    /*!
     * Starts listening for changes in a given key
     *
     * @param key The key to bind to
     * @param modifier The modifiers to listen for in association with this key
     */
    void bind_key(const std::string &key, KeyModifier modifier);

    /*!
     * Stops listening for changes in a given key
     *
     * @param key The key to unbind
     * @param modifier The exact same modifiers passed along to bind_key for this key
     */
    void unbind_key(const std::string &key, KeyModifier modifier);

    /*!
     * Blocks until one of the bound keys has activity
     *
     * @param key Set to the key which has changed
     * @param modifier Set to the modifiers which are in effect
     */
    void wait_for_keys(std::string &key, Keyboard::KeyModifier &modifier);
private:

    /*!
     * Converts a KeyModifier struct into an x11 key mask
     *
     * @param modifier Modifier to convert
     * @return x11 key mask
     */
    static unsigned int modifier_to_mask(KeyModifier modifier);

    /*!
     * Converts an x11 key mask to a KeyModifier struct
     *
     * @param mask Mask to convert
     * @return KeyModifier enum
     */
    static KeyModifier mask_to_modifier(unsigned int mask);

    //X11 connection state for key monitoring
    _XDisplay *display;
    unsigned long root_window;
};


#endif //CLIPUPLOAD_KEYBOARD_H
