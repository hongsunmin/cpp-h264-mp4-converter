#include <string>
#include <stdint.h>
#include <stdio.h>
#pragma once

class H264MP4ConverterPrivate;

#define HME_CHECK(expr, message)                                       \
  do                                                                   \
  {                                                                    \
    if (!(expr))                                                       \
    {                                                                  \
      printf("ERROR %s(%d): %s\n    EXPR: %s\n    FUNC: %s\n",         \
             __FILE__, __LINE__, message, #expr, __PRETTY_FUNCTION__); \
      abort();                                                         \
    }                                                                  \
  } while (false)

#define HME_CHECK_INTERNAL(expr) HME_CHECK(expr, "Internal error")

#define HME_PROPERTY(type, name, default_value)                     \
private:                                                            \
  type name = default_value;                                        \
                                                                    \
public:                                                             \
  const type &get_##name() const { return name; };                  \
  void set_##name(const type &value)                                \
  {                                                                 \
    HME_CHECK(!private_, "Cannot set properties after initialize"); \
    name = value;                                                   \
  };

class H264MP4Converter
{
public:
  friend class H264MP4ConverterPrivate;

  HME_PROPERTY(std::string, outputFilename, "output.mp4");

  HME_PROPERTY(uint32_t, width, 0);

  HME_PROPERTY(uint32_t, height, 0);

  HME_PROPERTY(uint32_t, frameRate, 30);

  // The bitrate in kbps relative to the frame_rate. Overwrites quantization_parameter.
  HME_PROPERTY(uint32_t, kbps, 0);

  // Speed where 0 means best quality [0..10].
  HME_PROPERTY(uint32_t, speed, 0);

  // Higher means better compression, and lower means better quality [10..51].
  HME_PROPERTY(uint32_t, quantizationParameter, 33);

  // Key frame period.
  HME_PROPERTY(uint32_t, groupOfPictures, 20);

  // Use temporal noise supression.
  HME_PROPERTY(bool, temporalDenoise, false);

  // Each NAL unit will be approximately capped at this size (0 means unlimited).
  HME_PROPERTY(uint32_t, desiredNaluBytes, 0);

  // Prints extra debug information.
  HME_PROPERTY(bool, debug, false);

  void initialize();

  void addFrame(const uint8_t *nalu_data, int sizeof_nalu_data);

  void finalize();

  ~H264MP4Converter();

private:
  H264MP4ConverterPrivate *private_ = nullptr;
};
