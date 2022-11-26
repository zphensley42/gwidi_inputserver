#ifndef GWIDI_INPUTSERVER_GWIDISOCKETSERVER_H
#define GWIDI_INPUTSERVER_GWIDISOCKETSERVER_H

#include <netinet/in.h>
#include <atomic>
#include <thread>
#include <memory>
#include <functional>
#include <string>

namespace gwidi::udpsocket {

enum ServerEventType {
    EVENT_HELLO = 0,
    EVENT_KEY = 1,
    EVENT_FOCUS = 2,
    EVENT_WATCHEDKEYS_RECONFIGURE = 3
};

struct KeyEvent {
    int code;
    int eventType;  // 0 == release, 1 == pressed
};

struct WindowFocusEvent {
    std::size_t windowNameSize;
    const char* windowName;
    bool hasFocus;
};

struct WatchedKeysReconfigEvent {
    std::size_t watchedKeysSize;
    int* watchedKeysList;
};

union ServerEvent {
    KeyEvent keyEvent;
    WindowFocusEvent focusEvent;
    WatchedKeysReconfigEvent watchedKeysReconfigEvent;
};

struct EventBuffer {
    char* buffer;
    std::size_t bufferSize;
};

class EventBuilder {
public:
    EventBuilder() = delete;

    static EventBuilder eventFor(ServerEventType type);
    EventBuilder& withKeyCode(int code);
    EventBuilder& withKeyEventType(int eventType);
    EventBuilder& withFocusWindowName(const std::string &windowName);
    EventBuilder& withFocusHasFocus(bool hasFocus);

    [[nodiscard]] EventBuffer build() const;

private:
    explicit EventBuilder(ServerEventType type);

    ServerEvent m_serverEvent;
    ServerEventType m_type;
};

class ReaderSocketClient {
public:
    ReaderSocketClient() = delete;
    explicit ReaderSocketClient(const sockaddr_in& toAddr);
    void sendKeyEvent(const KeyEvent& event);
    void sendWindowFocusEvent(const std::string &windowName, bool hasFocus);
    void sendHello();
private:
    int sockfd;
    struct sockaddr_in m_toAddr;
};

class ReaderSocketServer {
public:
    using EventCb = std::function<void(ServerEventType, ServerEvent)>;

    void beginListening();
    void stopListening();

    inline bool isAlive() {
        return m_thAlive.load();
    }

    inline bool isClientConnected() {
        return m_socketClient != nullptr;
    }

    inline void setEventCb(EventCb cb) {
        m_eventCb = std::move(cb);
    }

    void processEvent(char* buffer, struct sockaddr_in socketIn_client);
    void sendKeyEvent(const KeyEvent &event);
    void sendWindowFocusEvent(const std::string &windowName, bool hasFocus);

    ~ReaderSocketServer();

private:
    EventCb m_eventCb;

    std::atomic_bool m_thAlive{false};
    std::shared_ptr<std::thread> m_th;
    ReaderSocketClient* m_socketClient{nullptr};
};

}

#endif //GWIDI_INPUTSERVER_GWIDISOCKETSERVER_H
