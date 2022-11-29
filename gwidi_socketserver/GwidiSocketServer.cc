#include <arpa/inet.h>
#include <cstring>
#include <spdlog/spdlog.h>
#include "GwidiSocketServer.h"

namespace gwidi::udpsocket {

std::string ipForSin(const sockaddr_in& sin) {
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sin.sin_addr), str, INET_ADDRSTRLEN);
    return {str};
}

ReaderSocketClient::ReaderSocketClient(const sockaddr_in &toAddr) {
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    memset(&m_toAddr, '\0', sizeof(m_toAddr));
    m_toAddr.sin_family = AF_INET;
    m_toAddr.sin_port = htons(5578);
    m_toAddr.sin_addr.s_addr = toAddr.sin_addr.s_addr;
}

void ReaderSocketClient::sendKeyEvent(const KeyEvent &event) {
    auto toIp = ipForSin(m_toAddr);
    spdlog::info("Sending KeyEvent[ code: {}, eventType: {} ] to client: {}, port: {}", event.code, event.eventType, toIp,
                 ntohs(m_toAddr.sin_port));

    auto eventBuffer = EventBuilder::eventFor(ServerEventType::EVENT_KEY)
            .withKeyCode(event.code)
            .withKeyEventType(event.eventType)
            .build();

    auto bytesSent = sendto(sockfd, eventBuffer.buffer, eventBuffer.bufferSize, 0, (struct sockaddr*)&m_toAddr, sizeof(m_toAddr));
    spdlog::info("Sent {} bytes of data", bytesSent);
}

void ReaderSocketClient::sendWindowFocusEvent(const std::string &windowName, bool hasFocus) {
    auto toIp = ipForSin(m_toAddr);
    spdlog::info("Sending FocusEvent[ windowName: {}, hasFocus: {} ] to client: {}, port: {}", windowName, hasFocus, toIp,
                 ntohs(m_toAddr.sin_port));

    auto eventBuffer = EventBuilder::eventFor(ServerEventType::EVENT_FOCUS)
            .withFocusWindowName(windowName)
            .withFocusHasFocus(hasFocus)
            .build();

    auto bytesSent = sendto(sockfd, eventBuffer.buffer, eventBuffer.bufferSize, 0, (struct sockaddr*)&m_toAddr, sizeof(m_toAddr));
    spdlog::info("Sent {} bytes of data", bytesSent);
}

void ReaderSocketClient::sendHello() {
    auto toIp = ipForSin(m_toAddr);

    char* buffer = new char[1024];
    memset(buffer, '\0', sizeof(char) * 1024);

    std::size_t bufferOffset = 0;

    std::string msg = "msg_helloback";
    std::size_t msg_size = msg.size();
    int msg_type = static_cast<int>(ServerEventType::EVENT_HELLO);

    memcpy(buffer + bufferOffset, &(msg_type), sizeof(int));
    bufferOffset += sizeof(int);

    memcpy(buffer + bufferOffset, &(msg_size), sizeof(msg_size));
    bufferOffset += sizeof(msg_size);

    memcpy(buffer + bufferOffset, &(msg[0]), sizeof(char) * msg_size);
    bufferOffset += sizeof(char) * msg_size;

    auto bytesSent = sendto(sockfd, buffer, 1024, 0, (struct sockaddr*)&m_toAddr, sizeof(m_toAddr));
    spdlog::info("Sent {} bytes of data", bytesSent);

    delete[] buffer;
}

void ReaderSocketServer::beginListening() {
    if(m_thAlive.load()) {
        return;
    }

    m_thAlive.store(true);

    m_th = std::make_shared<std::thread>([this] {
        // For now, port is here
        int port = 5577;
        int sockfd;
        struct sockaddr_in socketIn_server, socketIn_client;
        char buffer[1024];  // probably too big for our needs
        socklen_t addr_size;

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        memset(&socketIn_server, '\0', sizeof(socketIn_server));
        socketIn_server.sin_family = AF_INET;
        socketIn_server.sin_port = htons(port);
        socketIn_server.sin_addr.s_addr = inet_addr("127.0.0.1");   // local listening server

        auto bindStatus = bind(sockfd, (struct sockaddr*)&socketIn_server, sizeof(socketIn_server));
        if(bindStatus != 0) {
            spdlog::warn("Failed to bind listening socket!");
            m_thAlive.store(false);
            return;
        }

        addr_size = sizeof(socketIn_client);
        while(m_thAlive.load()) {
            memset(buffer, '\0', sizeof(buffer));
            recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*)&socketIn_client, &addr_size);  // Should loop this to continue receiving messages
            spdlog::info("Received message from client: {}, message: {}", ipForSin(socketIn_client), buffer);
            // TODO: Probably do some type of shared secret thing where we only accept clients we trust
            // TODO: For now, just look for a header message first to determine this is our client (assign only a single client at a time)

            processEvent(buffer, socketIn_client);
        }
        m_socketClient = nullptr;

        m_thAlive.store(false);
    });
    m_th->detach();
}

void ReaderSocketServer::stopListening() {
    m_thAlive.store(false);
}

ReaderSocketServer::~ReaderSocketServer() {
    if(m_thAlive.load() && m_th->joinable()) {
        m_thAlive.store(false);
        m_th->join();
    }
    else {
        m_thAlive.store(false);
    }
}

void ReaderSocketServer::sendKeyEvent(const KeyEvent &event) {
    if(m_socketClient) {
        m_socketClient->sendKeyEvent(event);
    }
}

void ReaderSocketServer::sendWindowFocusEvent(const std::string &windowName, bool hasFocus) {
    if(m_socketClient) {
        m_socketClient->sendWindowFocusEvent(windowName, hasFocus);
    }
}

void ReaderSocketServer::processEvent(char *buffer, struct sockaddr_in socketIn_client) {
    // The message we expect to receive is of the format: [{msg_type}{size}{message}]
    std::size_t bufferOffset = 0;
    int msg_type;
    memcpy(&msg_type, buffer, sizeof(int));
    bufferOffset += sizeof(int);

    switch(static_cast<ServerEventType>(msg_type)) {
        case ServerEventType::EVENT_HELLO: {
            std::size_t msgSize;
            memcpy(&msgSize, buffer + bufferOffset, sizeof(std::size_t));
            bufferOffset += sizeof(std::size_t);

            // We probably shouldn't cap here, but this is trying to catch cases where junk data still matches our event type
            if(msgSize >= 1024) {
                msgSize = 1024;
            }
            char* msg = new char[msgSize];
            memcpy(&msg[0], buffer + bufferOffset, msgSize);
            bufferOffset += msgSize;

            // verify our hello string
            auto helloMsgPre = "msg_hello";
            auto selectionMessageMatched = strncmp(helloMsgPre, msg, strlen(helloMsgPre)) == 0;

            // Always accept a new client in case we dc or restart the client (we only accept one client at a time, currently)
            if(selectionMessageMatched) {
                m_socketClient = std::make_shared<ReaderSocketClient>(socketIn_client);
                m_socketClient->sendHello();
            }

            delete[] msg;

            break;
        }
        case ServerEventType::EVENT_WATCHEDKEYS_RECONFIGURE: {
            // Parse out the data
            std::size_t listSize;
            memcpy(&listSize, buffer + bufferOffset, sizeof(std::size_t));
            bufferOffset += sizeof(std::size_t);

            // TODO: Want to make sure we delete this list of keys after use
            int* keys = new int[listSize];
            for(auto i = 0; i < listSize; i++) {
                int key;
                memcpy(&key, buffer + bufferOffset, sizeof(int));
                keys[i] = key;
                bufferOffset += sizeof(int);
            }
            // Pass the data to the input reader
            if(m_eventCb) {
                m_eventCb(ServerEventType::EVENT_WATCHEDKEYS_RECONFIGURE, {.watchedKeysReconfigEvent{listSize, keys}});
            }
            break;
        }
        default: {
            spdlog::warn("Message type not supported, message: {}", buffer);
            break;
        }
    }
}


EventBuilder::EventBuilder(gwidi::udpsocket::ServerEventType type) : m_type{type} {
    switch(type) {
        case ServerEventType::EVENT_KEY: {
            m_serverEvent = { .keyEvent{} };
            break;
        }
        case ServerEventType::EVENT_FOCUS: {
            m_serverEvent = { .focusEvent{} };
            break;
        }
    }
}

EventBuilder EventBuilder::eventFor(ServerEventType type) {
    return EventBuilder(type);
}

EventBuilder &EventBuilder::withKeyCode(int code) {
    if(m_type == ServerEventType::EVENT_KEY) {
        m_serverEvent.keyEvent.code = code;
    }
    return *this;
}

EventBuilder &EventBuilder::withKeyEventType(int eventType) {
    if(m_type == ServerEventType::EVENT_KEY) {
        m_serverEvent.keyEvent.eventType = eventType;
    }
    return *this;
}

EventBuilder &EventBuilder::withFocusWindowName(const std::string &windowName) {
    if(m_type == ServerEventType::EVENT_FOCUS) {
        m_serverEvent.focusEvent.windowNameSize = windowName.size();
        m_serverEvent.focusEvent.windowName = windowName.c_str();
    }
    return *this;
}

EventBuilder &EventBuilder::withFocusHasFocus(bool hasFocus) {
    if(m_type == ServerEventType::EVENT_FOCUS) {
        m_serverEvent.focusEvent.hasFocus = hasFocus;
    }
    return *this;
}

EventBuffer EventBuilder::build() const {
    EventBuffer ret {
        new char[1024],
        1024
    };

    memset(ret.buffer, '\0', sizeof(char) * ret.bufferSize);

    std::size_t bufferOffset = 0;

    // Write our event type
    int eventType = static_cast<int>(m_type);
    memcpy(ret.buffer, &eventType, sizeof(int));
    bufferOffset += sizeof(int);

    // Write each event
    switch(m_type) {
        case ServerEventType::EVENT_KEY: {
            memcpy(ret.buffer + bufferOffset, &(m_serverEvent.keyEvent.code), sizeof(int));
            bufferOffset += sizeof(int);

            memcpy(ret.buffer + bufferOffset, &(m_serverEvent.keyEvent.eventType), sizeof(int));
            bufferOffset += sizeof(int);
            break;
        }
        case ServerEventType::EVENT_FOCUS: {
            memcpy(ret.buffer + bufferOffset, &(m_serverEvent.focusEvent.windowNameSize), sizeof(m_serverEvent.focusEvent.windowNameSize));
            bufferOffset += sizeof(m_serverEvent.focusEvent.windowNameSize);

            memcpy(ret.buffer + bufferOffset, &(m_serverEvent.focusEvent.windowName[0]), sizeof(char) * m_serverEvent.focusEvent.windowNameSize);
            bufferOffset += sizeof(char) * m_serverEvent.focusEvent.windowNameSize;

            memcpy(ret.buffer + bufferOffset, &(m_serverEvent.focusEvent.hasFocus), sizeof(bool));
            bufferOffset += sizeof(bool);
            break;
        }
    }

    return ret;
}

}