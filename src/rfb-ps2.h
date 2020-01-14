#ifndef RFB_PS2_H
#define RFB_PS2_H

#include <stdbool.h>
#include <stdint.h>
#include <rfb/rfbproto.h>

#include "sdl-ps2.h"

int rfb_ps2_encode(rfbKeySym key, bool make, uint8_t out[static MAX_PS2_CODE_LEN]);

#endif  // RFB_PS2_H
