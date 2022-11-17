#ifndef GWIDI_INPUTSERVER_LINUXINPUTREADER_H
#define GWIDI_INPUTSERVER_LINUXINPUTREADER_H

#include <utility>
#include <vector>
#include <sys/poll.h>
#include <memory>
#include <atomic>
#include <thread>
#include <netinet/in.h>

namespace gwidi::input {

struct KeyEvent {
    int code;
    int eventType;  // 0 == release, 1 == pressed
};

class ReaderSocketClient {
public:
    ReaderSocketClient() = delete;
    explicit ReaderSocketClient(const sockaddr_in& toAddr);
    void sendKeyEvent(const KeyEvent& event);
private:
    int sockfd;
    struct sockaddr_in m_toAddr;
    char buffer[100];
};

class ReaderSocketServer {
public:
    void beginListening();
    void stopListening();

    inline bool isAlive() {
        return m_thAlive.load();
    }

    void sendKeyEvent(const KeyEvent &event);

    ~ReaderSocketServer();

private:
    std::atomic_bool m_thAlive{false};
    std::shared_ptr<std::thread> m_th;
    ReaderSocketClient* m_socketClient{nullptr};
};

class LinuxInputReader {
public:
    void beginListening();
    void stopListening();

    inline bool isAlive() {
        return m_thAlive.load();
    }

    inline void setWatchedKeys(std::vector<int> watchedKeys) {
        m_watchedKeys = std::move(watchedKeys);
    }

    ~LinuxInputReader();

private:
    void sendKeyEvent(int code, int eventType);
    void findInputDevices();
    bool keyWatched(int code);

    std::vector<pollfd> m_inputDevices;
    static int m_timeoutMs;

    std::atomic_bool m_thAlive{false};
    std::shared_ptr<std::thread> m_th;

    std::vector<int> m_watchedKeys;
    std::shared_ptr<ReaderSocketServer> m_socketServer{nullptr};
};

}
#endif //GWIDI_INPUTSERVER_LINUXINPUTREADER_H
