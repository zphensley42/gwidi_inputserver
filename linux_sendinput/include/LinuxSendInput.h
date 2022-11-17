#ifndef EVDEV_TEST_LINUXSENDINPUT_H
#define EVDEV_TEST_LINUXSENDINPUT_H

#include <unordered_map>
#include <string>
#include <linux/uinput.h>

// see: https://www.kernel.org/doc/html/v5.11/input/uinput.html
class SendInput {
public:
    SendInput();
    SendInput(__u16 busType, __u16 vendor, __u16 product, const char* deviceName);
    ~SendInput();
    void sendInput(const std::string& key);
private:
    static std::unordered_map<std::string, int> hk_map;
    void emit(int fd, int type, int code, int val);
    static int keyToHk(const std::string& key);

    void setupInputDevice();
    void teardownInputDevice();

    int input_fd{-1};
    struct uinput_setup usetup{};
};

#endif //EVDEV_TEST_LINUXSENDINPUT_H
