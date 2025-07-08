#include <signal.h>
#include <sys/mman.h>
#include <poll.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <drm_fourcc.h>
#include <fcntl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "peanut.h"
#include "drm.h"
#include "lcd.h"
#include "main.h"

volatile sig_atomic_t stop = 0; /* programms runs as long as this is unset */

bool pollDrmThreadStarted = false;
pthread_t pollDrmThread;
static void *poll_drm_thread(void *data);

static uint32_t find_crtc(int drm_fd, drmModeRes *res, drmModeConnector *conn, uint32_t *taken_crtcs);

drmModeConnector *drm_conn = NULL;
uint32_t crtc = 0;
int drm_fd = -1;


uint8_t *fb0_data = NULL;
uint8_t *fb1_data = NULL;
uint8_t *back_fb_data = NULL;
uint8_t *front_fb_data = NULL;

struct drm_mode_create_dumb fb0;
struct drm_mode_create_dumb fb1;
uint32_t fb0_id;
uint32_t fb1_id;
uint32_t back_fb;
uint32_t front_fb;

uint_fast32_t frameCount = 0;

volatile sig_atomic_t waitForFlip = 0;
drmModeCrtc *saved = NULL;

static void page_flip_handler(int drm_fd, unsigned sequence, unsigned tv_sec,	unsigned tv_usec, void *data)
{
  uint32_t tmp_fb = front_fb;
  front_fb = back_fb;
  back_fb = tmp_fb;

  uint8_t *tmp_fb_data = front_fb_data;
  front_fb_data = back_fb_data;
  back_fb_data = tmp_fb_data;

  waitForFlip = 0;
}

/* all the drawing functions */
#include "drm_draw.c"


/* false = failed | true = success */
bool setup_drm(void){


  drm_fd = open("/dev/dri/card0", O_RDWR | O_NONBLOCK);
  if (drm_fd < 0){
    perror("/dev/dri/card0 error");
    return false;
  }
   LOGR("ALLOC: DRMMASTER",1); /* just assume we have master for now */
  
  /* get drm resources */

  drmModeRes *resources = drmModeGetResources(drm_fd);
  LOGR("ALLOC: DRMMODERES",1);
  if (resources == NULL) {
    perror("drmModeGetResources error");
    goto drm_resources_cleanup;
  }
  if (resources->count_connectors == 0){
    puts("no connectors!");
    goto drm_resources_cleanup;
  }

  if (resources->count_connectors > 1){
    puts("more connectors than expected! programm will only try to use the first one.");
  }


  /* get connector */

  //struct connector *conn = NULL;
  drm_conn = drmModeGetConnector(drm_fd, resources->connectors[0]);
  LOGR("ALLOC: DRMCONNECTOR",1);
  if (drm_conn == NULL){
    puts("error: connector was NULL!");
    goto drm_resources_cleanup;
  }

  if (drm_conn->count_modes == 0){
    puts("error: no valid display modes");
    goto drm_resources_conn_cleanup;
  }

  if (drm_conn->connection != DRM_MODE_CONNECTED){
    puts("error connector is not connected!");
    goto drm_resources_conn_cleanup;
  }

  /* check resolution */
  if (drm_conn->modes[0].hdisplay != H_RES || drm_conn->modes[0].vdisplay != V_RES){
    printf("ERROR: resulution is not what is expected.\n\tExpected: %d x %d\n\tGot     : %d x %x\n",H_RES, V_RES, drm_conn->modes[0].hdisplay, drm_conn->modes[0].vdisplay);
    goto drm_resources_conn_cleanup;
  }


  /* get crtc */
  uint32_t taken_crtcs = 0;
  crtc = find_crtc(drm_fd,resources,drm_conn,&taken_crtcs);
  if (crtc == 0){
    puts("no crtc found!");
    goto drm_resources_conn_cleanup;
  }

  /* framebuffer 0 */
  fb0.width = H_RES;
  fb0.height = V_RES;
  fb0.bpp = BYTES_PER_PIXEL * 8;

  drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &fb0);
  LOGR("ALLOC: FB0_D",1);
 
  {
    uint32_t handles[4] = { fb0.handle };
    uint32_t strides[4] = { fb0.pitch };
    uint32_t offsets[4] = { 0 };
    int ret = drmModeAddFB2(drm_fd, H_RES, V_RES, DRM_FORMAT_RGB565, handles, strides, offsets, &fb0_id, 0);
    if (ret != 0){
      perror("drmModeAddFB2 failed to create failbuffer0");
      goto drm_resources_conn_fb0_dumb_cleanup;
    }
  }

 /* prepare mapping fb0*/

  {
    struct drm_mode_map_dumb map = { .handle = fb0.handle };
    int ret = drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map);
    LOGR("ALLOC: FB0_M",1);
    if (ret < 0) {
      perror("DRM_IOCTL_MODE_MAP_DUMB failed to preapare map0");
      goto drm_resources_conn_fb0_cleanup;
    }

    fb0_data = mmap(0, fb0.size, PROT_READ | PROT_WRITE, MAP_SHARED,
            drm_fd, map.offset);
    if (fb0_data == 0) {
      perror("mmap0 failed");
      goto drm_resources_conn_fb0_cleanup;
    }

    memset(fb0_data, 0xff, fb0.size);
  }

  /* framebuffer 1 */
  fb1.width = H_RES;
  fb1.height = V_RES;
  fb1.bpp = BYTES_PER_PIXEL * 8;

  drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &fb1);
  LOGR("ALLOC: FB1_D",1);
  {
    uint32_t handles[4] = { fb1.handle };
    uint32_t strides[4] = { fb1.pitch };
    uint32_t offsets[4] = { 0 };
    int ret = drmModeAddFB2(drm_fd, H_RES, V_RES, DRM_FORMAT_RGB565, handles, strides, offsets, &fb1_id, 0);
    if (ret != 0){
      perror("drmModeAddFB2 failed to create failbuffer1");
      goto drm_resources_conn_fb0_fb1_dumb_cleanup;
    }
  }

  /* prepare mapping fb1*/
  
  {
    struct drm_mode_map_dumb map = { .handle = fb1.handle };
    int ret = drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map);
    LOGR("ALLOC: FB1_M",1);
    if (ret < 0) {
      perror("DRM_IOCTL_MODE_MAP_DUMB failed to preapare map1");
      goto drm_resources_conn_fb0_fb1_cleanup;
    }

    fb1_data = mmap(0, fb1.size, PROT_READ | PROT_WRITE, MAP_SHARED,
      drm_fd, map.offset);
    if (fb1_data == 0) {
      perror("mmap1 failed");
      goto drm_resources_conn_fb0_fb1_cleanup;
    }

    memset(fb1_data, 0xff, fb1.size);
  }

  /* front / back set */
  front_fb_data = fb1_data;
  back_fb_data = fb0_data;
  front_fb = fb1_id;
  back_fb = fb0_id;

  /* save old connection and perfom the modeset*/
  saved = drmModeGetCrtc(drm_fd, crtc);

  int ret = drmModeSetCrtc(drm_fd, crtc, fb1_id, 0, 0, &drm_conn->connector_id, 1, &drm_conn->modes[0]);
  if (ret < 0) {
    perror("drmModeSetCrtc could net set the mode");
  }


  /* clean the mode resources */
  drmModeFreeResources(resources); 
  LOGR("CLEAN: DRMMODERES",-1);

  /* polling for Flip handler*/

    if(pthread_create( &pollDrmThread, NULL, poll_drm_thread,NULL) != 0){
      cleanup_drm();
      return false; 
    }
    pollDrmThreadStarted = true;

  /* done, now clean and return */

  return true;

  /**** cleanup only reachable with goto: *******/

drm_resources_conn_fb0_fb1_cleanup:
  drmModeRmFB(drm_fd, fb1_id); LOGR("CLEAN: FB1_M",-1);
drm_resources_conn_fb0_fb1_dumb_cleanup:
  {  
    struct drm_mode_destroy_dumb destroy = { .handle = fb1.handle };
    drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
  }
  LOGR("CLEAN: FB1_D",-1);
drm_resources_conn_fb0_cleanup:
  drmModeRmFB(drm_fd, fb0_id); LOGR("CLEAN: FB0_M",-1);
drm_resources_conn_fb0_dumb_cleanup:
  {
    struct drm_mode_destroy_dumb destroy = { .handle = fb0.handle };
    drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
  }
  LOGR("CLEAN: FB0_D",-1);
drm_resources_conn_cleanup:
  drmModeFreeConnector(drm_conn); LOGR("CLEAN: DRMCONNECTOR",-1);
drm_resources_cleanup:
  drmModeFreeResources(resources); LOGR("CLEAN: DRMMODERES",-1);

  return false;
}

void cleanup_drm(void){

  /* polling */
  if (pollDrmThreadStarted){
    pthread_join(pollDrmThread,NULL);
    LOGR("THEAD END: POLLDRM",-1);
    pollDrmThreadStarted = false;
  }

  /* fb0 */

  if (fb0_data != 0){
    munmap(fb0_data, fb0.size); fb0_data = NULL;
    drmModeRmFB(drm_fd, fb0_id);
    struct drm_mode_destroy_dumb destroy = { .handle = fb0.handle };
    drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    LOGR("CLEAN: FB0_D",-1);
    LOGR("CLEAN: FB0_M",-1);
  }

  /* fb1 */

  if (fb1_data != 0){
    munmap(fb1_data, fb1.size); fb1_data = NULL;
    drmModeRmFB(drm_fd, fb1_id);
    struct drm_mode_destroy_dumb destroy = { .handle = fb1.handle };
    drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    LOGR("CLEAN: FB1_D",-1);
    LOGR("CLEAN: FB1_M",-1);
  }

  /* restore saved mode */

  if (saved != NULL){
    drmModeSetCrtc(drm_fd, saved->crtc_id, saved->buffer_id,saved->x, saved->y, &drm_conn->connector_id, 1, &saved->mode);
    drmModeFreeCrtc(saved); saved = NULL;
    puts("\trestored saved mode");
  }

  /* connector */

  if (drm_conn != NULL){
    drmModeFreeConnector(drm_conn); drm_conn = NULL;
    LOGR("CLEAN: DRMCONNECTOR",-1);
  }

  /* drop master */
  drmDropMaster(drm_fd); 
  LOGR("CLEAN: DRMMASTER",-1);
}


/* borrowed code from: https://github.com/ascent12/drm_doc/blob/master/02_modesetting/src/main.c#L46
 see LICENSE_DRM_DOC
*/

static uint32_t find_crtc(int drm_fd, drmModeRes *res, drmModeConnector *conn,
                uint32_t *taken_crtcs)
{
        for (int i = 0; i < conn->count_encoders; ++i) {
                drmModeEncoder *enc = drmModeGetEncoder(drm_fd, conn->encoders[i]);
                if (!enc)
                        continue;

                for (int i = 0; i < res->count_crtcs; ++i) {
                        uint32_t bit = 1 << i;
                        // Not compatible
                        if ((enc->possible_crtcs & bit) == 0)
                                continue;

                        // Already taken
                        if (*taken_crtcs & bit)
                                continue;

                        drmModeFreeEncoder(enc);
                        *taken_crtcs |= bit;
                        return res->crtcs[i];
                }

                drmModeFreeEncoder(enc);
        }

        return 0;
}


static void *poll_drm_thread(void *data){
  
  LOGR("THREAD CREATE: POLLDRM",1);

  struct pollfd pollfd = {
		.fd = drm_fd,
		.events = POLLIN,
	};

  while(!stop){
    int ret = poll(&pollfd, 1, 250);
    if (ret < 0 && errno != EAGAIN) {
      perror("poll");
    }

    if (pollfd.revents & POLLIN) {
      drmEventContext context = {
        .version = DRM_EVENT_CONTEXT_VERSION,
        .page_flip_handler = page_flip_handler,
      };

      if (drmHandleEvent(drm_fd, &context) < 0) {
        perror("drmHandleEvent");
      }
    }
  }
}


