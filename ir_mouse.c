#include <stdlib.h>
#include "wmplugin.h"

#define DEBOUNCE_THRESHOLD	50
#define NEW_AMOUNT	0.3
#define OLD_AMOUNT	(1.0 - NEW_AMOUNT)

#define X_EDGE		CWIID_IR_X_MAX / 8
#define X_MIN		X_EDGE
#define X_MAX		( CWIID_IR_X_MAX - X_EDGE )

#define Y_EDGE		CWIID_IR_Y_MAX / 8
#define Y_MIN		Y_EDGE
#define Y_MAX		( CWIID_IR_Y_MAX - Y_EDGE )

#define SENSITIVITY 5


cwiid_wiimote_t *wiimote;

static struct wmplugin_info info;
static struct wmplugin_data data;

wmplugin_info_t wmplugin_info;
wmplugin_init_t wmplugin_init;
wmplugin_exec_t wmplugin_exec;

struct wmplugin_info *wmplugin_info() {
	static unsigned char info_init = 0;

	if (!info_init) {
		info.button_count = 0;
		info.axis_count = 2;
		info.axis_info[0].name = "X";
		info.axis_info[0].type = WMPLUGIN_REL;
		info.axis_info[0].max  = CWIID_IR_X_MAX - X_EDGE;
		info.axis_info[0].min  = X_EDGE;
		info.axis_info[0].fuzz = 0;
		info.axis_info[0].flat = 0;
		info.axis_info[1].name = "Y";
		info.axis_info[1].type = WMPLUGIN_REL;
		info.axis_info[1].max  = CWIID_IR_Y_MAX - Y_EDGE;
		info.axis_info[1].min  = Y_EDGE;
		info.axis_info[1].fuzz = 0;
		info.axis_info[1].flat = 0;
		info.param_count = 0;
		info_init = 1;
	}
	return &info;
}

int wmplugin_init(int id, cwiid_wiimote_t *arg_wiimote)
{
	wiimote = arg_wiimote;

	data.buttons = 0;

	if (wmplugin_set_rpt_mode(id, CWIID_RPT_IR)) {
		return -1;
	}

	return 0;
}



double oldX = 0;
double oldY = 0;


struct wmplugin_data *wmplugin_exec(int mesg_count, union cwiid_mesg mesg[], struct timespec *timestamp)
{
  (void) timestamp;
  static int src_index = -1;
  static int debounce = 0;
  //static uint8_t old_flag;

  int i;
  //uint8_t flag;
  struct cwiid_ir_mesg *ir_mesg;

  double posX, posY;
  double newX, newY;

  ir_mesg = NULL;
  for (i=0; i < mesg_count; i++) {
    if (mesg[i].type == CWIID_MESG_IR) {
      ir_mesg = &mesg[i].ir_mesg;
      break;
    }
  }

  if (!ir_mesg) {
    return NULL;
  }

  /* invalidate src index if source is no longer present */
  if ((src_index != -1) && !ir_mesg->src[src_index].valid) {
    if (debounce > DEBOUNCE_THRESHOLD) {
      src_index = -1;
    }
    else {
      debounce++;
    }
  }
  else {
    debounce = 0;
  }

  /* of not set, pick largest available source */
  if (src_index == -1) {
    for (i=0; i < CWIID_IR_SRC_COUNT; i++) {
      if (ir_mesg->src[i].valid) {
        if ( (src_index == -1) ||
             (ir_mesg->src[i].size > ir_mesg->src[src_index].size) ) {
          src_index = i;
        }
      }
    }
  }

  /* invalidate with no source */
  if ((src_index == -1) || !ir_mesg->src[src_index].valid) {
    data.axes[0].valid = data.axes[1].valid = 0;
  }

  else {
//		newX = NEW_AMOUNT * (CWIID_IR_X_MAX -
//		                         ir_mesg->src[src_index].pos[CWIID_X])
//		                   + OLD_AMOUNT * data.axes[0].value;
//		newY = NEW_AMOUNT * ir_mesg->src[src_index].pos[CWIID_Y]
//		                   + OLD_AMOUNT * data.axes[1].value;

    posX = CWIID_IR_X_MAX - ir_mesg->src[src_index].pos[CWIID_X];
    posY = ir_mesg->src[src_index].pos[CWIID_Y];

    newX = NEW_AMOUNT * posX + OLD_AMOUNT * oldX;
    newY = NEW_AMOUNT * posY + OLD_AMOUNT * oldY;

/*
		if (newX > CWIID_IR_X_MAX - X_EDGE) {
			newX = CWIID_IR_X_MAX - X_EDGE;
		}
		else if (newX < X_EDGE) {
			newX = X_EDGE;
		}
		if (newY > CWIID_IR_Y_MAX - Y_EDGE) {
			newY = CWIID_IR_Y_MAX - Y_EDGE;
		}
		else if (newY < Y_EDGE) {
			newY = Y_EDGE;
		}
 */

    if ( abs(newX - oldX) > SENSITIVITY ) {
      data.axes[0].valid = 1;
      data.axes[0].value = newX - oldX;
      oldX = newX;
    } else {
      data.axes[0].valid = 1;
      if ( ( posX > X_MIN ) && ( posX < X_MAX ) )
        data.axes[0].value /= 2;
    }

    if ( abs(newY - oldY) > SENSITIVITY ) {
      data.axes[1].valid = 1;
      data.axes[1].value = newY - oldY;
      oldY = newY;
    } else {
      data.axes[1].valid = 1;
      if ( ( posY > Y_MIN ) && ( posY < Y_MAX ) )
        data.axes[1].value /= 2;
    }

  }

  return &data;
}
