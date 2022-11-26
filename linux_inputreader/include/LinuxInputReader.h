#ifndef GWIDI_INPUTSERVER_LINUXINPUTREADER_H
#define GWIDI_INPUTSERVER_LINUXINPUTREADER_H

#include "GwidiSocketServer.h"
#include <utility>
#include <vector>
#include <sys/poll.h>
#include <memory>
#include <atomic>
#include <thread>
#include <netinet/in.h>
#include <X11/Xlib.h>
#include <map>
#include <functional>

namespace gwidi::input {

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

    inline void setWatchedKeyCb(std::function<void(int, int)> cb) {
        m_watchedKeyCb = cb;
    }

    ~LinuxInputReader();

private:
    void findInputDevices();
    bool keyWatched(int code);

    std::vector<pollfd> m_inputDevices;
    static int m_timeoutMs;

    std::atomic_bool m_thAlive{false};
    std::shared_ptr<std::thread> m_th;

    std::vector<int> m_watchedKeys;
    std::function<void(int, int)> m_watchedKeyCb;
};

class InputFocusDetector {
public:
    InputFocusDetector();
    ~InputFocusDetector();

    std::map<std::string, std::string> windowStats();
    std::string currentFocusedWindowName();

    void beginListening();
    void stopListening();

    inline bool isAlive() {
        return m_thAlive.load();
    }

    inline void setGainFocusCb(std::function<void()> cb) {
        m_gainFocusCb = std::move(cb);
    }

    inline void setLoseFocusCb(std::function<void()> cb) {
        m_loseFocusCb = std::move(cb);
    }

    inline void setSelectedWindowName(const std::string& windowName) {
        m_selectedWindowName = windowName;
    }

private:
    Display *m_display{nullptr};
    Window m_rootWindow;

    std::map<std::string, Window> m_windowTree;
    void getAllWindowsFromRoot(Window root);

    void statusCheck(int status, unsigned long window);
    unsigned char* getStringProperty(unsigned long window, Display* display, char* propName);
    unsigned long getWindowProperty(unsigned long window, Display* display, char* propName);

    void registerForWindowEvents();

    std::atomic_bool m_thAlive{false};
    std::shared_ptr<std::thread> m_th;

    std::string m_selectedWindowName;
    Window m_selectedWindow;
    std::function<void()> m_gainFocusCb;
    std::function<void()> m_loseFocusCb;
};

}
#endif //GWIDI_INPUTSERVER_LINUXINPUTREADER_H
