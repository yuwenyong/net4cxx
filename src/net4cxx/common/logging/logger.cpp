//
// Created by yuwenyong on 17-9-15.
//

#include "net4cxx/common/logging/logger.h"
#include <boost/log/utility/manipulators.hpp>
#include "net4cxx/common/logging/logging.h"

NS_BEGIN

Logger* Logger::getChild(const std::string &suffix) const {
    return Logging::getChildLogger(this, suffix);
}


void Logger::write(Severity severity, const Byte *data, size_t length, size_t limit) {
    logging::record rec = _logger.open_record(keywords::severity=severity);
    if (rec) {
        logging::record_ostream strm(rec);
        if (limit > 0) {
            strm << logging::dump(data, length, limit);
        } else {
            strm << logging::dump(data, length);
        }
        _logger.push_record(std::move(rec));
    }
}

void Logger::write(const StringLiteral &file, size_t line, const StringLiteral &func, Severity severity,
                   const Byte *data, size_t length, size_t limit) {
    logging::record rec = _logger.open_record((keywords::severity=severity, logger_keywords::file=file,
                                               logger_keywords::line=line, logger_keywords::func=func));
    if (rec) {
        logging::record_ostream strm(rec);
        if (limit > 0) {
            strm << logging::dump(data, length, limit);
        } else {
            strm << logging::dump(data, length);
        }
        _logger.push_record(std::move(rec));
    }
}

NS_END