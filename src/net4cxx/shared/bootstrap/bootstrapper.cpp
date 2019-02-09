//
// Created by yuwenyong.vincent on 2018/10/5.
//

#include "net4cxx/shared/bootstrap/bootstrapper.h"
#include "net4cxx/shared/global/constants.h"
#include "net4cxx/common/configuration/options.h"
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/common/debugging/crashreport.h"
#include "net4cxx/common/debugging/watcher.h"
#include "net4cxx/common/logging/logging.h"
#include "net4cxx/common/utilities/objectmanager.h"
#include "net4cxx/core/network/reactor.h"
#include "net4cxx/shared/global/constants.h"
#include "net4cxx/shared/global/loggers.h"


NS_BEGIN

Bootstrapper* Bootstrapper::_instance = nullptr;

Bootstrapper::Bootstrapper(bool commandThreadEnabled)
        : _commandThreadEnabled(commandThreadEnabled) {
    if (_instance) {
        NET4CXX_THROW_EXCEPTION(AlreadyExist, "Bootstrapper object already exists");
    }
    _instance = this;
}

Bootstrapper::Bootstrapper(size_t numThreads, bool commandThreadEnabled)
    : _commandThreadEnabled(commandThreadEnabled) {
    if (_instance) {
        NET4CXX_THROW_EXCEPTION(AlreadyExist, "Bootstrapper object already exists");
    }
    _instance = this;
    _reactor = std::make_unique<Reactor>(numThreads);
#if (PLATFORM == PLATFORM_WINDOWS) && (COMPILER == COMPILER_GNU)
    _commandThreadEnabled = true;
#endif
}

Bootstrapper::~Bootstrapper() {
    cleanup();
}

void Bootstrapper::run(const boost::function1<std::string, std::string> &name_mapper) {
    doPreInit();
    NET4CXX_Options->parseEnvironment(name_mapper);
    doInit();
    doRun();
}

void Bootstrapper::run(int argc, const char *const *argv) {
    doPreInit();
    NET4CXX_Options->parseCommandLine(argc, argv);
    doInit();
    doRun();
}

void Bootstrapper::run(const char *path) {
    doPreInit();
    NET4CXX_Options->parseConfigFile(path);
    doInit();
    doRun();
}

void Bootstrapper::setCrashReportPath(const std::string &crashReportPath) {
    CrashReport::setCrashReportPath(crashReportPath);
}

void Bootstrapper::onPreInit() {

}

void Bootstrapper::onInit() {

}

void Bootstrapper::onRun() {

}

void Bootstrapper::onQuit() {

}

bool Bootstrapper::onSysCommand(const std::string &command) {
    if (boost::iequals(command, "quit")) {
        if (_reactor) {
            NET4CXX_LOG_INFO(gGenLog, "Command quit, shutting down.");
            _reactor->stop();
        }
        return true;
    }
    return false;
}

bool Bootstrapper::onUserCommand(const std::string &command) {
    return false;
}

void Bootstrapper::doPreInit() {
    NET4CXX_ASSERT(!_inited);
    onPreInit();
    CrashReport::outputCrashReport();
}

void Bootstrapper::doInit() {
    onInit();
    if (_reactor) {
        _reactor->makeCurrent();
    }
    _inited = true;
}

void Bootstrapper::doRun() {
    std::shared_ptr<std::thread> commandThread;
    if (_commandThreadEnabled) {
        commandThread.reset(new std::thread([this](){
            this->commandThread();
        }), &shutdownCommandThread);
    }
    onRun();
    if (_reactor) {
        _reactor->run();
    }
    _commandThreadStopped = true;
    onQuit();
    commandThread = nullptr;
    if (_reactor) {
        Reactor::clearCurrent();
        _reactor.reset();
    }
}

void Bootstrapper::commandThread() {
    char *commandStr;
    char commandBuf[256];
    std::string command;

    printf("Command>");
    while (!_commandThreadStopped) {
        fflush(stdout);
#if PLATFORM != PLATFORM_WINDOWS
        while (!kbHitReturn() && !_commandThreadStopped) {
            usleep(100);
        }
        if (_commandThreadStopped) {
           break;
        }
#endif
        commandStr = fgets(commandBuf, sizeof(commandBuf), stdin);
        if (commandStr != NULL) {
            for (int x = 0; commandStr[x]; ++x) {
                if (commandStr[x] == '\r' || commandStr[x] == '\n') {
                    commandStr[x] = 0;
                    break;
                }
            }
            command = commandStr;
            boost::trim(command);
            if (!command.empty()) {
                if (_reactor) {
                    _reactor->addCallback([this, command=std::move(command)](){
                        onCommand(command);
                    });
                } else {
                    onCommand(command);
                }
            }
            printf("Command>");
        } else if (feof(stdin)){
            if (_reactor) {
                _reactor->addCallback([this](){
                    _reactor->stop();
                });
            }
            break;
        }
    }
}

#if PLATFORM != PLATFORM_WINDOWS
int Bootstrapper::kbHitReturn() {
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}
#endif

void Bootstrapper::shutdownCommandThread(std::thread *commandThread) {
    if (commandThread != nullptr) {
#if PLATFORM == PLATFORM_WINDOWS
        if (!CancelSynchronousIo((HANDLE)commandThread->native_handle())) {
            DWORD errorCode = GetLastError();
            if (errorCode != ERROR_NOT_FOUND) {
                LPSTR errorBuffer;
                DWORD numCharsWritten = FormatMessage(
                        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                        nullptr, errorCode, 0, (LPTSTR)&errorBuffer, 0, nullptr);
                if (!numCharsWritten) {
                    NET4CXX_LOG_ERROR(gGenLog, "Error cancelling I/O of CommandThread, error code %u, detail: Unknown error",
                                      errorCode);
                } else {
                    NET4CXX_LOG_ERROR(gGenLog, "Error cancelling I/O of CommandThread, error code %u, detail: %s",
                                      errorCode, errorBuffer);
                    LocalFree(errorBuffer);
                }

                INPUT_RECORD b[4];
                HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
                b[0].EventType = KEY_EVENT;
                b[0].Event.KeyEvent.bKeyDown = TRUE;
                b[0].Event.KeyEvent.uChar.AsciiChar = 'X';
                b[0].Event.KeyEvent.wVirtualKeyCode = 'X';
                b[0].Event.KeyEvent.wRepeatCount = 1;

                b[1].EventType = KEY_EVENT;
                b[1].Event.KeyEvent.bKeyDown = FALSE;
                b[1].Event.KeyEvent.uChar.AsciiChar = 'X';
                b[1].Event.KeyEvent.wVirtualKeyCode = 'X';
                b[1].Event.KeyEvent.wRepeatCount = 1;

                b[2].EventType = KEY_EVENT;
                b[2].Event.KeyEvent.bKeyDown = TRUE;
                b[2].Event.KeyEvent.dwControlKeyState = 0;
                b[2].Event.KeyEvent.uChar.AsciiChar = '\r';
                b[2].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
                b[2].Event.KeyEvent.wRepeatCount = 1;
                b[2].Event.KeyEvent.wVirtualScanCode = 0x1c;

                b[3].EventType = KEY_EVENT;
                b[3].Event.KeyEvent.bKeyDown = FALSE;
                b[3].Event.KeyEvent.dwControlKeyState = 0;
                b[3].Event.KeyEvent.uChar.AsciiChar = '\r';
                b[3].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
                b[3].Event.KeyEvent.wVirtualScanCode = 0x1c;
                b[3].Event.KeyEvent.wRepeatCount = 1;
                DWORD numb;
                WriteConsoleInput(hStdIn, b, 4, &numb);
            }
        }
#endif
        commandThread->join();
        delete commandThread;
    }
}

void Bootstrapper::cleanup() {
    _reactor = nullptr;
    NET4CXX_ObjectManager->cleanup();
    NET4CXX_Watcher->dumpAll();
    Logging::close();
}


void BasicBootstrapper::onPreInit() {
    LogUtil::initGlobalLoggers();
    setupCommonWatchObjects();
}

void BasicBootstrapper::setupCommonWatchObjects() {
    NET4CXX_WATCH_OBJECT(WatchKeys::TCPServerConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::TCPListenerCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::TCPClientConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::TCPConnectorCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::SSLServerConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::SSLListenerCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::SSLClientConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::SSLConnectorCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::UNIXServerConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UNIXListenerCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UNIXClientConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UNIXConnectorCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::UDPConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UNIXDatagramConnectionCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::DeferredCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::IOStreamCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UInt8ReceiverCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UInt8ExtReceiverCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UInt16ReceiverCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UInt16ExtReceiverCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::UInt32ReceiverCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::WebSocketServerProtocolCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::WebSocketClientProtocolCount);

    NET4CXX_WATCH_OBJECT(WatchKeys::PeriodicCallbackCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::HTTPClientCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::HTTPClientConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::HTTPConnectionCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::HTTPServerRequestCount);
    NET4CXX_WATCH_OBJECT(WatchKeys::RequestHandlerCount);
}


void AppBootstrapper::onPreInit() {
    BasicBootstrapper::onPreInit();
    LogUtil::defineLoggingOptions(NET4CXX_Options);
}

NS_END