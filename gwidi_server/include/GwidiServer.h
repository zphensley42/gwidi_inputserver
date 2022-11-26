#ifndef GWIDI_INPUTSERVER_GWIDISERVER_H
#define GWIDI_INPUTSERVER_GWIDISERVER_H

#include <memory>
#include <functional>

#include "GwidiSocketServer.h"
#include "LinuxInputReader.h"

namespace gwidi::server {

using GainFocusCb = std::function<void()>;
using LoseFocusCb = std::function<void()>;
using WatchedKeyCb = std::function<void(int, int)>;

struct Configuration {
    GainFocusCb gainFocusCb;
    LoseFocusCb loseFocusCb;
    WatchedKeyCb watchedKeyCb;

    std::string watchedWindowName;
    std::vector<int> watchedKeys;
};

class GwidiServer {
public:
    GwidiServer() : GwidiServer(Configuration{}) {}
    explicit GwidiServer(Configuration cfg);
    ~GwidiServer();

    void start();
    void stop();

    bool isAlive();

    void setConfiguration(Configuration cfg);

    gwidi::udpsocket::ReaderSocketServer* socketServer();

private:
    std::unique_ptr<gwidi::udpsocket::ReaderSocketServer> m_socketServer;
    std::unique_ptr<gwidi::input::LinuxInputReader> m_inputReader;
    std::unique_ptr<gwidi::input::InputFocusDetector> m_focusDetector;

    Configuration m_configuration;
};

}

#endif //GWIDI_INPUTSERVER_GWIDISERVER_H
