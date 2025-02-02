#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include "sample_queue.hpp"
#include <util/collections.hpp>

class VolumeEstimator {
public:
    VolumeEstimator(size_t sampleRate);
    VolumeEstimator();

    VolumeEstimator(const VolumeEstimator&) = default;
    VolumeEstimator& operator=(const VolumeEstimator&) = default;

    VolumeEstimator(VolumeEstimator&&) = default;
    VolumeEstimator& operator=(VolumeEstimator&&) = default;

    void feedData(const float* pcm, size_t samples);

    void update(float dt);

    float getVolume();

private:
    static constexpr float BUFFER_SIZE = 1.0f;

    float volume;
    size_t sampleRate;
    AudioSampleQueue sampleQueue;
};

#endif // GLOBED_VOICE_SUPPORT