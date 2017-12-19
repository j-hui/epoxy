#include <binder/IEpoxy.h>
#include <binder/IServiceManager.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>

#include <utils/String16.h>

#include <unistd.h>

#include <cstdio>
#include <iostream>

using namespace android;
using namespace std;

int main(void)
{
    cerr << "Started epoxyservice" << endl;

    //daemon(0, 0);

    sp<IServiceManager> sm = defaultServiceManager();
    sm->addService(String16(EPOXY_SERVICE_NAME), new BnEpoxy());
    cerr << "Added service " << EPOXY_SERVICE_NAME << endl;

    cerr << "Joining ProcessState ThreadPool" << endl;
    ProcessState::self()->startThreadPool();

    cerr << "Joined IPCThreadState ThreadPool..." << endl;
    IPCThreadState::self()->joinThreadPool();

    // never reached
    return 0;
}
