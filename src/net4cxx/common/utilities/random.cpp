//
// Created by yuwenyong on 17-9-13.
//

#include "net4cxx/common/utilities/random.h"


NS_BEGIN

double Random::normalvariate(double mu, double sigma) {
    static const double NV_MAGICCONST = 4.0 * std::exp(-0.5) / std::sqrt(2.0);
    double u1, u2, z, zz;
    while (true) {
        u1 = random();
        u2 = 1.0 - random();
        z = NV_MAGICCONST * (u1 - 0.5) / u2;
        zz = z * z / 4.0;
        if (zz <= -std::log(u2)) {
            break;
        }
    }
    return mu + z * sigma;
}

thread_local std::default_random_engine Random::_engine;

NS_END
