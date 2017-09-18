//
// Created by yuwenyong on 17-9-18.
//

#ifndef NET4CXX_COMMON_CONFIGURATION_CSVREADER_H
#define NET4CXX_COMMON_CONFIGURATION_CSVREADER_H

#include "net4cxx/common/common.h"
#include <boost/ptr_container/ptr_vector.hpp>
#include "net4cxx/common/utilities/errors.h"


NS_BEGIN

class NET4CXX_COMMON_API CSVReader {
public:
    using ReadCallback = std::function<void (const StringVector &)>;

    CSVReader() = default;

    explicit CSVReader(const std::string &fileName) {
        open(fileName);
    }

    void open(const std::string &fileName);

    bool isOpen() const {
        return _file.is_open();
    }

    bool readNext(const ReadCallback &callback);

    void close() {
        _file.close();
    }
protected:
    std::ifstream _file;
};


class NET4CXX_COMMON_API CSVFile {
public:
    void load(const std::string &fileName);

    size_t getLines() const {
        return _table.size();
    }

    size_t getColumns(size_t line) const {
        checkRange(line);
        return _table[line].size();
    }

    const StringVector &getLine(size_t line) const {
        checkRange(line);
        return _table[line];
    }

    const std::string &getString(size_t line, size_t column) const {
        checkRange(line, column);
        return _table[line][column];
    }

    bool getBoolean(size_t line, size_t column) const {
        std::string retVal = getString(line, column);
        retVal.erase(std::remove(retVal.begin(), retVal.end(), '"'), retVal.end());
        return (retVal == "1" || retVal == "true" || retVal == "TRUE" || retVal == "yes" || retVal == "YES");
    }

    int getInt(size_t line, size_t column) const {
        const std::string &retVal = getString(line, column);
        return std::stoi(retVal);
    }

    float getFloat(size_t line, size_t column) const {
        const std::string &retVal = getString(line, column);
        return std::stof(retVal);
    }

    double getDouble(size_t line, size_t column) const {
        const std::string &retVal = getString(line, column);
        return std::stod(retVal);
    }
protected:
    void checkRange(size_t line) const {
        if (line >= _table.size()) {
            NET4CXX_THROW_EXCEPTION(IndexError, "line index out of range");
        }
    }

    void checkRange(size_t line, size_t column) const {
        checkRange(line);
        if (column >= _table[line].size()) {
            NET4CXX_THROW_EXCEPTION(IndexError, "column index out of range");
        }
    }

    boost::ptr_vector<StringVector> _table;
};

NS_END

#endif //NET4CXX_COMMON_CONFIGURATION_CSVREADER_H
