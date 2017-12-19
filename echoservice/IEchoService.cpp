
#define LOG_TAG "EchoService"
#include <utils/Log.h>

#include "IEchoService.h"
#include <utils/String8.h>

namespace android
{

IMPLEMENT_META_INTERFACE(EchoService, "columbia.echoservice.IEchoService");

BpEchoService::BpEchoService(const sp<IBinder>& impl) :
    BpInterface<IEchoService>(impl) {}

String16 BpEchoService::echo(const String16& s)
{
    Parcel data, reply;
    data.writeInterfaceToken(IEchoService::getInterfaceDescriptor());

    data.writeString16(s);

    ALOGD("EchoClient %p transacting msg %s", this, String8(s).string());
    remote()->transact(TO_UPPER_TRANSACTION, data, &reply);

    String16 ret = reply.readString16();
    ALOGD("EchoClient %p received reply %s", this, String8(ret).string());

    return ret;
}

BnEchoService::BnEchoService(const String16& s) : replySuffix(s)
{
    ALOGI("EchoService %p started with suffix %s", this, String8(s).string());
}

String16 BnEchoService::echo(const String16& msg)
{
    String16 reply = msg + replySuffix;
    ALOGI("EchoService %p received msg %s, replying %s", this,
            String8(msg).string(), String8(reply).string());
    return reply;
}

status_t BnEchoService::onTransact(uint32_t code, const Parcel& data,
        Parcel* reply, uint32_t /*flags*/)
{
    switch (code) {
    case TO_UPPER_TRANSACTION: {
        CHECK_INTERFACE(IEchoService, data, reply);

        String16 s1 = data.readString16();
        String16 s2 = echo(s1);

        reply->writeString16(s2);
        return NO_ERROR;
    } break;
    default: {
        ALOGE("Unknown code received: %d", code);
    } break;
    }
    return NO_ERROR;
}

} // namespace android
