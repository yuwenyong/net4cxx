//
// Created by yuwenyong on 17-9-13.
//

#include "net4cxx/net4cxx.h"

void test2();


void test();

class MyCls {
public:
    MyCls() {
        std::cout << "Construct" << std::endl;
    }

    ~MyCls() {
        std::cout << "Destruct" << std::endl;
    }

    void disp() {
        std::cout << "disp" << std::endl;
    }
};

class MyCls2 {
public:
    MyCls2() {
        std::cout << "Construct2" << std::endl;
    }

    ~MyCls2() {
        std::cout << "Destruct2" << std::endl;
    }

    void disp() {
        std::cout << "disp2" << std::endl;
    }
};

class Cls1 {
public:
    Cls1() = default;
    Cls1(const std::string &name, int age, int sex, const std::vector<std::string> &addrs)
            : _name(name), _age(age), _sex(sex), _addrs(addrs) {

    }

    void print(const char *prompt) const {
        std::cout << "prompt:" << prompt << std::endl;
        std::cout << "Name:" << _name << std::endl;
        std::cout << "age:" << _age << std::endl;
        std::cout << "Sex:" << _sex << std::endl;
        std::cout << "Addrs:" << std::endl;
        for (auto &addr: _addrs) {
            std::cout << "\t" << addr << std::endl;
        }
    }

    template <typename AR>
    void serialize(AR &ar) {
        ar & _name;
        ar & _age;
        ar & _sex;
        ar & _addrs;
    }
protected:
    std::string _name;
    int _age;
    int _sex;
    std::vector<std::string> _addrs;
};

//bool isIpValid(const std::string &ip) {
//    std::istringstream ss(ip);
//    unsigned int field;
//    size_t count = 0;
//    char c;
//    for (; count < 4 && ss.get(c); ++count) {
//        if (c != '0') {
//            ss.unget();
//            ss >> field;
//            if (!ss || field < 0 || field > 255) {
//                return false;
//            }
//            if (count != 3 && (!ss.get(c) || c != '.')) {
//                return false;
//            }
//        } else {
//            if (ss.tellg() != ip.size() && (!ss.get(c) || c != '.')) {
//                return false;
//            }
//        }
//    }
//    return ss.tellg() == ip.size() && count == 4;
//}

bool isIpValid(const std::string &ip) {
    unsigned int b[4], length{7};
    if (sscanf(ip.c_str(), "%u.%u.%u.%u", &b[0], &b[1], &b[2], &b[3]) != 4) {
        return false;
    }
    for (auto f: b) {
        if (f > 255) {
            return false;
        } else if (f > 99) {
            length += 2;
        } else if (f > 9) {
            ++length;
        }
    }
    return length == ip.size();
}

int main () {
    std::cout << std::boolalpha << isIpValid("111.111.225.255") << std::endl;
    std::cout << std::boolalpha << isIpValid("111.111.22.0") << std::endl;
    std::cout << std::boolalpha << isIpValid("111.111.22.00") << std::endl;
    std::cout << std::boolalpha << isIpValid("111.111.22xx") << std::endl;
    std::cout << std::boolalpha << isIpValid("4294967299.1.1.1") << std::endl;
    std::cout << std::boolalpha << isIpValid("1.1.1.1a") << std::endl;
    std::cout << std::boolalpha << isIpValid("") << std::endl;
    std::cout << std::boolalpha << isIpValid("01.01.01.01") << std::endl;
////    net4cxx::Logging::init();
////    NET4CXX_INFO("abc");
//    Cls1 obj1{"Hehe", 10, 1, {"sz", "sh"}}, obj2;
//    obj1.print("obj1");
//    net4cxx::OArchive oar;
//    oar << obj1;
//    net4cxx::IArchive iar(oar.contents(), oar.size());
//    iar >> obj2;
//    obj2.print("obj2");
//    net4cxx::Logging::close();
////    net4cxx::Singleton<MyCls>::instance()->disp();
////    net4cxx::Singleton<MyCls2>::instance()->disp();
////    try {
////        net4cxx::CrashReport::printCrashInfo();
////        BOOST_ASSERT(false);
////        test();
////    } catch (std::exception &e) {
////        std::cerr << e.what() << std::endl;
////    }
    return 0;
}

void test2() {
    std::list<int> s;
    s.sort();
    std::cout << boost::stacktrace::stacktrace() << std::endl;
    NET4CXX_ASSERT(false, "addw %s", "dd");
    NET4CXX_THROW_EXCEPTION(net4cxx::Exception, "wocaonima");
}

void test() {
    test2();
}

