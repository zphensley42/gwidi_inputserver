#include <csignal>
#include "LinuxInputReader.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <linux/input.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>

namespace gwidi::input {

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

    memset(buffer, '\0', sizeof(buffer));
    memcpy(buffer, &(event.code), sizeof(int));
    memcpy(buffer + sizeof(int), &(event.eventType), sizeof(int));

    auto bytesSent = sendto(sockfd, buffer, 100, 0, (struct sockaddr*)&m_toAddr, sizeof(m_toAddr));
    spdlog::info("Sent {} bytes of data", bytesSent);
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
        sockaddr_in selected_client{};
        bool client_selected = false;
        while(m_thAlive.load()) {
            memset(buffer, '\0', sizeof(buffer));
            recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*)&socketIn_client, &addr_size);  // Should loop this to continue receiving messages
            spdlog::info("Received message from client: {}, message: {}", ipForSin(socketIn_client), buffer);
            // TODO: Probably do some type of shared secret thing where we only accept clients we trust
            // TODO: For now, just look for a header message first to determine this is our client (assign only a single client at a time)

            auto helloMsgPre = "msg_hello";
            auto selectionMessageMatched = strncmp(helloMsgPre, buffer, strlen(helloMsgPre)) == 0;
            if(!client_selected && selectionMessageMatched) {
                client_selected = true;
                selected_client = socketIn_client;
                m_socketClient = new ReaderSocketClient(selected_client);
            }
            else {
                auto selected_client_ip = ipForSin(selected_client);
                auto in_client_ip = ipForSin(socketIn_client);
                spdlog::warn("Ignoring connection/message -- client_selected: {}, selected_client: {}, socketIn_client: {}", client_selected, selected_client_ip, in_client_ip);
            }
        }
        if(m_socketClient != nullptr) {
            delete m_socketClient;
            m_socketClient = nullptr;
        }

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


int LinuxInputReader::m_timeoutMs{5000};

bool supports_key_events(const int &fd) {
    unsigned long evbit = 0;
    ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);
    return (evbit & (1 << EV_KEY));
}

void LinuxInputReader::findInputDevices() {
    // Requires root
    auto uid = getuid();
    if(uid != 0) {
        spdlog::warn("User is not root, cross-process hotkeys may not function!");
    }

    char eventPathStart[] = "/dev/input/event";
    m_inputDevices.clear();
    for (auto &i : std::filesystem::directory_iterator("/dev/input")) {
        if(i.is_character_file()) {
            std::string view(i.path());
            if (view.compare(0, sizeof(eventPathStart)-1, eventPathStart) == 0) {
                int evfile = open(view.c_str(), O_RDONLY | O_NONBLOCK);
                if(supports_key_events(evfile)) {
                    spdlog::debug("Adding {}", i.path().c_str());
                    m_inputDevices.push_back({evfile, POLLIN, 0});
                } else {
                    close(evfile);
                }
            }
        }
    }
}

void LinuxInputReader::beginListening() {
    if(m_thAlive.load()) {
        return;
    }

    m_socketServer = std::make_shared<ReaderSocketServer>();
    m_socketServer->beginListening();

    m_thAlive.store(true);

    m_th = std::make_shared<std::thread>([this] {
        findInputDevices();

        // Requires root
        auto uid = getuid();
        if(uid != 0) {
            spdlog::warn("User is not root, cross-process hotkeys may not function!");
        }

        input_event ev{};
        while(m_thAlive.load()) {
            poll(m_inputDevices.data(), m_inputDevices.size(), m_timeoutMs);
            for (auto &pfd : m_inputDevices) {
                if (pfd.revents & POLLIN) {
                    if(read(pfd.fd, &ev, sizeof(ev)) == sizeof(ev)) {
                        // value: or 0 for EV_KEY for release, 1 for keypress and 2 for autorepeat
                        if(ev.type == EV_KEY && ev.code < 0x100) {
                            if(keyWatched(ev.code)) {
                                if(ev.value == 0) {
                                    spdlog::info("Key released: {}", ev.code);
                                    sendKeyEvent(ev.code, ev.value);
                                }
                                else if(ev.value == 1) {
                                    spdlog::info("Key pressed: {}", ev.code);
                                    sendKeyEvent(ev.code, ev.value);
                                }
                            }
                        }
                    }
                }
            }
        }

        for(auto &pfd : m_inputDevices) {
            close(pfd.fd);
        }
    });
    m_th->detach();
}

void LinuxInputReader::stopListening() {
    m_thAlive.store(false);
}

void LinuxInputReader::sendKeyEvent(int code, int eventType) {
    m_socketServer->sendKeyEvent(KeyEvent{code, eventType});
}

bool LinuxInputReader::keyWatched(int code) {
    return std::find(m_watchedKeys.begin(), m_watchedKeys.end(), code) != m_watchedKeys.end();
}

LinuxInputReader::~LinuxInputReader() {
    if(m_thAlive.load() && m_th->joinable()) {
        m_thAlive.store(false);
        m_th->join();
    }
    else {
        m_thAlive.store(false);
    }
}

}