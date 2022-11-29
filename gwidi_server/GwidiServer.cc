#include "GwidiServer.h"

#include <utility>

namespace gwidi::server {

GwidiServer::GwidiServer(Configuration cfg) : m_configuration{std::move(cfg)} {
}

GwidiServer::~GwidiServer() {
    stop();
}

void GwidiServer::start() {
    m_socketServer = std::make_unique<gwidi::udpsocket::ReaderSocketServer>();
    m_socketServer->setEventCb([this](gwidi::udpsocket::ServerEventType type, gwidi::udpsocket::ServerEvent event){
        if(type == udpsocket::ServerEventType::EVENT_WATCHEDKEYS_RECONFIGURE) {
            std::vector<int> watchedKeys;
            for(auto i = 0; i < event.watchedKeysReconfigEvent.watchedKeysSize; i++) {
                int key = event.watchedKeysReconfigEvent.watchedKeysList[i];
                watchedKeys.emplace_back(key);
            }
            m_inputReader->setWatchedKeys(watchedKeys);

            delete[] event.watchedKeysReconfigEvent.watchedKeysList;    // free the list we were sent
        }
    });
    m_socketServer->beginListening();

    m_inputReader = std::make_unique<gwidi::input::LinuxInputReader>();
    m_inputReader->setWatchedKeyCb(m_configuration.watchedKeyCb);
    m_inputReader->setWatchedKeys(m_configuration.watchedKeys);
    m_inputReader->beginListening();

    m_focusDetector = std::make_unique<gwidi::input::InputFocusDetector>();
    m_focusDetector->setSelectedWindowName(m_configuration.watchedWindowName);
    m_focusDetector->setGainFocusCb(m_configuration.gainFocusCb);
    m_focusDetector->setLoseFocusCb(m_configuration.loseFocusCb);
    m_focusDetector->beginListening();
}

void GwidiServer::stop() {
    m_socketServer->stopListening();
    m_inputReader->stopListening();
    m_focusDetector->stopListening();
}

void GwidiServer::setConfiguration(Configuration cfg) {
    m_configuration = std::move(cfg);

    if(m_inputReader) {
        m_inputReader->setWatchedKeyCb(m_configuration.watchedKeyCb);
        m_inputReader->setWatchedKeys(m_configuration.watchedKeys);
    }

    if(m_focusDetector) {
        m_focusDetector->setSelectedWindowName(m_configuration.watchedWindowName);
        m_focusDetector->setGainFocusCb(m_configuration.gainFocusCb);
        m_focusDetector->setLoseFocusCb(m_configuration.loseFocusCb);
    }
}

bool GwidiServer::isAlive() {
    if(m_socketServer) {
        return m_socketServer->isAlive();
    }

    if(m_inputReader) {
        return m_inputReader->isAlive();
    }

    if(m_focusDetector) {
        return m_focusDetector->isAlive();
    }

    return false;
}

gwidi::udpsocket::ReaderSocketServer *GwidiServer::socketServer() {
    return m_socketServer.get();
}

}