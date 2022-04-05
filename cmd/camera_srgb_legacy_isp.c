#include "camera.h"

#include "hw/buffer.h"
#include "hw/buffer_list.h"
#include "hw/device.h"
#include "hw/links.h"
#include "hw/v4l2.h"
#include "hw/buffer_list.h"
#include "http/http.h"

extern bool check_streaming();

void write_yuvu(buffer_t *buffer)
{
#if 0
  FILE *fp = fopen("/tmp/capture-yuyv.raw.tmp", "wb");
  if (fp) {
    fwrite(buffer->start, 1, buffer->used, fp);
    fclose(fp);
  }
  rename("/tmp/capture-yuyv.raw.tmp", "/tmp/capture-yuyv.raw");
#endif
}

int camera_configure_srgb_legacy_isp(camera_t *camera, float div)
{
  if (device_open_buffer_list(camera->camera, true, camera->width, camera->height, V4L2_PIX_FMT_SRGGB10P, 0, camera->nbufs) < 0) {
    return -1;
  }

  buffer_list_t *src = camera->camera->capture_list;

  camera->legacy_isp.isp = device_open("ISP", "/dev/video12");
  camera->codec_jpeg = device_open("JPEG", "/dev/video31");
  camera->codec_h264 = device_open("H264", "/dev/video11");

  if (device_open_buffer_list(camera->legacy_isp.isp, false, src->fmt_width, src->fmt_height, src->fmt_format, src->fmt_bytesperline, camera->nbufs) < 0 ||
    device_open_buffer_list(camera->legacy_isp.isp, true, src->fmt_width / div, src->fmt_height / div, V4L2_PIX_FMT_YUYV, 0, camera->nbufs) < 0) {
    return -1;
  }

  src = camera->legacy_isp.isp->capture_list;

  if (device_open_buffer_list(camera->codec_jpeg, false, src->fmt_width, src->fmt_height, src->fmt_format, src->fmt_bytesperline, camera->nbufs) < 0 ||
    device_open_buffer_list(camera->codec_jpeg, true, src->fmt_width, src->fmt_height, V4L2_PIX_FMT_JPEG, 0, camera->nbufs) < 0) {
    return -1;
  }

  if (device_open_buffer_list(camera->codec_h264, false, src->fmt_width, src->fmt_height, src->fmt_format, src->fmt_bytesperline, camera->nbufs) < 0 ||
    device_open_buffer_list(camera->codec_h264, true, src->fmt_width, src->fmt_height, V4L2_PIX_FMT_H264, 0, camera->nbufs) < 0) {
    return -1;
  }

  link_t *links = camera->links;

  *links++ = (link_t){ camera->camera, { camera->legacy_isp.isp }, { NULL, check_streaming } };
  *links++ = (link_t){ camera->legacy_isp.isp, { camera->codec_jpeg, camera->codec_h264 }, { write_yuvu, NULL } };
  *links++ = (link_t){ camera->codec_jpeg, { }, { http_jpeg_capture, http_jpeg_needs_buffer } };
  *links++ = (link_t){ camera->codec_h264, { }, { http_h264_capture, http_h264_needs_buffer } };
  return 0;
}