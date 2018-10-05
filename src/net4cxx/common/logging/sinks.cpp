//
// Created by yuwenyong on 17-9-15.
//

#include "net4cxx/common/logging/sinks.h"
#include <boost/algorithm/string.hpp>
#include <boost/core/null_deleter.hpp>
#include "net4cxx/common/logging/logging.h"


NS_BEGIN

SinkPtr SinkBuilder::build() const {
    FrontendSinkPtr sink;
    if (_async) {
        sink = createAsyncSink();
    } else {
        sink = createSink();
    }
    onSetFilter(sink);
    onSetFormatter(sink);
    return sink;
}


ConsoleSinkBuilder::FrontendSinkPtr ConsoleSinkBuilder::createSink() const {
    auto backend = createBackend();
    auto sink = boost::make_shared<sinks::synchronous_sink<BackendSink>>(backend);
    return sink;
}

ConsoleSinkBuilder::FrontendSinkPtr ConsoleSinkBuilder::createAsyncSink() const {
    auto backend = createBackend();
    auto sink = boost::make_shared<sinks::asynchronous_sink<BackendSink>>(backend);
    sink->set_exception_handler(logging::nop());
    return sink;
}

ConsoleSinkBuilder::BackendSinkPtr ConsoleSinkBuilder::createBackend() const {
    auto backend = boost::make_shared<BackendSink>();
    backend->add_stream(boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));
    backend->auto_flush(_autoFlush);
    return backend;
}


FileSinkBuilder::BackendSinkPtr FileSinkBuilder::createBackend() const {
    auto backend = boost::make_shared<BackendSink>();
    backend->add_stream(boost::make_shared<std::ofstream>(_fileName));
    backend->auto_flush(_autoFlush);
    return backend;
}


RotatingFileSinkBuilder::FrontendSinkPtr RotatingFileSinkBuilder::createSink() const {
    auto backend = createBackend();
    auto sink = boost::make_shared<sinks::synchronous_sink<BackendSink>>(backend);
    return sink;
}

RotatingFileSinkBuilder::FrontendSinkPtr RotatingFileSinkBuilder::createAsyncSink() const {
    auto backend = createBackend();
    auto sink = boost::make_shared<sinks::asynchronous_sink<BackendSink>>(backend);
    sink->set_exception_handler(logging::nop());
    return sink;
}

RotatingFileSinkBuilder::BackendSinkPtr RotatingFileSinkBuilder::createBackend() const {
    auto backend = boost::make_shared<BackendSink>(
            keywords::file_name = _fileName,
            keywords::open_mode = _mode,
            keywords::rotation_size = _maxFileSize,
            keywords::auto_flush = _autoFlush
    );
    return backend;
}


TimedRotatingFileSinkBuilder::BackendSinkPtr TimedRotatingFileSinkBuilder::createBackend() const {
    return boost::apply_visitor(RotationTimeVisitor(_fileName, _maxFileSize, _mode, _autoFlush), _rotationTime);
}


#ifndef BOOST_LOG_WITHOUT_SYSLOG

SyslogSinkBuilder::FrontendSinkPtr SyslogSinkBuilder::createSink() const {
    auto backend = createBackend();
    auto sink = boost::make_shared<sinks::synchronous_sink<BackendSink>>(backend);
    return sink;
}

SyslogSinkBuilder::FrontendSinkPtr SyslogSinkBuilder::createAsyncSink() const {
    auto backend = createBackend();
    auto sink = boost::make_shared<sinks::asynchronous_sink<BackendSink>>(backend);
    sink->set_exception_handler(logging::nop());
    return sink;
}

SyslogSinkBuilder::BackendSinkPtr SyslogSinkBuilder::createBackend() const {
    auto backend = boost::make_shared<BackendSink>(keywords::facility=_facility);
    if (_targetAddress) {
        backend->set_target_address(*_targetAddress, _targetPort);
    }
    sinks::syslog::custom_severity_mapping< std::string > mapping("Severity");
    mapping["trace"] = sinks::syslog::debug;
    mapping["debug"] = sinks::syslog::debug;
    mapping["info"] = sinks::syslog::info;
    mapping["warning"] = sinks::syslog::warning;
    mapping["error"] = sinks::syslog::error;
    mapping["fatal"] = sinks::syslog::critical;
    backend->set_severity_mapper(mapping);
    return backend;
}

#endif


#ifndef BOOST_LOG_WITHOUT_EVENT_LOG

SimpleEventLogSinkBuilder::FrontendSinkPtr SimpleEventLogSinkBuilder::createSink() const {
    auto backend = createBackend();
    auto sink = boost::make_shared<sinks::synchronous_sink<BackendSink>>(backend);
    return sink;
}

SimpleEventLogSinkBuilder::FrontendSinkPtr SimpleEventLogSinkBuilder::createAsyncSink() const {
    auto backend = createBackend();
    auto sink = boost::make_shared<sinks::asynchronous_sink<BackendSink>>(backend);
    sink->set_exception_handler(logging::nop());
    return sink;
}

SimpleEventLogSinkBuilder::BackendSinkPtr SimpleEventLogSinkBuilder::createBackend() const {
    std::string logName = _logName, logSource = _logSource;
    if (logName.empty()) {
        logName = BackendSink::get_default_log_name();
    }
    if (logSource.empty()) {
        logSource = BackendSink::get_default_source_name();
    }
    auto backend = boost::make_shared<BackendSink>(
            keywords::log_name = logName,
            keywords::log_source = logSource,
            keywords::registration = _registrationMode
    );
    sinks::event_log::custom_event_type_mapping<LogLevel> mapping("Severity");
    mapping[SEVERITY_TRACE] = sinks::event_log::info;
    mapping[SEVERITY_DEBUG] = sinks::event_log::info;
    mapping[SEVERITY_INFO] = sinks::event_log::info;
    mapping[SEVERITY_WARN] = sinks::event_log::warning;
    mapping[SEVERITY_ERROR] = sinks::event_log::error;
    mapping[SEVERITY_FATAL] = sinks::event_log::error;
    backend->set_event_type_mapper(mapping);
    return backend;
}

#endif


#ifndef BOOST_LOG_WITHOUT_DEBUG_OUTPUT

DebuggerSinkBuilder::FrontendSinkPtr DebuggerSinkBuilder::createSink() const {
    auto backend = createBackend();
    auto sink = boost::make_shared<sinks::synchronous_sink<BackendSink>>(backend);
    return sink;
}

DebuggerSinkBuilder::FrontendSinkPtr DebuggerSinkBuilder::createAsyncSink() const {
    auto backend = createBackend();
    auto sink = boost::make_shared<sinks::asynchronous_sink<BackendSink>>(backend);
    sink->set_exception_handler(logging::nop());
    return sink;
}

DebuggerSinkBuilder::BackendSinkPtr DebuggerSinkBuilder::createBackend() const {
    auto backend = boost::make_shared<BackendSink>();
    return backend;
}

#endif


bool BasicSinkFactory::paramCastToBool(const string_type &param) {
    if (boost::iequals(param, "true")) {
        return true;
    }
    if (boost::iequals(param, "false")) {
        return false;
    }
    return std::stoi(param) != 0;
}


boost::shared_ptr<sinks::sink> FileSinkFactory::create_sink(settings_section const &settings) {
    std::string fileName;
    if (boost::optional<std::string> param = settings["FileName"]) {
        fileName = param.get();
    } else {
        throw std::runtime_error("No target file name specified in settings");
    }
    bool autoFlush = false;
    if (boost::optional<string_type> autoFlushParam = settings["AutoFlush"]) {
        autoFlush = paramCastToBool(autoFlushParam.get());
    }
    auto backend = boost::make_shared<BackendSink>();
    backend->add_stream(boost::make_shared<std::ofstream>(fileName));
    backend->auto_flush(autoFlush);
    return initSink(backend, settings);
}

NS_END