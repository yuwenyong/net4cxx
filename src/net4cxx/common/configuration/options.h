//
// Created by yuwenyong on 17-9-18.
//

#ifndef NET4CXX_COMMON_CONFIGURATION_OPTIONS_H
#define NET4CXX_COMMON_CONFIGURATION_OPTIONS_H

#include "net4cxx/common/common.h"
#include <boost/function.hpp>
#include <boost/program_options.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include "net4cxx/common/utilities/errors.h"

NS_BEGIN

namespace po = boost::program_options;

class NET4CXX_COMMON_API OptionParser {
public:
    explicit OptionParser(const std::string &caption)
            : _opts(caption) {
        addArgument("version,v", "print version string");
        addArgument("help,h", "display help message");
    }

    void addArgument(const char *name, const char *help, const char *group=nullptr) {
        if (group != nullptr) {
            auto opt = getGroup(group);
            opt->add_options()(name, help);
        } else {
            _opts.add_options()(name, help);
        }
    }

    template<typename ArgT>
    void addArgument(const char *name, const char *help, const boost::optional<ArgT> &defaultValue = boost::none,
                     boost::function1<void, const ArgT &> action = {}, const char *group = nullptr) {
        po::typed_value<ArgT> *value= po::value<ArgT>();
        if (defaultValue) {
            value->default_value(defaultValue.get());
        }
        if (action) {
            value->notifier(std::move(action));
        }
        if (group) {
            auto opt = getGroup(group);
            opt->add_options()(name, value, help);
        } else {
            _opts.add_options()(name, value, help);
        }
    }

    template<typename ArgT>
    void addArguments(const char *name, const char *help, boost::function1<void, const std::vector<ArgT> &> action = {},
                      const char *group = nullptr) {
        po::typed_value <std::vector<ArgT>> *value = po::value<std::vector<ArgT>>();
        if (action) {
            value->notifier(std::move(action));
        }
        value->composing();
        if (group) {
            auto opt = getGroup(group);
            opt->add_options()(name, value, help);
        } else {
            _opts.add_options()(name, value, help);
        }
    }

    bool has(const char *name) const {
        return _vm.count(name) != 0;
    }

    template <typename ValueT>
    const ValueT& get(const char *name) const {
        if (!has(name)) {
            NET4CXX_THROW_EXCEPTION(KeyError, "Unrecognized option %s", name);
        }
        auto &value = _vm[name];
        return value.as<ValueT>();
    }

    void parseCommandLine(int argc, const char * const argv[], bool final=true);

    void parseConfigFile(const char *path, bool final=true);

    void parseEnvironment(const boost::function1<std::string, std::string> &nameMapper, bool final=true);

    template <typename CallbackT>
    void addParseCallback(CallbackT &&calback) {
        _parseCallbacks.emplace_back(std::forward<CallbackT>(calback));
    }

    void printHelp() {
        std::cout << composeOptions() << std::endl;
    }

    static OptionParser * instance();
protected:
    void runParseCallbacks() const {
        for (auto &callback: _parseCallbacks) {
            callback();
        }
    }

    po::options_description* getGroup(std::string group);

    po::options_description composeOptions() const;

    void helpCallback() {
        printHelp();
        exit(1);
    }

    void versionCallback() {
        std::cout << NET4CXX_VERSION << std::endl;
        exit(2);
    }

    po::options_description _opts;
    boost::ptr_map<std::string, po::options_description> _groups;
    po::variables_map _vm;
    std::vector<std::function<void()>> _parseCallbacks;
};

NS_END

#define NET4CXX_Options     net4cxx::OptionParser::instance()

#endif //NET4CXX_COMMON_CONFIGURATION_OPTIONS_H
