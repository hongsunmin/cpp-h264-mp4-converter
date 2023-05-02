// Reference:
// https://github.com/TrevorSundberg/h264-mp4-encoder/blob/master/h264-mp4-encoder.cpp

#include <string.h>
#include <stdlib.h>

#include "mp4v2/mp4v2.h"

#include "h264-mp4-converter.h"

#define STARTCODE_4BYTES 4

#define INITIALIZE_MESSAGE "Function initialize has not been called"

#define ALIGNED_ALLOC(n, size) aligned_alloc(n, (size + n - 1) / n * n)

#define TIMESCALE 90000

enum class NALU
{
  SPS = 7,
  PPS = 8,
  I = 5,
  P = 1,
  INVALID = 0x00,
};

class H264MP4ConverterPrivate
{
public:
  H264MP4Converter *encoder = nullptr;
  MP4FileHandle mp4 = nullptr;
  MP4TrackId video_track = 0;

  int frame = 0;

  static void nalu_callback(const uint8_t *nalu_data, int sizeof_nalu_data, void *token);
};

void H264MP4ConverterPrivate::nalu_callback(
    const uint8_t *nalu_data, int sizeof_nalu_data, void *token)
{
  H264MP4ConverterPrivate *encoder_private = (H264MP4ConverterPrivate *)token;
  H264MP4Converter *encoder = encoder_private->encoder;

  uint8_t *data = const_cast<uint8_t *>(nalu_data - STARTCODE_4BYTES);
  const int size = sizeof_nalu_data + STARTCODE_4BYTES;

  HME_CHECK_INTERNAL(size >= 5);
  HME_CHECK_INTERNAL(data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1);

  NALU index = (NALU)(data[4] & 0x1F);

  if (encoder->debug)
  {
    printf("nalu=");
    switch (index)
    {
    case NALU::SPS:
      printf("SPS");
      break;
    case NALU::PPS:
      printf("PPS");
      break;
    case NALU::I:
      printf("I");
      break;
    case NALU::P:
      printf("P");
      break;
    default:
      HME_CHECK(false, "Unknown frame type");
      break;
    }

    printf(", hex=");
    for (size_t i = 0; i < sizeof_nalu_data && i < 16; ++i)
    {
      if (i != 0)
      {
        printf(" ");
      }
      printf("%02X", nalu_data[i]);
      if (i == 15 && sizeof_nalu_data != 16)
      {
        printf("...");
      }
    }
    printf(", bytes=%d\n", sizeof_nalu_data);
  }

  switch (index)
  {
  case NALU::SPS:
    if (encoder_private->video_track == MP4_INVALID_TRACK_ID)
    {
      encoder_private->video_track = MP4AddH264VideoTrack(
          encoder_private->mp4,
          TIMESCALE,
          TIMESCALE / encoder->frameRate,
          encoder->width,
          encoder->height,
          data[5], // sps[1] AVCProfileIndication
          data[6], // sps[2] profile_compat
          data[7], // sps[3] AVCLevelIndication
          3);      // 4 bytes length before each NAL unit
      HME_CHECK_INTERNAL(encoder_private->video_track != MP4_INVALID_TRACK_ID);
      //  Simple Profile @ Level 3
      MP4SetVideoProfileLevel(encoder_private->mp4, 0x7F);
    }
    MP4AddH264SequenceParameterSet(encoder_private->mp4, encoder_private->video_track, nalu_data, sizeof_nalu_data);
    break;
  case NALU::PPS:
    MP4AddH264PictureParameterSet(encoder_private->mp4, encoder_private->video_track, nalu_data, sizeof_nalu_data);
    break;
  case NALU::I:
  case NALU::P:
    data[0] = sizeof_nalu_data >> 24;
    data[1] = sizeof_nalu_data >> 16;
    data[2] = sizeof_nalu_data >> 8;
    data[3] = sizeof_nalu_data & 0xff;
    HME_CHECK_INTERNAL(MP4WriteSample(encoder_private->mp4, encoder_private->video_track, data, size, MP4_INVALID_DURATION, 0, 1));
    break;
  default:
    HME_CHECK(false, "Unknown frame type");
    break;
  }
}

void H264MP4Converter::initialize()
{
  HME_CHECK(!private_, "Cannot call initialize more than once without calling finalize");
  private_ = new H264MP4ConverterPrivate();
  HME_CHECK_INTERNAL(private_);
  private_->encoder = this;

  HME_CHECK_INTERNAL(!outputFilename.empty());
  HME_CHECK_INTERNAL(width > 0);
  HME_CHECK_INTERNAL(height > 0);
  HME_CHECK_INTERNAL(frameRate > 0);
  HME_CHECK_INTERNAL(quantizationParameter >= 10 && quantizationParameter <= 51);
  HME_CHECK_INTERNAL(speed <= 10);

  private_->mp4 = MP4Create(outputFilename.c_str(), 0);
  HME_CHECK_INTERNAL(private_->mp4 != MP4_INVALID_FILE_HANDLE);
  MP4SetTimeScale(private_->mp4, TIMESCALE);
}

void H264MP4Converter::addFrame(const uint8_t *nalu_data, int sizeof_nalu_data) {
  HME_CHECK(private_, INITIALIZE_MESSAGE);
  H264MP4ConverterPrivate::nalu_callback(nalu_data, sizeof_nalu_data, private_);
  if (debug)
  {
    printf("frame=%d, bytes=%d\n", private_->frame, sizeof_nalu_data);
  }
  ++private_->frame;
}

void H264MP4Converter::finalize()
{
  HME_CHECK(private_, INITIALIZE_MESSAGE);
  MP4Close(private_->mp4, 0);
  delete private_;
  private_ = nullptr;
}

H264MP4Converter::~H264MP4Converter()
{
  HME_CHECK(private_ == nullptr, "Function finalize was not called before the encoder destructed");
}