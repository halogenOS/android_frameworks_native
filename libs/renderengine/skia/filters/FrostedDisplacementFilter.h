/*
 * Copyright (C) 2026 The halogenOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "BlurFilter.h"
#include <SkCanvas.h>
#include <SkImage.h>
#include <SkRuntimeEffect.h>
#include <SkSurface.h>

#include "RuntimeEffectManager.h"

using namespace std;

namespace android {
namespace renderengine {
namespace skia {

/**
 * Frosted glass blur: downscales the input and applies a hash-based pixel
 * displacement that scatters detail, simulating frosted/etched glass.
 * Much cheaper than iterative Kawase — a single displacement pass replaces
 * multiple blur iterations.
 */
class FrostedDisplacementFilter : public BlurFilter {
public:
    explicit FrostedDisplacementFilter(RuntimeEffectManager& effectManager);
    virtual ~FrostedDisplacementFilter() {}

    sk_sp<SkImage> generate(SkiaGpuContext* context, const uint32_t radius,
                            const sk_sp<SkImage> blurInput,
                            const SkRect& blurRect) const override;

    void drawBlurRegion(SkCanvas* canvas, const SkRRect& effectRegion,
                        const uint32_t blurRadius, const float zoomScale,
                        const float blurAlpha, const SkRect& blurRect,
                        sk_sp<SkImage> blurredImage, sk_sp<SkImage> input) override;

private:
    sk_sp<SkRuntimeEffect> mFrostedEffect;
    sk_sp<SkRuntimeEffect> mKawaseEffect;
};

} // namespace skia
} // namespace renderengine
} // namespace android
