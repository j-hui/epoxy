#include "IEchoService.h"
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>

#include <cstdio>
#include <iostream>

using namespace std;
using namespace android;

int main(int argc, char** argv)
{
    String16 suffix{};
    if (argc > 1) {
        suffix = String16(argv[1]);
    }

    sp<IServiceManager> sm = defaultServiceManager();
    sm->addService(ECHO_SERVICE_NAME16, new BnEchoService(suffix));
    cerr << "Added service " << ECHO_SERVICE_NAME << endl;

    cerr << "Joining ProcessState ThreadPool" << endl;
    ProcessState::self()->startThreadPool();

    cerr << "Joined IPCThreadState ThreadPool..." << endl;
    IPCThreadState::self()->joinThreadPool();

    // never reached
    return 0;
}
