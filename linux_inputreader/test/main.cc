#include "LinuxInputReader.h"
#include <linux/input-event-codes.h>

int main() {
    gwidi::input::LinuxInputReader server{};
    server.setWatchedKeys({KEY_Q});
    server.beginListening();
    while(server.isAlive()) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}