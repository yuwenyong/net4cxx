//
// Created by yuwenyong on 17-9-18.
//

#include "net4cxx/common/configuration/csvreader.h"
#include <boost/functional/factory.hpp>
#include <boost/tokenizer.hpp>


NS_BEGIN


void CSVReader::open(const std::string &fileName) {
    std::ifstream file(fileName);
    if (!_file.is_open()) {
        NET4CXX_THROW_EXCEPTION(IOError, "No such file or directory:" + fileName);
    }
    _file.swap(file);
}

bool CSVReader::readNext(const ReadCallback &callback) {
    if (!_file.is_open()) {
        NET4CXX_THROW_EXCEPTION(IOError, "file is not opened");
    }
    if (!_file.good()) {
        return false;
    }
    std::string line;
    if (!std::getline(_file, line)){
        return false;
    }
    using boost::escaped_list_separator;
    using boost::tokenizer;
    escaped_list_separator<char> sep;
    tokenizer<escaped_list_separator<char>> tok(line, sep);
    StringVector fields;
    for (const auto &filed: tok) {
        fields.emplace_back(filed);
    }
    callback(fields);
    return true;
}


void CSVFile::load(const std::string &fileName) {
    CSVReader reader;
    reader.open(fileName);
    boost::ptr_vector<StringVector> table;
    while (reader.readNext([&table](const StringVector &fields) {
        table.push_back(boost::factory<StringVector *>()(fields));
    }));
    _table.swap(table);
}

NS_END