//
// Created by yuwenyong on 17-9-20.
//

#ifndef NET4CXX_CORE_NETWORK_ERROR_H
#define NET4CXX_CORE_NETWORK_ERROR_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/utilities/errors.h"

NS_BEGIN

NET4CXX_DECLARE_EXCEPTION(AlreadyCancelled, Exception);
NET4CXX_DECLARE_EXCEPTION(ReactorNotRunning, Exception);
NET4CXX_DECLARE_EXCEPTION(ReactorAlreadyRunning, Exception);
NET4CXX_DECLARE_EXCEPTION(NotConnectingError, Exception);
NET4CXX_DECLARE_EXCEPTION(ConnectionAbort, IOError);
NET4CXX_DECLARE_EXCEPTION(UserAbort, Exception);

NS_END

#endif //NET4CXX_CORE_NETWORK_ERROR_H
