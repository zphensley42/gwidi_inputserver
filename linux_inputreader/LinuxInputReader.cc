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

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#define MAXSTR 1000

namespace gwidi::input {


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
                        if(ev.type == EV_KEY && ev.code < 0x100 && ev.value >= 0 && ev.value <= 1) {
                            if(keyWatched(ev.code)) {
                                if((ev.value == 0 || ev.value == 1) && m_watchedKeyCb) {
                                    spdlog::info("sending key: {}, {}", ev.code, ev.value);
                                    m_watchedKeyCb(ev.code, ev.value);
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

bool LinuxInputReader::keyWatched(int code) {
    // There is a special case for when we have an empty list -- we just allow all keys
    if(m_watchedKeys.empty()) {
        return true;
    }
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


std::string InputFocusDetector::currentFocusedWindowName() {
    auto window = getWindowProperty(m_rootWindow, m_display, "_NET_ACTIVE_WINDOW");
    auto windowName = getStringProperty(window, m_display, "_NET_WM_NAME");
    auto ret = std::string{reinterpret_cast<char*>(windowName)};
    XFree(windowName);
    return ret;
}

unsigned long InputFocusDetector::getWindowProperty(unsigned long window, Display* display, const char *propName) {
    auto prop = getStringProperty(window, display, propName);
    unsigned long long_property = prop[0] + (prop[1]<<8) + (prop[2]<<16) + (prop[3]<<24);
    XFree(prop);
    return long_property;
}

unsigned char *InputFocusDetector::getStringProperty(unsigned long window, Display* display, const char* propName) {
    Atom actual_type, filter_atom;
    int actual_format, status;
    unsigned long nitems, bytes_after;

    unsigned char* prop;

    filter_atom = XInternAtom(display, propName, True);
    status = XGetWindowProperty(display, window, filter_atom, 0, MAXSTR, False, AnyPropertyType,
                                &actual_type, &actual_format, &nitems, &bytes_after, &prop);
    statusCheck(status, window);
    return prop;
}

void InputFocusDetector::statusCheck(int status, unsigned long window) {
    if (status == BadWindow) {
        spdlog::error("window id # {} does not exist!", window);
    }

    if (status != Success) {
        spdlog::error("XGetWindowProperty failed!");
    }
}

std::map<std::string, std::string> InputFocusDetector::windowStats() {
    auto window = getWindowProperty(m_rootWindow, m_display, "_NET_ACTIVE_WINDOW");

    auto windowClass = getStringProperty(window, m_display, "WM_CLASS");
    auto windowName = getStringProperty(window, m_display, "_NET_WM_NAME");
    auto pid = getWindowProperty(window, m_display, "_NET_WM_PID");

    std::map<std::string, std::string>  ret = {
            {"_NET_WM_PID", fmt::format("{}", pid)},
            {"WM_CLASS", std::string{reinterpret_cast<char*>(windowClass)}},
            {"_NET_WM_NAME", std::string{reinterpret_cast<char*>(windowName)}},
    };

    XFree(windowClass);
    XFree(windowName);

    return ret;
}

// https://stackoverflow.com/questions/57896007/detect-window-focus-changes-with-xcb
// https://stackoverflow.com/questions/27910906/xlib-test-window-names
InputFocusDetector::InputFocusDetector() {
    char* displayName = nullptr;

    m_display = XOpenDisplay(displayName);
    if (m_display == nullptr) {
        spdlog::error("Unable to open display: {}", XDisplayName(displayName));
    }
    int screen = XDefaultScreen(m_display);
    m_rootWindow = RootWindow(m_display, screen);

    getAllWindowsFromRoot(m_rootWindow);
}

// TODO: Register for focus events for a specific window instead of having to poll
// https://tronche.com/gui/x/xlib/event-handling/selecting.html
void InputFocusDetector::registerForWindowEvents() {
    if(m_selectedWindowName.empty()) {
        m_selectedWindowName = "gwidi_inputserver – LinuxInputReader.cc";
    }

    // First, ensure that we have a window
    auto it = m_windowTree.find(m_selectedWindowName);
    if(it != m_windowTree.end()) {
        m_selectedWindow = it->second;
        spdlog::info("Found window for name: {}", it->first);

        XSelectInput(m_display, it->second, FocusChangeMask);
        XEvent nextEvent;

        while(m_thAlive.load()) {
            auto status = XNextEvent(m_display, &nextEvent);
            if(nextEvent.type == FocusIn | nextEvent.type == FocusOut) {
                auto event = nextEvent.xfocus;
                auto isFocused = event.type == FocusIn;
                spdlog::info("FocusChangeMask event notify received, has focus: {}", isFocused);
                if(isFocused) {
                    if(m_gainFocusCb) {
                        m_gainFocusCb();
                    }
                }
                else if(m_loseFocusCb) {
                    m_loseFocusCb();
                }
            }
        }
    }
}

void InputFocusDetector::getAllWindowsFromRoot(Window root) {
    Window root_ret;
    Window parent_ret;
    Window *children_ret;
    unsigned int child_count;

    auto status = XQueryTree(m_display, root, &root_ret, &parent_ret, &children_ret, &child_count);
    for(auto i = 0; i < child_count; i++) {
        auto child = children_ret[i];
        auto childWinName = getStringProperty(child, m_display, "_NET_WM_NAME");
        auto wmClass = getStringProperty(child, m_display, "WM_CLASS");
        if(childWinName != nullptr) {
            auto winStr = std::string{reinterpret_cast<char*>(childWinName)};
            spdlog::info("child window name: {}", winStr);
            m_windowTree[winStr] = child;
        }

        if(wmClass != nullptr) {
            auto wmStr = std::string{reinterpret_cast<char*>(wmClass)};
            spdlog::info("child window class: {}", wmStr);
        }

        XFree(childWinName);
        XFree(wmClass);

        getAllWindowsFromRoot(child);
    }

    if(children_ret != nullptr) {
        XFree(children_ret);
    }
}

void InputFocusDetector::beginListening() {
    if(m_thAlive.load()) {
        return;
    }

    m_thAlive.store(true);

    // First, send a gain focus if we are already focused when we start
    auto curWindow = currentFocusedWindowName();
    if(curWindow == m_selectedWindowName) {
        if(m_gainFocusCb) {
            m_gainFocusCb();
        }
    }

    m_th = std::make_shared<std::thread>([this] {
        registerForWindowEvents();
    });
    m_th->detach();
}

void InputFocusDetector::stopListening() {
    m_thAlive.store(false);
}

InputFocusDetector::~InputFocusDetector() {
    if(m_thAlive.load() && m_th->joinable()) {
        m_thAlive.store(false);
        m_th->join();
    }
    else {
        m_thAlive.store(false);
    }
}

}