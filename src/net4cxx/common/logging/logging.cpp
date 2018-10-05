//
// Created by yuwenyong on 17-9-15.
//

#include "net4cxx/common/logging/logging.h"
#include <boost/functional/factory.hpp>
#include <boost/log/utility/exception_handler.hpp>
#include "net4cxx/common/utilities/errors.h"


NS_BEGIN

volatile bool Logging::_initialized = false;

volatile bool Logging::_sinkRegistered = false;

std::mutex Logging::_initLock;

std::mutex Logging::_loggersLock;

Logging::LoggerMap Logging::_loggers;

const std::string Logging::_rootLoggerName = "root";

Logger* Logging::_rootLogger = nullptr;

const std::map<std::string, Severity> Logging::_severityMapping = {
        {"trace",   SEVERITY_TRACE},
        {"debug",   SEVERITY_DEBUG},
        {"info",    SEVERITY_INFO},
        {"warning", SEVERITY_WARN},
        {"warn",    SEVERITY_WARN},
        {"error",   SEVERITY_ERROR},
        {"fatal",   SEVERITY_FATAL},
};


void Logging::init() {
    if (!_initialized) {
        doInit(true);
    }
}

void Logging::initFromSettings(const Settings &settings) {
    if (!_initialized) {
        doInit(false);
    }
    logging::init_from_settings(settings);
}

void Logging::initFromFile(const std::string &fileName) {
    if (!_initialized) {
        doInit(false);
    }
    std::ifstream file(fileName);
    logging::init_from_stream(file);
}

Severity Logging::toSeverity(std::string severity) {
    boost::to_lower(severity);
    auto iter  = _severityMapping.find(severity);
    if (iter == _severityMapping.end()) {
        NET4CXX_THROW_EXCEPTION(ValueError, "Invalid severity name:%s", severity);
    }
    return iter->second;
}

void Logging::doInit(bool registerSink) {
    std::lock_guard<std::mutex> lock(_initLock);
    if (!_initialized) {
        logging::core::get()->set_exception_handler(logging::make_exception_suppressor());
        logging::add_common_attributes();
        logging::core::get()->add_global_attribute("Uptime", attrs::timer());
        logging::register_simple_formatter_factory<logging::trivial::severity_level , char>("Severity");
        logging::register_formatter_factory("TimeStamp", boost::make_shared<TimeStampFormatterFactory>());
        logging::register_formatter_factory("Uptime", boost::make_shared<UptimeFormatterFactory>());
        logging::register_simple_filter_factory<logging::trivial::severity_level , char>("Severity");
        logging::register_sink_factory("File", boost::make_shared<FileSinkFactory>());

        if (registerSink && !_sinkRegistered) {
            ConsoleSinkBuilder builder;
            builder.setFormatter();
#ifdef NET4CXX_DEBUG
            builder.setFilter(SEVERITY_DEBUG);
#else
            builder.setFilter(SEVERITY_INFO);
#endif
            addSink(builder.build());
        }

        if (!_rootLogger) {
            _rootLogger = getLoggerByName(_rootLoggerName);
        }
        _initialized = true;
    }
}

Logger* Logging::getLoggerByName(std::string loggerName) {
    std::lock_guard<std::mutex> lock(_loggersLock);
    auto iter  = _loggers.find(loggerName);
    if (iter != _loggers.end()) {
        return iter->second;
    }
    Logger *logger = boost::factory<Logger*>()(loggerName);
    _loggers.insert(loggerName, logger);
    return logger;
}

NS_END