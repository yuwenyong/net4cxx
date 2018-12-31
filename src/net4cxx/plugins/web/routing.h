//
// Created by yuwenyong.vincent on 2018-12-22.
//

#ifndef NET4CXX_PLUGINS_WEB_ROUTING_H
#define NET4CXX_PLUGINS_WEB_ROUTING_H

#include "net4cxx/common/common.h"
#include "net4cxx/plugins/web/httpserver.h"

NS_BEGIN

class BasicRequestHandlerFactory;


class NET4CXX_COMMON_API UrlSpec: public boost::noncopyable {
public:
    UrlSpec(std::string pattern, std::shared_ptr<BasicRequestHandlerFactory> handlerFactory, std::string name);

    UrlSpec(std::string pattern, std::shared_ptr<BasicRequestHandlerFactory> handlerFactory, boost::any args={},
            std::string name={});

    template <typename... Args>
    std::string reverse(Args&&... args) {
        if (_path.empty()) {
            NET4CXX_THROW_EXCEPTION(ValueError, "Cannot reverse url regex %s", _pattern.c_str());
        }
        NET4CXX_ASSERT_THROW(sizeof...(Args) == _groupCount, "required number of arguments not found");
        return StrUtil::format(_path, UrlParse::quote(boost::lexical_cast<std::string>(args))...);
    }

    const boost::regex& getRegex() const {
        return _regex;
    }

    const std::string& getPattern() const {
        return _pattern;
    }

    const std::string& getName() const {
        return _name;
    }

    std::shared_ptr<const BasicRequestHandlerFactory> getHandlerFactory() const {
        return _handlerFactory;
    }

    std::shared_ptr<BasicRequestHandlerFactory> getHandlerFactory() {
        return _handlerFactory;
    }

    const boost::any& getArgs() const {
        return _args;
    }
protected:
    std::tuple<std::string, int> findGroups();

    std::string reUnescape(const std::string &s) const;

    std::string _pattern;
    boost::regex _regex;
    std::shared_ptr<BasicRequestHandlerFactory> _handlerFactory;
    boost::any _args;
    std::string _name;
    std::string _path;
    int _groupCount;
};

using UrlSpecPtr = std::shared_ptr<UrlSpec>;

NET4CXX_COMMON_API std::ostream& operator<<(std::ostream &sout, const UrlSpec &url);

using urls = std::vector<std::shared_ptr<UrlSpec>>;

NS_END

#endif //NET4CXX_PLUGINS_WEB_ROUTING_H
