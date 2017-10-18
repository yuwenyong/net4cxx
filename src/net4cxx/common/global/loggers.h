//
// Created by yuwenyong on 17-9-19.
//

#ifndef NET4CXX_COMMON_GLOBAL_LOGGERS_H
#define NET4CXX_COMMON_GLOBAL_LOGGERS_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/configuration/options.h"
#include "net4cxx/common/logging/logging.h"

NS_BEGIN

extern Logger *gAccessLog;
extern Logger *gAppLog;
extern Logger *gGenLog;


class LogUtil {
public:
    static void initGlobalLoggers();

    static void enablePrettyLogging(const OptionParser *options);

    static void defineLoggingOptions(OptionParser *options);
};

NS_END


#define NET4CXX_TCPServerConnection_COUNT   "net4cxx.TCPServerConnection.count"
#define NET4CXX_TCPListener_COUNT           "net4cxx.TCPListener.count"
#define NET4CXX_TCPClientConnection_COUNT   "net4cxx.TCPClientConnection.count"
#define NET4CXX_TCPConnector_COUNT          "net4cxx.TCPConnector.count"

#define NET4CXX_SSLServerConnection_COUNT   "net4cxx.SSLServerConnection.count"
#define NET4CXX_SSLListener_COUNT           "net4cxx.SSLListener.count"
#define NET4CXX_SSLClientConnection_COUNT   "net4cxx.SSLClientConnection.count"
#define NET4CXX_SSLConnector_COUNT          "net4cxx.SSLConnector.count"

#endif //NET4CXX_COMMON_GLOBAL_LOGGERS_H
