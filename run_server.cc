#include "GwidiServer.h"
#include <linux/input-event-codes.h>
#include <spdlog/spdlog.h>

int main() {
    std::unique_ptr<gwidi::server::GwidiServer> gwidiServer;
    std::string windowName = "Guild Wars 2";

    gwidi::server::Configuration cfg {
        [&gwidiServer, &windowName](){
            spdlog::info("Focus gained!");
            auto socketServer = gwidiServer->socketServer();
            if(socketServer) {
                socketServer->sendWindowFocusEvent(windowName, true);
            }
        },
        [&gwidiServer, &windowName](){
            spdlog::info("Focus lost!");
            auto socketServer = gwidiServer->socketServer();
            if(socketServer) {
                socketServer->sendWindowFocusEvent(windowName, false);
            }
        },
        [&gwidiServer](int code, int type){
            spdlog::info("Key {}, type: {} detected", code, type);
            auto socketServer = gwidiServer->socketServer();
            if(socketServer) {
                socketServer->sendKeyEvent(gwidi::udpsocket::KeyEvent{code, type});
            }
        },
        windowName,
        {KEY_Q} // TODO: This needs to be something passed when the server is started
        // TODO: We won't always know when starting the server, build a message from client -> server that reconfigures the watched keys
    };

    gwidiServer = std::make_unique<gwidi::server::GwidiServer>(cfg);
    gwidiServer->start();

    while(gwidiServer->isAlive()) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
