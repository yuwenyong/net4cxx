//
// Created by yuwenyong on 17-9-13.
//

#include "net4cxx/common/debugging/crashreport.h"
#include <boost/filesystem.hpp>
#include <boost/stacktrace.hpp>


NS_BEGIN

void CrashReport::crashHandler(int signum) {
    ::signal(signum, SIG_DFL);
    boost::stacktrace::safe_dump_to("./backtrace.dump");
    ::raise(SIGABRT);
}

class CrashReportInstaller {
public:
    CrashReportInstaller() noexcept {
#ifdef SIGSEGV
        ::signal(SIGSEGV, &CrashReport::crashHandler);
#endif
#ifdef SIGABRT
        ::signal(SIGABRT, &CrashReport::crashHandler);
#endif
    }
};


void CrashReport::printCrashInfo() {
    if (boost::filesystem::exists("./backtrace.dump")) {
        std::ifstream ifs("./backtrace.dump");
        boost::stacktrace::stacktrace st = boost::stacktrace::stacktrace::from_dump(ifs);
        std::cout << "Previous run crashed:\n" << st << std::endl;
        ifs.close();
    }
}

CrashReportInstaller gCrashReportInit;

NS_END