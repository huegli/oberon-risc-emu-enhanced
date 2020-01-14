// Translate SDL scancodes to PS/2 codeset 2 scancodes.

#include <rfb/rfbproto.h>
#include <rfb/keysym.h>
#include "sdl-ps2.h"
#include "rfb-ps2.h"

struct k_info {
  unsigned char code;
  unsigned char type;
};

enum k_type {
  K_UNKNOWN = 0,
  K_NORMAL,
  K_EXTENDED,
  K_NUMLOCK_HACK,
  K_SHIFT_HACK,
};

static struct k_info rfb_keymap[128];


int rfb_ps2_encode(rfbKeySym key, bool make, uint8_t out[static MAX_PS2_CODE_LEN]) {
  int i = 0;
  struct k_info info = rfb_keymap[key];
  switch (info.type) {
    case K_UNKNOWN: {
      break;
    }

    case K_NORMAL: {
      if (!make) {
        out[i++] = 0xF0;
      }
      out[i++] = info.code;
      break;
    }

    case K_EXTENDED: {
      out[i++] = 0xE0;
      if (!make) {
        out[i++] = 0xF0;
      }
      out[i++] = info.code;
      break;
    }

    case K_NUMLOCK_HACK: {
      // This assumes Num Lock is always active
      if (make) {
        // fake shift press
        out[i++] = 0xE0;
        out[i++] = 0x12;
        out[i++] = 0xE0;
        out[i++] = info.code;
      } else {
        out[i++] = 0xE0;
        out[i++] = 0xF0;
        out[i++] = info.code;
        // fake shift release
        out[i++] = 0xE0;
        out[i++] = 0xF0;
        out[i++] = 0x12;
      }
      break;
    }

//    case K_SHIFT_HACK: {
//      SDL_Keymod mod = SDL_GetModState();
//      if (make) {
//        // fake shift release
//        if (mod & KMOD_LSHIFT) {
//          out[i++] = 0xE0;
//          out[i++] = 0xF0;
//          out[i++] = 0x12;
//        }
//        if (mod & KMOD_RSHIFT) {
//          out[i++] = 0xE0;
//          out[i++] = 0xF0;
//          out[i++] = 0x59;
//        }
//        out[i++] = 0xE0;
//        out[i++] = info.code;
//      } else {
//        out[i++] = 0xE0;
//        out[i++] = 0xF0;
//        out[i++] = info.code;
//        // fake shift press
//        if (mod & KMOD_RSHIFT) {
//          out[i++] = 0xE0;
//          out[i++] = 0x59;
//        }
//        if (mod & KMOD_LSHIFT) {
//          out[i++] = 0xE0;
//          out[i++] = 0x12;
//        }
//      }
//      break;
//    }
  }
  return i;
}

static struct k_info rfb_keymap[128] = {

  [XK_1] = { 0x16, K_NORMAL },
  [XK_2] = { 0x1E, K_NORMAL },
  [XK_3] = { 0x26, K_NORMAL },
  [XK_4] = { 0x25, K_NORMAL },
  [XK_5] = { 0x2E, K_NORMAL },
  [XK_6] = { 0x36, K_NORMAL },
  [XK_7] = { 0x3D, K_NORMAL },
  [XK_8] = { 0x3E, K_NORMAL },
  [XK_9] = { 0x46, K_NORMAL },
  [XK_0] = { 0x45, K_NORMAL },


  [XK_A] = { 0x1C, K_NORMAL },
  [XK_B] = { 0x32, K_NORMAL },
  [XK_C] = { 0x21, K_NORMAL },
  [XK_D] = { 0x23, K_NORMAL },
  [XK_E] = { 0x24, K_NORMAL },
  [XK_F] = { 0x2B, K_NORMAL },
  [XK_G] = { 0x34, K_NORMAL },
  [XK_H] = { 0x33, K_NORMAL },
  [XK_I] = { 0x43, K_NORMAL },
  [XK_J] = { 0x3B, K_NORMAL },
  [XK_K] = { 0x42, K_NORMAL },
  [XK_L] = { 0x4B, K_NORMAL },
  [XK_M] = { 0x3A, K_NORMAL },
  [XK_N] = { 0x31, K_NORMAL },
  [XK_O] = { 0x44, K_NORMAL },
  [XK_P] = { 0x4D, K_NORMAL },
  [XK_Q] = { 0x15, K_NORMAL },
  [XK_R] = { 0x2D, K_NORMAL },
  [XK_S] = { 0x1B, K_NORMAL },
  [XK_T] = { 0x2C, K_NORMAL },
  [XK_U] = { 0x3C, K_NORMAL },
  [XK_V] = { 0x2A, K_NORMAL },
  [XK_W] = { 0x1D, K_NORMAL },
  [XK_X] = { 0x22, K_NORMAL },
  [XK_Y] = { 0x35, K_NORMAL },
  [XK_Z] = { 0x1A, K_NORMAL },

  [XK_a] = { 0x1C, K_NORMAL },
  [XK_b] = { 0x32, K_NORMAL },
  [XK_c] = { 0x21, K_NORMAL },
  [XK_d] = { 0x23, K_NORMAL },
  [XK_e] = { 0x24, K_NORMAL },
  [XK_f] = { 0x2B, K_NORMAL },
  [XK_g] = { 0x34, K_NORMAL },
  [XK_h] = { 0x33, K_NORMAL },
  [XK_i] = { 0x43, K_NORMAL },
  [XK_j] = { 0x3B, K_NORMAL },
  [XK_k] = { 0x42, K_NORMAL },
  [XK_l] = { 0x4B, K_NORMAL },
  [XK_m] = { 0x3A, K_NORMAL },
  [XK_n] = { 0x31, K_NORMAL },
  [XK_o] = { 0x44, K_NORMAL },
  [XK_p] = { 0x4D, K_NORMAL },
  [XK_q] = { 0x15, K_NORMAL },
  [XK_r] = { 0x2D, K_NORMAL },
  [XK_s] = { 0x1B, K_NORMAL },
  [XK_t] = { 0x2C, K_NORMAL },
  [XK_u] = { 0x3C, K_NORMAL },
  [XK_v] = { 0x2A, K_NORMAL },
  [XK_w] = { 0x1D, K_NORMAL },
  [XK_x] = { 0x22, K_NORMAL },
  [XK_y] = { 0x35, K_NORMAL },
  [XK_z] = { 0x1A, K_NORMAL },


  // [XK_RETURN]    = { 0x5A, K_NORMAL },
  // [XK_ESCAPE]    = { 0x76, K_NORMAL },
  // [XK_BACKSPACE] = { 0x66, K_NORMAL },
  // [XK_TAB]       = { 0x0D, K_NORMAL },
  [XK_space]     = { 0x29, K_NORMAL },

  [XK_minus]        = { 0x4E, K_NORMAL },
  [XK_equal]        = { 0x55, K_NORMAL },
  //[XK_leftbracket]  = { 0x54, K_NORMAL },
  //[XK_rightbracket] = { 0x5B, K_NORMAL },
  [XK_backslash]    = { 0x5D, K_NORMAL },
  // [XK__NONUSHASH]    = { 0x5D, K_NORMAL },  // same key as BACKSLASH

  [XK_semicolon]  = { 0x4C, K_NORMAL },
  [XK_apostrophe] = { 0x52, K_NORMAL },
  //[XK_grave]      = { 0x0E, K_NORMAL },
  [XK_comma]      = { 0x41, K_NORMAL },
  [XK_period]     = { 0x49, K_NORMAL },
  [XK_slash]      = { 0x4A, K_NORMAL },

  // [XK__F1]  = { 0x05, K_NORMAL },
  // [XK__F2]  = { 0x06, K_NORMAL },
  //[XK__F3]  = { 0x04, K_NORMAL },
  // [XK__F4]  = { 0x0C, K_NORMAL },
  //[XK__F5]  = { 0x03, K_NORMAL },
  //[XK__F6]  = { 0x0B, K_NORMAL },
  //[XK__F7]  = { 0x83, K_NORMAL },
  //[XK__F8]  = { 0x0A, K_NORMAL },
  //[XK__F9]  = { 0x01, K_NORMAL },
  //[XK__F10] = { 0x09, K_NORMAL },
  //[XK__F11] = { 0x78, K_NORMAL },
  //[XK__F12] = { 0x07, K_NORMAL },

  // Most of the keys below are not used by Oberon

  //[XK__INSERT]   = { 0x70, K_NUMLOCK_HACK },
  //[XK__HOME]     = { 0x6C, K_NUMLOCK_HACK },
  //[XK__PAGEUP]   = { 0x7D, K_NUMLOCK_HACK },
  //[XK__DELETE]   = { 0x71, K_NUMLOCK_HACK },
  //[XK__END]      = { 0x69, K_NUMLOCK_HACK },
  //[XK__PAGEDOWN] = { 0x7A, K_NUMLOCK_HACK },
  //[XK__RIGHT]    = { 0x74, K_NUMLOCK_HACK },
  //[XK__LEFT]     = { 0x6B, K_NUMLOCK_HACK },
  //[XK__DOWN]     = { 0x72, K_NUMLOCK_HACK },
  //[XK__UP]       = { 0x75, K_NUMLOCK_HACK },

  //[XK__KP_DIVIDE]   = { 0x4A, K_SHIFT_HACK },
  //[XK__KP_MULTIPLY] = { 0x7C, K_NORMAL },
  //[XK__KP_MINUS]    = { 0x7B, K_NORMAL },
  //[XK__KP_PLUS]     = { 0x79, K_NORMAL },
  //[XK__KP_ENTER]    = { 0x5A, K_EXTENDED },
  //[XK__KP_1]        = { 0x69, K_NORMAL },
  //[XK__KP_2]        = { 0x72, K_NORMAL },
  //[XK__KP_3]        = { 0x7A, K_NORMAL },
  //[XK__KP_4]        = { 0x6B, K_NORMAL },
  //[XK__KP_5]        = { 0x73, K_NORMAL },
  //[XK__KP_6]        = { 0x74, K_NORMAL },
  //[XK__KP_7]        = { 0x6C, K_NORMAL },
  //[XK__KP_8]        = { 0x75, K_NORMAL },
  //[XK__KP_9]        = { 0x7D, K_NORMAL },
  //[XK__KP_0]        = { 0x70, K_NORMAL },
  //[XK__KP_PERIOD]   = { 0x71, K_NORMAL },

  //[XK__NONUSBACKSLASH] = { 0x61, K_NORMAL },
  //[XK__APPLICATION]    = { 0x2F, K_EXTENDED },

  //[XK__LCTRL]  = { 0x14, K_NORMAL },
  //[XK__LSHIFT] = { 0x12, K_NORMAL },
  //[XK__LALT]   = { 0x11, K_NORMAL },
  //[XK__LGUI]   = { 0x1F, K_EXTENDED },
  //[XK__RCTRL]  = { 0x14, K_EXTENDED },
  //[XK__RSHIFT] = { 0x59, K_NORMAL },
  //[XK__RALT]   = { 0x11, K_EXTENDED },
  //[XK__RGUI]   = { 0x27, K_EXTENDED },
};
