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

int main () {
//    net4cxx::Logging::init();
//    NET4CXX_INFO("abc");
    Cls1 obj1{"Hehe", 10, 1, {"sz", "sh"}}, obj2;
    obj1.print("obj1");
    net4cxx::OArchive oar;
    oar << obj1;
    net4cxx::IArchive iar(oar.contents(), oar.size());
    iar >> obj2;
    obj2.print("obj2");
    net4cxx::Logging::close();
//    net4cxx::Singleton<MyCls>::instance()->disp();
//    net4cxx::Singleton<MyCls2>::instance()->disp();
//    try {
//        net4cxx::CrashReport::printCrashInfo();
//        BOOST_ASSERT(false);
//        test();
//    } catch (std::exception &e) {
//        std::cerr << e.what() << std::endl;
//    }
    return 0;
}

void test2() {
    std::cout << boost::stacktrace::stacktrace() << std::endl;
    NET4CXX_ASSERT(false, "addw %s", "dd");
    NET4CXX_THROW_EXCEPTION(net4cxx::Exception, "wocaonima");
}

void test() {
    test2();
}