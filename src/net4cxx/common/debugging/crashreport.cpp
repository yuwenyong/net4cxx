//
// Created by yuwenyong on 17-9-13.
//

#include "net4cxx/common/debugging/crashreport.h"
#include <boost/filesystem.hpp>
#include <boost/stacktrace.hpp>


NS_BEGIN


std::string CrashReport::_crashReportPath = "./backtrace.dump";

void CrashReport::crashHandler(int signum) {
    ::signal(signum, SIG_DFL);
    boost::stacktrace::safe_dump_to(_crashReportPath.c_str());
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


void CrashReport::outputCrashReport() {
    if (boost::filesystem::exists(_crashReportPath)) {
        std::ifstream ifs(_crashReportPath);
        boost::stacktrace::stacktrace st = boost::stacktrace::stacktrace::from_dump(ifs);
        std::cout << "Previous run crashed:\n" << st << std::endl;
        ifs.close();
    }
}

bool CrashReport::checkCrashReportExists() {
    return boost::filesystem::exists(_crashReportPath);
}

CrashReportInstaller gCrashReportInit;

NS_END