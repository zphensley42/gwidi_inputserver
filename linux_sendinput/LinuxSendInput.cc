#include "LinuxSendInput.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <unordered_map>

std::unordered_map<std::string, int> SendInput::hk_map {
        {"0", KEY_0}, // octave up
        {"1", KEY_1},
        {"2", KEY_2},
        {"3", KEY_3},
        {"4", KEY_4},
        {"5", KEY_5},
        {"6", KEY_6},
        {"7", KEY_7},
        {"8", KEY_8},
        {"9", KEY_9}, // octave down
};

SendInput::SendInput() : SendInput(BUS_USB, 0x1234, 0x5678, "Gwidi Device") {
}

SendInput::SendInput(__u16 busType, __u16 vendor, __u16 product, const char* deviceName) {
    input_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    usetup.id.bustype = busType;
    usetup.id.vendor = vendor; /* sample vendor */
    usetup.id.product = product; /* sample product */
    strcpy(usetup.name, deviceName);    // max length of 80 (TODO: Add error handling)

    setupInputDevice();
}

SendInput::~SendInput() {
    teardownInputDevice();
}

void SendInput::sendInput(const std::string &key) {
    auto k = keyToHk(key);

    // down and report
    emit(input_fd, EV_KEY, k, 1);
    emit(input_fd, EV_SYN, SYN_REPORT, 0);

    //up and report
    emit(input_fd, EV_KEY, k, 0);
    emit(input_fd, EV_SYN, SYN_REPORT, 0);
}

int SendInput::keyToHk(const std::string& key) {
    auto it = hk_map.find(key);
    if(it == hk_map.end()) {
        return KEY_RESERVED;   // just some dummy for now
    }
    return it->second;
}

void SendInput::emit(int fd, int type, int code, int val) {
    struct input_event ie{};

    ie.type = type;
    ie.code = code;
    ie.value = val;
    /* timestamp values below are ignored */
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    write(fd, &ie, sizeof(ie));
}

void SendInput::setupInputDevice() {
    /*
     * The ioctls below will enable the device that is about to be
     * created, to pass key events, in this case all keys in the map
     */
    ioctl(input_fd, UI_SET_EVBIT, EV_KEY);
    for(auto &hk_entry : hk_map) {
        ioctl(input_fd, UI_SET_KEYBIT, hk_entry.second);
    }

    ioctl(input_fd, UI_DEV_SETUP, &usetup);
    ioctl(input_fd, UI_DEV_CREATE);

    // possibly pause if we need this device at the same time it is created
    // normally, this is not needed since we will be creating the device much earlier than when we need it
    sleep(1);
}

void SendInput::teardownInputDevice() {
    /*
     * Give userspace some time to read the events before we destroy the
     * device with UI_DEV_DESTROY.
     */
    sleep(1);

    ioctl(input_fd, UI_DEV_DESTROY);
    close(input_fd);
}
