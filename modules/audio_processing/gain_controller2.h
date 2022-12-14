/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_AUDIO_PROCESSING_GAIN_CONTROLLER2_H_
#define MODULES_AUDIO_PROCESSING_GAIN_CONTROLLER2_H_

#include <atomic>
#include <memory>
#include <string>

#include "modules/audio_processing/agc2/adaptive_digital_gain_controller.h"
#include "modules/audio_processing/agc2/cpu_features.h"
#include "modules/audio_processing/agc2/gain_applier.h"
#include "modules/audio_processing/agc2/input_volume_controller.h"
#include "modules/audio_processing/agc2/limiter.h"
#include "modules/audio_processing/agc2/vad_wrapper.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/audio_processing/logging/apm_data_dumper.h"

namespace webrtc {

class AudioBuffer;

// Gain Controller 2 aims to automatically adjust levels by acting on the
// microphone gain and/or applying digital gain.
class GainController2 {
 public:
  // Ctor. If `use_internal_vad` is true, an internal voice activity
  // detector is used for digital adaptive gain.
  GainController2(const AudioProcessing::Config::GainController2& config,
                  int sample_rate_hz,
                  int num_channels,
                  bool use_internal_vad);
  GainController2(const GainController2&) = delete;
  GainController2& operator=(const GainController2&) = delete;
  ~GainController2();

  // Sets the fixed digital gain.
  void SetFixedGainDb(float gain_db);

  // Updates the input volume controller about whether the capture output is
  // used or not.
  void SetCaptureOutputUsed(bool capture_output_used);

  // Analyzes `audio_buffer` before `Process()` is called so that the analysis
  // can be performed before digital processing operations take place (e.g.,
  // echo cancellation). The analysis consists of input clipping detection and
  // prediction (if enabled). The value of `applied_input_volume` is limited to
  // [0, 255].
  void Analyze(int applied_input_volume, const AudioBuffer& audio_buffer);

  // Applies fixed and adaptive digital gains to `audio` and runs a limiter.
  // If the internal VAD is used, `speech_probability` is ignored. Otherwise
  // `speech_probability` is used for digital adaptive gain if it's available
  // (limited to values [0.0, 1.0]). Handles input volume changes; if the caller
  // cannot determine whether an input volume change occurred, set
  // `input_volume_changed` to false.
  void Process(absl::optional<float> speech_probability,
               bool input_volume_changed,
               AudioBuffer* audio);

  static bool Validate(const AudioProcessing::Config::GainController2& config);

  AvailableCpuFeatures GetCpuFeatures() const { return cpu_features_; }

  // Returns the recommended input volume if input volume controller is enabled
  // and if a volume recommendation is available.
  absl::optional<int> GetRecommendedInputVolume() const;

 private:
  static std::atomic<int> instance_count_;
  const AvailableCpuFeatures cpu_features_;
  ApmDataDumper data_dumper_;
  GainApplier fixed_gain_applier_;
  std::unique_ptr<VoiceActivityDetectorWrapper> vad_;
  std::unique_ptr<AdaptiveDigitalGainController> adaptive_digital_controller_;
  std::unique_ptr<InputVolumeController> input_volume_controller_;
  Limiter limiter_;
  int calls_since_last_limiter_log_;
};

}  // namespace webrtc

#endif  // MODULES_AUDIO_PROCESSING_GAIN_CONTROLLER2_H_
