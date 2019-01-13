//
// Created by yuwenyong.vincent on 2019-01-13.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;


class Person {
public:
    Person() = default;
    Person(std::string name, int age, std::string gender, double height)
            : _name(std::move(name)), _age(age), _gender(std::move(gender)), _height(height) {

    }

    void display() const {
        std::cout << _name << ":" << _age << ":" << _gender << ":" << _height << std::endl;
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


class ArchiveTest: public CommonBootstrapper {
public:
    void onRun() override {
        Person p1{"testName", 21, "M", 167.5};
        Person p2;

        Archive<> a1;
        Archive<ByteOrderNetwork> a2;
        Archive<ByteOrderBigEndian> a3;
        Archive<ByteOrderLittleEndian> a4;

        std::cout << "Archive test start" << std::endl;
        p1.display();

        std::cout << "Archive Native" << std::endl;
        a1 << p1;
        a1 >> p2;
        p2.display();

        std::cout << "Archive Network" << std::endl;
        a2 << p1;
        a2 >> p2;
        p2.display();

        std::cout << "Archive BE" << std::endl;
        a3 << p1;
        a3 >> p2;
        p2.display();

        std::cout << "Archive LE" << std::endl;
        a4 << p1;
        a4 >> p2;
        p2.display();
    }
};


int main(int argc, char **argv) {
    ArchiveTest app;
    app.run(argc, argv);
    return 0;
}