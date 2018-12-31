//
// Created by yuwenyong.vincent on 2018-12-22.
//

#include "net4cxx/plugins/web/routing.h"
#include "net4cxx/plugins/web/web.h"


NS_BEGIN

UrlSpec::UrlSpec(std::string pattern, std::shared_ptr<BasicRequestHandlerFactory> handlerFactory, std::string name)
        : UrlSpec(std::move(pattern), std::move(handlerFactory), {}, std::move(name)) {

}

UrlSpec::UrlSpec(std::string pattern, std::shared_ptr<BasicRequestHandlerFactory> handlerFactory, boost::any args,
                 std::string name)
        : _pattern(std::move(pattern))
        , _handlerFactory(std::move(handlerFactory))
        , _args(std::move(args))
        , _name(std::move(name)) {
    if (!boost::ends_with(_pattern, "$")) {
        _pattern.push_back('$');
    }
    _regex = _pattern;
    std::tie(_path, _groupCount) = findGroups();
}

std::tuple<std::string, int> UrlSpec::findGroups() {
    auto beg = _pattern.begin(), end = _pattern.end();
    std::string pattern;
    if (boost::starts_with(_pattern, "^")) {
        std::advance(beg, 1);
    }
    if (boost::ends_with(_pattern, "$")) {
        std::advance(end, -1);
    }
    pattern.assign(beg, end);
    if (_regex.mark_count() != StrUtil::count(pattern, '(')) {
        return std::make_tuple("", -1);
    }
    StringVector pieces, fragments;
    std::string::size_type parenLoc;
    fragments = StrUtil::split(pattern, '(');
    for (auto &fragment: fragments) {
        parenLoc = fragment.find(')');
        if (parenLoc != std::string::npos) {
            pieces.push_back("%s" + fragment.substr(parenLoc + 1));
        } else {
            try {
                pieces.push_back(reUnescape(fragment));
            } catch (std::exception &e) {
                return std::make_tuple("", -1);
            }
        }
    }
    return std::make_tuple(boost::join(pieces, ""), (int) _regex.mark_count());
}

std::string UrlSpec::reUnescape(const std::string &s) const {
    const boost::regex pat(R"(\\(.))");
    boost::smatch what;
    auto start = s.cbegin(), end = s.cend();
    while (boost::regex_search(start, end, what, pat)) {
        char c = *what[1].begin();
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            NET4CXX_THROW_EXCEPTION(ValueError, "Cannot unescape '\\\\%s", what[1].str());
        }
        start = what[0].second;
    }
    return boost::regex_replace(s, pat, "\\1", boost::match_default | boost::format_sed);
}


std::ostream& operator<<(std::ostream &sout, const UrlSpec &url) {
    sout << StrUtil::format("UrlSpec(%s, %s, name=%s)", url.getPattern(), url.getHandlerFactory()->getName(),
                            url.getName());
    return sout;
}

NS_END