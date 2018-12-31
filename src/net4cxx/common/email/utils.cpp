//
// Created by yuwenyong.vincent on 2018-12-23.
//

#include "net4cxx/common/email/utils.h"
#include "net4cxx/common/utilities/strutil.h"


NS_BEGIN

const boost::regex EMailUtils::rfc2231Continuation(
        R"((?<name>\w+)\*((?<num>[0-9]+)\*?)?)",
        boost::regex::perl
);

QueryArgList EMailUtils::decodeParams(QueryArgList params) {
    using RFC2231Param = std::tuple<boost::optional<int>, std::string, bool>;
    using RFC2231Params = std::vector<RFC2231Param>;
    using RFC2231ParamsMap = std::map<std::string, RFC2231Params>;

    QueryArgList newParams;
    RFC2231ParamsMap rfc2231Params;
    std::string name, value, numStr;

    name = std::move(params.begin()->first);
    value = std::move(params.begin()->second);
    params.erase(params.begin());
    newParams.emplace_back(std::move(name), std::move(value));

    bool encoded;
    boost::optional<int> num;
    boost::smatch mo;
    while (!params.empty()) {
        name = std::move(params.begin()->first);
        value = std::move(params.begin()->second);
        params.erase(params.begin());
        encoded = boost::ends_with(name, "*");
        value = unquote(std::move(value));

        if (boost::regex_match(name,  mo, rfc2231Continuation)) {
            name = mo.str("name");
            numStr = mo.str("num");
            if (!numStr.empty()) {
                num = std::stoi(numStr);
            } else {
                num = boost::none;
            }
            rfc2231Params[std::move(name)].emplace_back(std::make_tuple(num, std::move(value), encoded));
        } else {
            newParams.emplace_back(std::move(name), "\"" + quote(std::move(value)) + "\"");
        }
    }

    if (!rfc2231Params.empty()) {
        std::string s;
        StringVector values;
        bool extended;
        for (auto &kv: rfc2231Params) {
            values.clear();
            extended = false;
            std::sort(kv.second.begin(), kv.second.end(), [](const RFC2231Param &lhs, const RFC2231Param &rhs) {
                if (!std::get<0>(lhs)) {
                    return true;
                }
                if (!std::get<0>(rhs)) {
                    return false;
                }
                return std::get<0>(lhs) < std::get<0>(rhs);
            });
            for (auto &continuation: kv.second) {
                if (std::get<2>(continuation)) {
                    s = UrlParse::unquote(std::get<1>(continuation));
                    extended = true;
                } else {
                    s = std::get<1>(continuation);
                }
                values.emplace_back(std::move(s));
            }
            value = quote(boost::join(values, ""));
            if (extended) {
                value = decodeRFC2231(value);
            }
            newParams.emplace_back(kv.first, "\"" + value + "\"");
        }
    }
    return newParams;
}

std::string EMailUtils::decodeRFC2231(const std::string &s) {
    auto pos = s.find('\'');
    if (pos == std::string::npos) {
        return s;
    }
    pos = s.find('\'', pos + 1);
    if (pos == std::string::npos) {
        return s;
    }
    return s.substr(pos + 1);
}

std::string EMailUtils::unquote(std::string value) {
    if (value.length() > 1) {
        if (value.front() == '\"' && value.back() == '\"') {
            value = value.substr(1, value.size() - 2);
            boost::replace_all(value, "\\\\", "\\");
            boost::replace_all(value, "\\\"", "\"");
        } else if (value.front() == '<' && value.back() == '>') {
            value = value.substr(1, value.size() - 2);
        }
    }
    return value;
}

std::string EMailUtils::quote(std::string value) {
    boost::replace_all(value, "\\", "\\\\");
    boost::replace_all(value, "\"", "\\\"");
    return value;
}

NS_END
