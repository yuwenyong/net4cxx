//
// Created by yuwenyong on 17-9-15.
//

#ifndef NET4CXX_COMMON_LOGGING_LOGGING_H
#define NET4CXX_COMMON_LOGGING_LOGGING_H

#include "net4cxx/common/common.h"
#include <mutex>
#include <boost/log/core.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include "net4cxx/common/debugging/assert.h"
#include "net4cxx/common/logging/logger.h"
#include "net4cxx/common/logging/sinks.h"

NS_BEGIN

class NET4CXX_COMMON_API Logging {
public:
    friend class Logger;
    typedef logging::settings Settings;
    typedef boost::ptr_map<std::string, Logger> LoggerMap;

    static void init();

    static void initFromSettings(const Settings &settings);

    static void initFromFile(const std::string &fileName);

    static void close() {
        logging::core::get()->flush();
        logging::core::get()->remove_all_sinks();
        _loggers.clear();
        _rootLogger = nullptr;
    }

    static void disable() {
        logging::core::get()->set_logging_enabled(false);
    }

    static void enable() {
        logging::core::get()->set_logging_enabled(true);
    }

    static void setFilter(Severity severity) {
        logging::core::get()->set_filter(attr_severity >= severity);
    }

    static void setFilter(const std::string &filter) {
        logging::core::get()->set_filter(logging::parse_filter(filter));
    }

    static void resetFilter() {
        logging::core::get()->reset_filter();
    }

    static Logger* getRootLogger() {
        return _rootLogger;
    }

    static bool  isInitialized() {
        return _rootLogger != nullptr;
    }

    static Logger* getLogger(const std::string &name="") {
        std::string loggerName = getLoggerName(name);
        return getLoggerByName(std::move(loggerName));
    }

    static std::string getLoggerName(const std::string &name) {
        std::string prefix("root");
        if (name.empty()) {
            return prefix;
        } else {
            return prefix + '.' + name;
        }
    }

    static void addSink(const BaseSink &sink) {
        logging::core::get()->add_sink(sink.makeSink());
    }

    static Severity toSeverity(std::string severity);

    template <typename... Args>
    static void trace(const char *format, Args&&... args) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->trace(format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void trace(const StringLiteral &file, size_t line, const StringLiteral &func, const char *format,
                      Args&&... args) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->trace(file, line, func, format, std::forward<Args>(args)...);
    }

    static void trace(const Byte *data, size_t length, size_t limit=0) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->trace(data, length, limit);
    }

    static void trace(const StringLiteral &file, size_t line, const StringLiteral &func, const Byte *data,
                      size_t length, size_t limit=0) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->trace(file, line, func, data, length, limit);
    }

    template <typename... Args>
    static void debug(const char *format, Args&&... args) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->debug(format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void debug(const StringLiteral &file, size_t line, const StringLiteral &func, const char *format,
                      Args&&... args) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->debug(file, line, func, format, std::forward<Args>(args)...);
    }

    static void debug(const Byte *data, size_t length, size_t limit=0) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->debug(data, length, limit);
    }

    static void debug(const StringLiteral &file, size_t line, const StringLiteral &func, const Byte *data,
                      size_t length, size_t limit=0) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->debug(file, line, func, data, length, limit);
    }

    template <typename... Args>
    static void info(const char *format, Args&&... args) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->info(format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void info(const StringLiteral &file, size_t line, const StringLiteral &func, const char *format,
                     Args&&... args) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->info(file, line, func, format, std::forward<Args>(args)...);
    }

    static void info(const Byte *data, size_t length, size_t limit=0) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->info(data, length, limit);
    }

    static void info(const StringLiteral &file, size_t line, const StringLiteral &func, const Byte *data,
                     size_t length, size_t limit=0) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->info(file, line, func, data, length, limit);
    }

    template <typename... Args>
    static void warn(const char *format, Args&&... args) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->warn(format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(const StringLiteral &file, size_t line, const StringLiteral &func, const char *format,
                        Args&&... args) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->warn(file, line, func, format, std::forward<Args>(args)...);
    }

    static void warn(const Byte *data, size_t length, size_t limit=0) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->warn(data, length, limit);
    }

    static void warn(const StringLiteral &file, size_t line, const StringLiteral &func, const Byte *data,
                        size_t length, size_t limit=0) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->warn(file, line, func, data, length, limit);
    }

    template <typename... Args>
    static void error(const char *format, Args&&... args) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->error(format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(const StringLiteral &file, size_t line, const StringLiteral &func, const char *format,
                      Args&&... args) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->error(file, line, func, format, std::forward<Args>(args)...);
    }

    static void error(const Byte *data, size_t length, size_t limit=0) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->error(data, length, limit);
    }

    static void error(const StringLiteral &file, size_t line, const StringLiteral &func, const Byte *data,
                      size_t length, size_t limit=0) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->error(file, line, func, data, length, limit);
    }

    template <typename... Args>
    static void fatal(const char *format, Args&&... args) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->fatal(format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void fatal(const StringLiteral &file, size_t line, const StringLiteral &func, const char *format,
                      Args&&... args) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->fatal(file, line, func, format, std::forward<Args>(args)...);
    }

    static void fatal(const Byte *data, size_t length, size_t limit=0) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->fatal(data, length, limit);
    }

    static void fatal(const StringLiteral &file, size_t line, const StringLiteral &func, const Byte *data,
                      size_t length, size_t limit=0) {
        NET4CXX_ASSERT(_rootLogger != nullptr);
        _rootLogger->fatal(file, line, func, data, length, limit);
    }
protected:
    static void onPreInit();

    static void onPostInit();

    static Logger* getLoggerByName(std::string loggerName);

    static Logger* getChildLogger(const Logger *logger, const std::string &suffix) {
        std::string loggerName = joinLoggerName(logger->getName(), suffix);
        return getLoggerByName(std::move(loggerName));
    }

    static std::string joinLoggerName(const std::string &name, const std::string &suffix) {
        NET4CXX_ASSERT(!name.empty() && !suffix.empty());
        return name + '.' + suffix;
    }

    static std::mutex _lock;
    static LoggerMap _loggers;
    static Logger *_rootLogger;
    static const std::map<std::string, Severity> _severityMapping;
};


class NET4CXX_COMMON_API LoggingHelper {
public:
    template <typename... Args>
    static void trace(const StringLiteral &file, size_t line, const StringLiteral &func, const char *format,
                      Args&&... args) {
        Logging::trace(file, line, func, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void trace(const StringLiteral &file, size_t line, const StringLiteral &func, Logger *logger,
                      const char *format, Args&&... args) {
        logger->trace(file, line, func, format, std::forward<Args>(args)...);
    }


    static void trace(const StringLiteral &file, size_t line, const StringLiteral &func, const Byte *data,
                      size_t length, size_t limit=0) {
        Logging::trace(file, line, func, data, length, limit);
    }

    static void trace(const StringLiteral &file, size_t line, const StringLiteral &func, Logger *logger,
                      const Byte *data, size_t length, size_t limit=0) {
        logger->trace(file, line, func, data, length, limit);
    }

    template <typename... Args>
    static void debug(const StringLiteral &file, size_t line, const StringLiteral &func, const char *format,
                      Args&&... args) {
        Logging::debug(file, line, func, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void debug(const StringLiteral &file, size_t line, const StringLiteral &func, Logger *logger,
                      const char *format, Args&&... args) {
        logger->debug(file, line, func, format, std::forward<Args>(args)...);
    }


    static void debug(const StringLiteral &file, size_t line, const StringLiteral &func, const Byte *data,
                      size_t length, size_t limit=0) {
        Logging::debug(file, line, func, data, length, limit);
    }

    static void debug(const StringLiteral &file, size_t line, const StringLiteral &func, Logger *logger,
                      const Byte *data, size_t length, size_t limit=0) {
        logger->debug(file, line, func, data, length, limit);
    }

    template <typename... Args>
    static void info(const StringLiteral &file, size_t line, const StringLiteral &func, const char *format,
                     Args&&... args) {
        Logging::info(file, line, func, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void info(const StringLiteral &file, size_t line, const StringLiteral &func, Logger *logger,
                     const char *format, Args&&... args) {
        logger->info(file, line, func, format, std::forward<Args>(args)...);
    }


    static void info(const StringLiteral &file, size_t line, const StringLiteral &func, const Byte *data,
                     size_t length, size_t limit=0) {
        Logging::info(file, line, func, data, length, limit);
    }

    static void info(const StringLiteral &file, size_t line, const StringLiteral &func, Logger *logger,
                     const Byte *data, size_t length, size_t limit=0) {
        logger->info(file, line, func, data, length, limit);
    }

    template <typename... Args>
    static void warn(const StringLiteral &file, size_t line, const StringLiteral &func, const char *format,
                     Args&&... args) {
        Logging::warn(file, line, func, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(const StringLiteral &file, size_t line, const StringLiteral &func, Logger *logger,
                     const char *format, Args&&... args) {
        logger->warn(file, line, func, format, std::forward<Args>(args)...);
    }


    static void warn(const StringLiteral &file, size_t line, const StringLiteral &func, const Byte *data, size_t length,
                     size_t limit=0) {
        Logging::warn(file, line, func, data, length, limit);
    }

    static void warn(const StringLiteral &file, size_t line, const StringLiteral &func, Logger *logger,
                     const Byte *data, size_t length, size_t limit=0) {
        logger->warn(file, line, func, data, length, limit);
    }

    template <typename... Args>
    static void error(const StringLiteral &file, size_t line, const StringLiteral &func, const char *format,
                      Args&&... args) {
        Logging::error(file, line, func, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(const StringLiteral &file, size_t line, const StringLiteral &func, Logger *logger,
                      const char *format, Args&&... args) {
        logger->error(file, line, func, format, std::forward<Args>(args)...);
    }


    static void error(const StringLiteral &file, size_t line, const StringLiteral &func, const Byte *data,
                      size_t length, size_t limit=0) {
        Logging::error(file, line, func, data, length, limit);
    }

    static void error(const StringLiteral &file, size_t line, const StringLiteral &func, Logger *logger,
                      const Byte *data, size_t length, size_t limit=0) {
        logger->error(file, line, func, data, length, limit);
    }

    template <typename... Args>
    static void fatal(const StringLiteral &file, size_t line, const StringLiteral &func, const char *format,
                      Args&&... args) {
        Logging::fatal(file, line, func, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void fatal(const StringLiteral &file, size_t line, const StringLiteral &func, Logger *logger,
                      const char *format, Args&&... args) {
        logger->fatal(file, line, func, format, std::forward<Args>(args)...);
    }


    static void fatal(const StringLiteral &file, size_t line, const StringLiteral &func, const Byte *data,
                      size_t length, size_t limit=0) {
        Logging::fatal(file, line, func, data, length, limit);
    }

    static void fatal(const StringLiteral &file, size_t line, const StringLiteral &func, Logger *logger,
                      const Byte *data, size_t length, size_t limit=0) {
        logger->fatal(file, line, func, data, length, limit);
    }
};

NS_END

#define NET4CXX_LOG_TRACE(...)      net4cxx::LoggingHelper::trace(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#ifdef NET4CXX_DEBUG
#define NET4CXX_LOG_DEBUG(...)      net4cxx::LoggingHelper::debug(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define NET4CXX_LOG_DEBUG(...)
#endif
#define NET4CXX_LOG_INFO(...)       net4cxx::LoggingHelper::info(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define NET4CXX_LOG_WARN(...)       net4cxx::LoggingHelper::warn(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define NET4CXX_LOG_ERROR(...)      net4cxx::LoggingHelper::error(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define NET4CXX_LOG_FATAL(...)      net4cxx::LoggingHelper::fatal(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)


#endif //NET4CXX_COMMON_LOGGING_LOGGING_H
