#include <stdio.h>
#include <util/impl.h>

#include "h264-mp4-converter.h"

using namespace mp4v2::util;

char *prog_name;

int read_nal_data(FILE *fp, long int offset, uint8_t * &out_buffer) {
  const uint8_t nal_start_code[4] = {0, 0, 0, 1};
  
  int buffer_cap = 512 * 1024;
  uint8_t *buffer = (uint8_t *) malloc(buffer_cap);
  fseek(fp, offset, SEEK_SET);
  int ret = fread(buffer, 1, buffer_cap, fp);
  printf("read %d bytes\n", ret);

  uint8_t *buffer_begin = buffer + 4;
  uint8_t *buffer_end = buffer + ret;
  
  int packet_size = -1;
  while (buffer_begin != buffer_end) {
    if (*buffer_begin == 0x01) {
      if (memcmp(buffer_begin - 3, nal_start_code, 4) == 0) {
        packet_size = buffer_begin - buffer - 3;
        printf("nalu size is %d, nal_unit_type: %x\n", packet_size, buffer[4]);
        printf("%d %d %d %d\n", buffer[0], buffer[1], buffer[2], buffer[3]);
        out_buffer = (uint8_t *) malloc(packet_size);
        memcpy(out_buffer, buffer, packet_size);
        break;
      }
    }
    
    ++buffer_begin;
  }

  free(buffer);
  return packet_size;
}

int main(int argc, char *argv[]) {
  const char* usageString = "usage: %s [-f [<framerate>]] [-v [<level>]] <h264-file-name>\n";
  MP4LogLevel verbosity = MP4_LOG_ERROR;
  uint32_t frame_rate = 25;

  prog_name = argv[0];
  while (true) {
    int c = -1;
    int option_index = 0;
    static const prog::Option long_options[] = {
      { "framerate",  prog::Option::OPTIONAL_ARG, 0, 'f' },
      { "verbose", prog::Option::OPTIONAL_ARG, 0, 'v' },
      { NULL, prog::Option::NO_ARG, 0, 0 }
    };

    c = prog::getOptionSingle( argc, argv, "f:v", long_options, &option_index );
    if (c == -1) {
      break;
    }

    switch (c) {
    case 'f':
      if (sscanf(prog::optarg, "%d", &frame_rate) != 1) {
          fprintf(stderr,
              "%s: bad frame rate: %s\n",
              prog_name, optarg);
      }
      break;

    case 'v':
      verbosity = MP4_LOG_VERBOSE1;
      if (prog::optarg) {
          uint32_t level;
          if (sscanf(prog::optarg, "%u", &level) == 1) {
              if (level >= 2) {
                  verbosity = MP4_LOG_VERBOSE2;
              }
              if (level >= 3) {
                  verbosity = MP4_LOG_VERBOSE3;
              }
              if (level >= 4) {
                  verbosity = MP4_LOG_VERBOSE4;
              }
          }
      }
    case '?':
      fprintf(stderr, usageString, prog_name);
      return 0;
    
    default:
      fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
          prog_name, c);
    }
  }

  if ((argc - prog::optind) < 1) {
    fprintf(stderr, usageString, prog_name);
    return 1;
  }

  MP4LogSetLevel(verbosity);

  H264MP4Converter converter;
  converter.set_width(1280);
  converter.set_height(720);
  converter.set_frameRate(frame_rate);
  converter.set_debug(true);

  char *src_name = argv[prog::optind++];
  FILE *fp = fopen(src_name, "rb");
  if (fp == nullptr) {
    printf("%s is not exist\n", src_name);
  } else {
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    rewind(fp);
    printf("%s file size is %d\n", src_name, size);

    converter.initialize();

    int offset = 0;
    int buffer_size = 0;
    do {
      uint8_t *buffer = nullptr;
      buffer_size = read_nal_data(fp, offset, buffer);
      if (buffer_size > 0) {
        int nal_unit_type = buffer[4] & 0x1F;
        if (nal_unit_type == 7 || nal_unit_type == 8
            || nal_unit_type == 5 || nal_unit_type == 1)
        converter.addFrame(buffer + 4, buffer_size);
      }

      offset += buffer_size;
      free(buffer);
    } while (buffer_size > 0);

    converter.finalize();
    fclose(fp);
  }
  return 0;
}