#include "device/v4l2/v4l2.h"
#include "device/hw/v4l2.h"
#include "device/hw/device.h"

int v4l2_device_open(device_t *dev)
{
  dev->fd = -1;
  dev->subdev_fd = -1;

  dev->fd = open(dev->path, O_RDWR|O_NONBLOCK);
  if (dev->fd < 0) {
		E_LOG_ERROR(dev, "Can't open device: %s", dev->path);
    goto error;
  }

	E_LOG_DEBUG(dev, "Querying device capabilities ...");
  struct v4l2_capability v4l2_cap;
  E_XIOCTL(dev, dev->fd, VIDIOC_QUERYCAP, &v4l2_cap, "Can't query device capabilities");

	if (!(v4l2_cap.capabilities & V4L2_CAP_STREAMING)) {
		E_LOG_ERROR(dev, "Device doesn't support streaming IO");
	}

  strcpy(dev->bus_info, v4l2_cap.bus_info);
  dev->subdev_fd = v4l2_device_open_v4l2_subdev(dev, 0);

  return 0;

error:
  return -1;
}

void v4l2_device_close(device_t *dev)
{
  if (dev->subdev_fd >= 0) {
    close(dev->subdev_fd);
  }

  if(dev->fd >= 0) {
    close(dev->fd);
  }

}

int v4l2_device_set_decoder_start(device_t *dev, bool do_on)
{
  struct v4l2_decoder_cmd cmd = {0};

  cmd.cmd = do_on ? V4L2_DEC_CMD_START : V4L2_DEC_CMD_STOP;

  E_LOG_DEBUG(dev, "Setting decoder state %s...", do_on ? "Start" : "Stop");
  E_XIOCTL(dev, dev->fd, VIDIOC_DECODER_CMD, &cmd, "Cannot set decoder state");

  return 0;

error:
  return -1;
}

int v4l2_device_video_force_key(device_t *dev)
{
  struct v4l2_control ctl = {0};
  ctl.id = V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME;
  ctl.value = 1;
  E_LOG_DEBUG(dev, "Forcing keyframe ...");
  E_XIOCTL(dev, dev->fd, VIDIOC_S_CTRL, &ctl, "Can't force keyframe");
  return 0;

error:
  return -1;
}

int v4l2_device_set_fps(device_t *dev, int desired_fps)
{
  struct v4l2_streamparm setfps = {0};

  if (!dev) {
    return -1;
  }

  setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  setfps.parm.output.timeperframe.numerator = 1;
  setfps.parm.output.timeperframe.denominator = desired_fps;
  E_LOG_DEBUG(dev, "Configuring FPS ...");
  E_XIOCTL(dev, dev->fd, VIDIOC_S_PARM, &setfps, "Can't set FPS");
  return 0;
error:
  return -1;
}
