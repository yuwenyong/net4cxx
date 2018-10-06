//
// Created by yuwenyong.vincent on 2018/10/6.
//

#include "net4cxx/shared/global/loggers.h"


NS_BEGIN


Logger *gAccessLog = nullptr;
Logger *gAppLog = nullptr;
Logger *gGenLog = nullptr;


void LogUtil::initGlobalLoggers() {
    gAccessLog = Logging::getLogger("access");
    gAppLog = Logging::getLogger("application");
    gGenLog = Logging::getLogger("general");
}

void LogUtil::enablePrettyLogging(const OptionParser *options) {
    const auto &logLevel = options->get<std::string>("log_level");
    if (logLevel == "none") {
        return;
    }
    Severity severity = Logging::toSeverity(logLevel);
    if (options->has("log_file_prefix")) {
        const auto &logFilePrefix = options->get<std::string>("log_file_prefix");
        size_t fileMaxSize = options->get<size_t>("log_file_max_size");
        RotatingFileSinkBuilder builder(logFilePrefix, fileMaxSize);
        builder.setFormatter();
        builder.setFilter(severity);
        Logging::addSink(builder.build());
    }
    if (options->has("log_to_console") || !options->has("log_file_prefix")) {
        ConsoleSinkBuilder builder;
        builder.setFormatter();
        builder.setFilter(severity);
        Logging::addSink(builder.build());
    }
}

void LogUtil::defineLoggingOptions(OptionParser *options) {
#ifdef NET4CXX_DEBUG
    options->addArgument<std::string>("log_level", "Set the log level", std::string("debug"), {}, "logging");
#else
    options->addArgument<std::string>("log_level", "Set the log level", std::string("info"), {}, "logging");
#endif
    options->addArgument("log_to_console", "Send log output to stderr", "logging");
    options->addArgument<std::string>("log_file_prefix", "Path prefix for log files", {}, {}, "logging");
    options->addArgument<size_t>("log_file_max_size", "Max size of log files", 100 * 1000 * 1000, {}, "logging");
    options->addParseCallback([options] () {
        enablePrettyLogging(options);
    });
}

NS_END