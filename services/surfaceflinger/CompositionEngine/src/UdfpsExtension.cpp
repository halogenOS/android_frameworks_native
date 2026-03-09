/*
 * SPDX-FileCopyrightText: 2026 The halogenOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <compositionengine/UdfpsExtension.h>

#include <android/binder_manager.h>
#include <log/log.h>

using aidl::custom::hardware::biometrics::fingerprint::udfps::IUdfpsExtension;

namespace android::compositionengine {

UdfpsExtensionClient& UdfpsExtensionClient::get() {
    static UdfpsExtensionClient instance;
    return instance;
}

std::shared_ptr<IUdfpsExtension> UdfpsExtensionClient::getService() {
    std::lock_guard lock(mMutex);
    if (!mServiceLookupDone) {
        const std::string instance = std::string(IUdfpsExtension::descriptor) + "/default";
        auto binder = ndk::SpAIBinder(AServiceManager_checkService(instance.c_str()));
        if (binder.get()) {
            mService = IUdfpsExtension::fromBinder(binder);
            ALOGI("UDFPS Extension HAL discovered");
        }
        mServiceLookupDone = true;
    }
    return mService;
}

uint32_t UdfpsExtensionClient::getUdfpsDimZOrder(uint32_t z) {
    auto service = getService();
    if (!service) return z;

    int32_t result;
    if (service->getUdfpsDimZOrder(static_cast<int32_t>(z), &result).isOk()) {
        return static_cast<uint32_t>(result);
    }
    return z;
}

uint32_t UdfpsExtensionClient::getUdfpsZOrder(uint32_t z, bool touched) {
    auto service = getService();
    if (!service) return z;

    int32_t result;
    if (service->getUdfpsZOrder(static_cast<int32_t>(z), touched, &result).isOk()) {
        return static_cast<uint32_t>(result);
    }
    return z;
}

uint64_t UdfpsExtensionClient::getUdfpsUsageBits(uint64_t usageBits, bool touched) {
    auto service = getService();
    if (!service) return usageBits;

    int64_t result;
    if (service->getUdfpsUsageBits(static_cast<int64_t>(usageBits), touched, &result).isOk()) {
        return static_cast<uint64_t>(result);
    }
    return usageBits;
}

}  // namespace android::compositionengine
