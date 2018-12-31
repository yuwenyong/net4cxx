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
        const auto &rotateMode = options->get<std::string>("log_rotate_mode");
        if (rotateMode == "size") {
            const auto &logFilePrefix = options->get<std::string>("log_file_prefix");
            size_t fileMaxSize = options->get<size_t>("log_file_max_size");
            RotatingFileSinkBuilder builder(logFilePrefix, fileMaxSize);
            builder.setFormatter();
            builder.setFilter(severity);
            Logging::addSink(builder.build());
        } else if (rotateMode == "time") {
            const auto &logFilePrefix = options->get<std::string>("log_file_prefix");
            size_t fileMaxSize = options->get<size_t>("log_file_max_size");
            TimedRotatingFileSinkBuilder builder(logFilePrefix, sinks::file::rotation_at_time_point(0, 0, 0),
                                                 fileMaxSize);
            if (options->has("log_rotate_interval")) {
                builder.setRotationTimeInterval(parseRotateInterval(options));
            } else {
                builder.setRotationTimePoint(parseRotateWhen(options));
            }
            builder.setFormatter();
            builder.setFilter(severity);
            Logging::addSink(builder.build());
        } else {
            NET4CXX_THROW_EXCEPTION(ValueError, "The value of log_rotate_mode option should be"
                                                " \"size\" or \"time\", not \"%s\"", rotateMode);
        }
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
    options->addArgument<std::string>("log_rotate_when", "The time point of timed rotating", std::string("0:0:0"),
                                      {}, "logging");
    options->addArgument<std::string>("log_rotate_interval", "The interval value of timed rotating", {}, {}, "logging");
    options->addArgument<std::string>("log_rotate_mode", "The mode of rotating files(time or size)",
                                      std::string("size"), {}, "logging");
    options->addParseCallback([options]() {
        enablePrettyLogging(options);
    });
}

sinks::file::rotation_at_time_point LogUtil::parseRotateWhen(const OptionParser *options) {
    const auto &logRotateWhen = options->get<std::string>("log_rotate_when");
    const boost::regex pat(R"((\d+):(\d+):(\d+))");
    boost::smatch what;
    if (!boost::regex_match(logRotateWhen, what, pat)) {
        NET4CXX_THROW_EXCEPTION(ValueError, "The format of log_rotate_when should be \"hh:mm:ss\", not \"%s\"",
                                logRotateWhen);
    }
    int hour, minute, second;
    hour = std::stoi(what[1].str());
    minute = std::stoi(what[2].str());
    second = std::stoi(what[3].str());
    if (hour < 0 || hour > 23) {
        NET4CXX_THROW_EXCEPTION(ValueError, "The hour part of log_rotate_when should be between 0 and 23, not %d",
                                hour);
    }
    if (minute < 0 || minute > 59) {
        NET4CXX_THROW_EXCEPTION(ValueError, "The minute part of log_rotate_when should be between 0 and 59, not %d",
                                minute);
    }
    if (second < 0 || second > 59) {
        NET4CXX_THROW_EXCEPTION(ValueError, "The second part of log_rotate_when should be between 0 and 59, not %d",
                                second);
    }
    return sinks::file::rotation_at_time_point((unsigned char)hour, (unsigned char)minute, (unsigned char)second);
}

sinks::file::rotation_at_time_interval LogUtil::parseRotateInterval(const OptionParser *options) {
    auto logRotateInterval = options->get<std::string>("log_rotate_interval");
    const boost::regex pat(R"((\d+)([hms]))");
    boost::smatch what;
    if (!boost::regex_match(logRotateInterval, what, pat)) {
        NET4CXX_THROW_EXCEPTION(ValueError, "The format of log_rotate_interval should be \"ddh\", \"ddm\" or \"dds\", "
                                            "not \"%s\"", logRotateInterval);
    }
    if (what[2].str() == "h") {
        int hours = std::stoi(what[1].str());
        if (hours <= 0) {
            NET4CXX_THROW_EXCEPTION(ValueError, "The hour interval of log_rotate_when should be greater than zero");
        }
        return sinks::file::rotation_at_time_interval(boost::posix_time::hours(hours));
    } else if (what[2].str() == "m") {
        int minutes = std::stoi(what[1].str());
        if (minutes <= 0) {
            NET4CXX_THROW_EXCEPTION(ValueError, "The minute interval of log_rotate_when should be greater than zero");
        }
        return sinks::file::rotation_at_time_interval(boost::posix_time::minutes(minutes));
    } else {
        NET4CXX_ASSERT(what[2].str() == "s");
        int seconds = std::stoi(what[1].str());
        if (seconds <= 0) {
            NET4CXX_THROW_EXCEPTION(ValueError, "The second interval of log_rotate_when should be greater than zero");
        }
        return sinks::file::rotation_at_time_interval(boost::posix_time::seconds(seconds));
    }
}

NS_END