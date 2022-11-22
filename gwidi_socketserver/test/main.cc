#include "GwidiSocketServer.h"

int main() {

    auto server = gwidi::udpsocket::ReaderSocketServer();
    server.beginListening();

    // Server starts and waits for client connection (first message from client: 'msg_hello')
    // Server transfers udp packets to client after connection when told to (focus/key events)

    bool testSent = false;
    while(server.isAlive()) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        if(!testSent && server.isClientConnected()) {
            testSent = true;

            server.sendKeyEvent({
                16,
                1
            });

            server.sendWindowFocusEvent("Test Window", true);
        }
    }

    return 0;
}