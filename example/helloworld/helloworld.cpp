//
// Created by yuwenyong on 17-9-13.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;

NET4CXX_DECLARE_EXCEPTION(Test1Error, Exception);
NET4CXX_DECLARE_EXCEPTION(Test2Error, Test1Error);

int test1(int arg) {
    NET4CXX_THROW_EXCEPTION(Test2Error, "Hehe");
    return 0;
}

int test2(int arg1, int arg2) {
    return test1(arg1);
}

int test3(int arg1, int arg2) {
    return test2(arg1, arg2);
}

std::map<int, std::unique_ptr<int>> x = PtrMapCreator<int, int>()
        (10, std::make_unique<int>(11))
        (11, std::make_unique<int>(12))();


class Person {
public:
    Person() = default;
    Person(std::string name, int age, std::string gender, double height)
            : _name(std::move(name)), _age(age), _gender(std::move(gender)), _height(height) {

    }

    void display() const {
        std::cout << _name << ":" << _age << ":" << _gender << ":" << _height;
    }

    template<typename ArchiveT>
    void serialize(ArchiveT &archive) {
        archive & _name & _age & _gender & _height;
    }
protected:
    std::string _name;
    int _age;
    std::string _gender;
    double _height{0.0};
};

int main () {
    auto d1 = makeDeferred<std::string>();
    d1->addCallback([](DeferredValue<std::string> &value) {
        std::cout << "success:" << *value.asValue() << std::endl;
        value = "yyy";
    })->addErrback([](DeferredValue<std::string> &value) {
        std::cout << "error:" << value.isError() << std::endl;
    })->addCallback([](DeferredValue<std::string> &value) {
        std::cout << "success:" << *value.asValue() << std::endl;
    });
    d1->callback("xxx");

    auto d2 = makeDeferred<void>();
    d2->addBoth([](DeferredValue<void> &value){
        std::cout << "callback isError:" << value.isError() << std::endl;
        try {
            value.throwError();
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
        }
    });
    d2->errback(NET4CXX_MAKE_EXCEPTION_PTR(ValueError, "value error"));

//    DeferredPtr<int> d = makeDeferred<int>();
//    DeferredValue<int> x{d};
//    std::cout << "IsEmpty:" << x.isEmpty() << std::endl;
//    std::cout << "IsNull:" << x.isNull() << std::endl;
//    std::cout << "IsValue:" << x.isValue() << std::endl;
//    if (x.isValue()) {
//        std::cout << "Value:" << *x.asValue() << std::endl;
//        *x.asValue() = 20;
//        std::cout << "value2:" << *x.asValue() << std::endl;
//    }
//    std::cout << "IsError:" << x.isError() << std::endl;
//    if (x.isError()) {
//        try {
//            x.throwError();
//        } catch (std::exception &e) {
//            std::cout << "Error:" << e.what() << std::endl;
//        }
//    }
//    std::cout << "IsDeferred:" << x.isDeferred() << std::endl;
//    if (x.isDeferred()) {
//        std::cout << "UseCout:" << d.use_count() << std::endl;
//        d->result() = 10;
//        std::cout << "Value3:" << *x.asDeferred()->result().asValue() << std::endl;
//        x.throwError();
//        x.releaseDeferred();
//        std::cout << "UseCout:" << d.use_count() << std::endl;
//        d->result() = 30;
//        std::cout << "Value4:" << *x.asDeferred()->result().asValue() << std::endl;
//    }
//    std::cout << std::endl;
//
//    DeferredValue<void> dv2;
//    std::cout << "IsEmpty:" << dv2.isEmpty() << std::endl;
//    std::cout << "IsNull:" << dv2.isNull() << std::endl;
//    std::cout << "IsError:" << dv2.isError() << std::endl;
//    if (x.isError()) {
//        try {
//            x.throwError();
//        } catch (std::exception &e) {
//            std::cout << "Error:" << e.what() << std::endl;
//        }
//    }
//    std::cout << "IsDeferred:" << dv2.isDeferred() << std::endl;
    return 0;
}

//int main() {
//    JSONValue root{JSONType::objectValue};
//    root["str"] = "Hello world";
//    root["int"] = 3000;
//    root["double"] = 10001.1023;
//    root["bool"] = false;
//    root["null"] = nullptr;
//    root["array"] = JSONValue(JSONType::arrayValue);
//    root["array"][0] = 1;
//    root["array"][1] = "stt";
//    root["array"][2] = JSONValue(JSONType::objectValue);
//    root["array"][2]["t"] = "ttt";
//    root["object"] = JSONValue(JSONType::objectValue);
//    root["object"]["key"] = "ttt";
//    root["object"]["obj"] = JSONValue(JSONType::objectValue);
//    root["object"]["obj"]["s"] = "test";
//    root["object"]["obj"]["a"] = JSONValue(JSONType::arrayValue);
//    root["object"]["obj"]["a"][0] = "ttt";
//    root["object"]["zzz"] = true;
//    JSONValue cp, nu;
//    std::cerr << root << std::endl;
//    cp = root;
//    std::cerr << cp << std::endl;
//    if (cp == root) {
//        std::cerr << "cp == root" << std::endl;
//    }  else {
//        std::cerr << "cp != root" << std::endl;
//    }
//    if (nu == root) {
//        std::cerr << "nu == root" << std::endl;
//    }  else {
//        std::cerr << "nu != root" << std::endl;
//    }
//    std::stringstream ss;
//    JSONValue result;
//    ss << cp;
//    ss >> result;
//    std::cerr << "result" << std::endl;
//    std::cerr << result << std::endl;
//}
