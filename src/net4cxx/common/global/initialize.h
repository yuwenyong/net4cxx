//
// Created by yuwenyong on 17-9-19.
//

#ifndef NET4CXX_COMMON_GLOBAL_INITIALIZE_H
#define NET4CXX_COMMON_GLOBAL_INITIALIZE_H

#include "net4cxx/common/common.h"
#include <boost/function.hpp>


NS_BEGIN

class NET4CXX_COMMON_API GlobalInit {
public:
    explicit GlobalInit(const boost::function1<std::string, std::string> &name_mapper) {
        initFromEnvironment(name_mapper);
    }

    GlobalInit(int argc, const char * const argv[]) {
        initFromCommandLine(argc, argv);
    }

    explicit GlobalInit(const char *path) {
        initFromConfigFile(path);
    }

    ~GlobalInit() {
        cleanup();
    }

    static void initFromEnvironment(const boost::function1<std::string, std::string> &name_mapper={});

    static void initFromCommandLine(int argc, const char * const argv[]);

    static void initFromConfigFile(const char *path);

    static void cleanup();
protected:
    static void setupWatcherHook();

    static bool _inited;
};

NS_END


#define NET4CXX_PARSE_COMMAND_LINE(argc, argv)      net4cxx::GlobalInit _initializer(argc, argv)
#define NET4CXX_PARSE_CONFIG_FILE(fileName)         net4cxx::GlobalInit _initializer(fileName)

#endif //NET4CXX_COMMON_GLOBAL_INITIALIZE_H
