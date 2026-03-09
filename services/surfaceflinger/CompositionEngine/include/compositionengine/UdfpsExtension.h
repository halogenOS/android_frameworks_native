/*
 * SPDX-FileCopyrightText: 2026 The halogenOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>

#include <aidl/custom/hardware/biometrics/fingerprint/udfps/IUdfpsExtension.h>

namespace android::compositionengine {

#define UDFPS_BIOMETRIC_PROMPT_LAYER_NAME "BiometricPrompt"
#define UDFPS_DIM_LAYER_NAME "Dim Layer for UDFPS"
#define UDFPS_LAYER_NAME "UdfpsControllerOverlay"
#define UDFPS_TOUCHED_LAYER_NAME " SurfaceView[UdfpsControllerOverlay](BLAST)"

class UdfpsExtensionClient {
public:
    static UdfpsExtensionClient& get();

    uint32_t getUdfpsDimZOrder(uint32_t z);
    uint32_t getUdfpsZOrder(uint32_t z, bool touched);
    uint64_t getUdfpsUsageBits(uint64_t usageBits, bool touched);

private:
    UdfpsExtensionClient() = default;

    std::shared_ptr<aidl::custom::hardware::biometrics::fingerprint::udfps::IUdfpsExtension> getService();

    std::mutex mMutex;
    std::shared_ptr<aidl::custom::hardware::biometrics::fingerprint::udfps::IUdfpsExtension> mService;
    bool mServiceLookupDone = false;
};

}  // namespace android::compositionengine
