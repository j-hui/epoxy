#ifndef ECHO_SERVICE_H
#define ECHO_SERVICE_H

#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <binder/IBinder.h>
//#include <binder/Binder.h>

#include <utils/String16.h>

#define ECHO_SERVICE_NAME "echoservice"
#define ECHO_SERVICE_NAME16 String16(ECHO_SERVICE_NAME)

namespace android
{

class IEchoService : public IInterface {
public:
    DECLARE_META_INTERFACE(EchoService);
    virtual String16 echo(const String16& s) = 0;
    
    enum {
        TO_UPPER_TRANSACTION = IBinder::FIRST_CALL_TRANSACTION,
    };
};

class BpEchoService : public BpInterface<IEchoService> {
public:
    BpEchoService(const sp<IBinder>& impl);
    virtual String16 echo(const String16& s);
};

class BnEchoService : public BnInterface<IEchoService> {
public:
    BnEchoService(const String16& s);
    virtual status_t onTransact(uint32_t code, const Parcel& data,
            Parcel* reply, uint32_t flags = 0);
    virtual String16 echo(const String16& s);
private:
    const String16 replySuffix;
};

}

#endif // ECHO_SERVICE
