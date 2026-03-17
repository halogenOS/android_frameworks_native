/*
 * Copyright (C) 2026 The halogenOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "FrostedDisplacementFilter.h"
#include <SkAlphaType.h>
#include <SkBlendMode.h>
#include <SkCanvas.h>
#include <SkImageInfo.h>
#include <SkPaint.h>
#include <SkRRect.h>
#include <SkRuntimeEffect.h>
#include <SkShader.h>
#include <SkString.h>
#include <SkSurface.h>
#include <SkTileMode.h>
#include <common/trace.h>
#include <log/log.h>

#include "RuntimeEffectManager.h"

namespace android {
namespace renderengine {
namespace skia {

const SkString kEffectSource_FrostedDisplacementEffect(R"(
    uniform shader child;
    uniform float displacement;

    float hash(vec2 p) {
        vec3 p3 = fract(vec3(p.xyx) * 0.1037);
        p3 += dot(p3, p3.yzx + 31.97);
        return fract((p3.x + p3.y) * p3.z);
    }

    half4 main(float2 xy) {
        half4 acc = half4(0.0);
        float limit = displacement + 1.0;

        for (int i = 0; i < 8; i++) {
            float seed = float(i) * 13.37;
            float angle = hash(xy + 5.71 + seed) * 6.2832;
            float u = hash(xy + 29.3 + seed);
            float mag = -log(1.0 - pow(u, 1.2) * 0.98) / 2.8 * displacement;
            vec2 offset = vec2(cos(angle), sin(angle)) * mag;

            float w = mag <= 3.0 ? 1.0
                    : clamp((limit - mag) / (limit - 3.0), 0.0, 1.0);
            acc += child.eval(xy + offset) * w;
        }

        return acc / 8.0;
    }
)");

FrostedDisplacementFilter::FrostedDisplacementFilter(RuntimeEffectManager& effectManager)
      : BlurFilter(effectManager) {
    mFrostedEffect = effectManager.mKnownEffects[kFrostedDisplacementEffect];
    mKawaseEffect = effectManager.mKnownEffects[kKawaseBlurEffect];
}

sk_sp<SkImage> FrostedDisplacementFilter::generate(SkiaGpuContext* context,
                                                    const uint32_t blurRadius,
                                                    const sk_sp<SkImage> input,
                                                    const SkRect& blurRect) const {
    LOG_ALWAYS_FATAL_IF(context == nullptr, "%s: Needs GPU context", __func__);
    LOG_ALWAYS_FATAL_IF(input == nullptr, "%s: Invalid input image", __func__);

    // Downscale to 25% and crop to blurRect
    SkImageInfo scaledInfo = input->imageInfo().makeWH(
            std::ceil(blurRect.width() * kInputScale),
            std::ceil(blurRect.height() * kInputScale));

    SkMatrix blurMatrix = SkMatrix::Translate(-blurRect.fLeft, -blurRect.fTop);
    blurMatrix.postScale(kInputScale, kInputScale);

    SkSamplingOptions linear(SkFilterMode::kLinear, SkMipmapMode::kNone);

    // Phase 1: Kawase pre-blur (radius 6 → 3 passes, smooth base)
    static constexpr uint32_t kPreBlurRadius = 6;
    float tmpRadius = (float)kPreBlurRadius / 2.0f;
    uint32_t numPasses = std::min((uint32_t)3, (uint32_t)std::ceil(tmpRadius));
    float radiusByPasses = tmpRadius / (float)numPasses;

    SkRuntimeShaderBuilder kawaseBuilder(mKawaseEffect);
    kawaseBuilder.child("child") =
            input->makeShader(SkTileMode::kClamp, SkTileMode::kClamp, linear, blurMatrix);
    kawaseBuilder.uniform("in_blurOffset") = radiusByPasses * kInputScale;

    sk_sp<SkSurface> surface = context->createRenderTarget(scaledInfo);
    LOG_ALWAYS_FATAL_IF(!surface, "%s: Failed to create surface!", __func__);

    SkPaint paint;
    paint.setShader(kawaseBuilder.makeShader());
    paint.setBlendMode(SkBlendMode::kSrc);
    surface->getCanvas()->drawPaint(paint);
    sk_sp<SkImage> tmpBlur = surface->makeTemporaryImage();

    sk_sp<SkSurface> surfaceTwo = surface->makeSurface(scaledInfo);
    LOG_ALWAYS_FATAL_IF(!surfaceTwo, "%s: Failed to create second surface!", __func__);

    for (uint32_t i = 1; i < numPasses; i++) {
        kawaseBuilder.child("child") =
                tmpBlur->makeShader(SkTileMode::kClamp, SkTileMode::kClamp, linear);
        kawaseBuilder.uniform("in_blurOffset") = (float)i * radiusByPasses * kInputScale;
        paint.setShader(kawaseBuilder.makeShader());
        surfaceTwo->getCanvas()->drawPaint(paint);
        tmpBlur = surfaceTwo->makeTemporaryImage();
        using std::swap;
        swap(surface, surfaceTwo);
    }

    // Phase 2: Frosted displacement (4 samples on the pre-blurred image)
    SkRuntimeShaderBuilder frostedBuilder(mFrostedEffect);
    frostedBuilder.child("child") =
            tmpBlur->makeShader(SkTileMode::kClamp, SkTileMode::kClamp, linear);
    frostedBuilder.uniform("displacement") = (float)blurRadius * kInputScale;

    paint.setShader(frostedBuilder.makeShader());
    surfaceTwo->getCanvas()->drawPaint(paint);

    return surfaceTwo->makeTemporaryImage();
}

void FrostedDisplacementFilter::drawBlurRegion(SkCanvas* canvas, const SkRRect& effectRegion,
                                                const uint32_t blurRadius, const float zoomScale,
                                                const float blurAlpha, const SkRect& blurRect,
                                                sk_sp<SkImage> blurredImage,
                                                sk_sp<SkImage> input) {
    // Delegate to base class — output is 25% scale, base handles upscale
    BlurFilter::drawBlurRegion(canvas, effectRegion, blurRadius, zoomScale,
                                blurAlpha, blurRect, blurredImage, input);
}

} // namespace skia
} // namespace renderengine
} // namespace android
