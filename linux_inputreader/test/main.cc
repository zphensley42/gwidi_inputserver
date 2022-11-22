#include "LinuxInputReader.h"
#include <linux/input-event-codes.h>
#include <spdlog/spdlog.h>

int main() {

    std::string windowName = "gwidi_inputserver – main.cc";

    gwidi::udpsocket::ReaderSocketServer socketServer{};
    socketServer.beginListening();

    gwidi::input::InputFocusDetector focusDetector{};
    focusDetector.setGainFocusCb([&windowName, &socketServer](){
        spdlog::info("Window gained focus!");
        socketServer.sendWindowFocusEvent(windowName, true);
    });

    focusDetector.setLoseFocusCb([&windowName, &socketServer](){
        spdlog::info("Window lost focus!");
        socketServer.sendWindowFocusEvent(windowName, false);
    });

    focusDetector.setSelectedWindowName(windowName);
    focusDetector.beginListening();

    gwidi::input::LinuxInputReader server{};
    server.setWatchedKeys({KEY_Q});
    server.setWatchedKeyCb([&socketServer](int code, int value){
        socketServer.sendKeyEvent(gwidi::udpsocket::KeyEvent{code, value});

    });
    server.beginListening();

    while(server.isAlive()) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}