#include <SDL.h>
#include <rfb/rfb.h>
#include <rfb/keysym.h>
#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "risc.h"
#include "risc-io.h"
#include "disk.h"
#include "pclink.h"
#include "raw-serial.h"
#include "sdl-ps2.h"
#include "sdl-clipboard.h"
#include "rfb-ps2.h"

#define CPU_HZ 25000000
#define FPS 60

static uint32_t BLACK = 0x657b83, WHITE = 0xfdf6e3;
//static uint32_t BLACK = 0x000000, WHITE = 0xFFFFFF;
//static uint32_t BLACK = 0x0000FF, WHITE = 0xFFFF00;
//static uint32_t BLACK = 0x000000, WHITE = 0x00FF00;

#define MAX_HEIGHT 2048
#define MAX_WIDTH  2048

static int best_display(const SDL_Rect *rect);
static int clamp(int x, int min, int max);
static enum Action map_keyboard_event(SDL_KeyboardEvent *event);
static void show_leds(const struct RISC_LED *leds, uint32_t value);
static double scale_display(SDL_Window *window, const SDL_Rect *risc_rect, SDL_Rect *display_rect);
static void update_texture(struct RISC *risc, SDL_Texture *texture, const SDL_Rect *risc_rect, bool color);
static void update_rfb(struct RISC *risc, rfbScreenInfoPtr screen, bool color);
static void doptr(int buttonMask,int x,int y,rfbClientPtr cl);
static void dokey(rfbBool down,rfbKeySym key,rfbClientPtr cl);

enum Action {
  ACTION_OBERON_INPUT,
  ACTION_QUIT,
  ACTION_RESET,
  ACTION_TOGGLE_FULLSCREEN,
  ACTION_FAKE_MOUSE1,
  ACTION_FAKE_MOUSE2,
  ACTION_FAKE_MOUSE3
};

struct KeyMapping {
  int state;
  SDL_Keycode sym;
  SDL_Keymod mod1, mod2;
  enum Action action;
};

struct KeyMapping key_map[] = {
  { SDL_PRESSED,  SDLK_F4,     KMOD_ALT, 0,           ACTION_QUIT },
  { SDL_PRESSED,  SDLK_F12,    0, 0,                  ACTION_RESET },
  { SDL_PRESSED,  SDLK_DELETE, KMOD_CTRL, KMOD_SHIFT, ACTION_RESET },
  { SDL_PRESSED,  SDLK_F11,    0, 0,                  ACTION_TOGGLE_FULLSCREEN },
  { SDL_PRESSED,  SDLK_RETURN, KMOD_ALT, 0,           ACTION_TOGGLE_FULLSCREEN },
  { SDL_PRESSED,  SDLK_f,      KMOD_GUI, KMOD_SHIFT,  ACTION_TOGGLE_FULLSCREEN },  // Mac?
  { SDL_PRESSED,  SDLK_LALT,   0, 0,                  ACTION_FAKE_MOUSE2 },
  { SDL_RELEASED, SDLK_LALT,   0, 0,                  ACTION_FAKE_MOUSE2 },
};

static struct option long_options[] = {
  { "zoom",             required_argument, NULL, 'z' },
  { "fullscreen",       no_argument,       NULL, 'f' },
  { "leds",             no_argument,       NULL, 'L' },
  { "rtc",              no_argument,       NULL, 'r' },
  { "mem",              required_argument, NULL, 'm' },
  { "size",             required_argument, NULL, 's' },
  { "serial-in",        required_argument, NULL, 'I' },
  { "serial-out",       required_argument, NULL, 'O' },
  { "boot-from-serial", no_argument,       NULL, 'S' },
  { "color",            no_argument,       NULL, 'c' },
  { "hostfs",           required_argument, NULL, 'H' },
  { "vnc",              no_argument,       NULL, 'v' },
  { "headless",         no_argument,       NULL, 'h' },
  { NULL,               no_argument,       NULL, 0   }
};

static void fail(int code, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  exit(code);
}

static void usage() {
  puts("Usage: risc [OPTIONS...] DISK-IMAGE\n"
       "\n"
       "Options:\n"
       "  --fullscreen          Start the emulator in full screen mode\n"
       "  --zoom REAL           Scale the display in windowed mode\n"
       "  --leds                Log LED state on stdout\n"
       "  --mem MEGS            Set memory size\n"
       "  --color               Use 16 color mode (requires modified Display.Mod)\n"
       "  --size WIDTHxHEIGHT   Set framebuffer size\n"
       "  --boot-from-serial    Boot from serial line (disk image not required)\n"
       "  --serial-in FILE      Read serial input from FILE\n"
       "  --serial-out FILE     Write serial output to FILE\n"
       "  --hostfs DIRECTORY    Use DIRECTORY as HostFS directory\n"
       "  --vnc                 Set up VNC server for display access\n"
       "  --headless.           Disable display (impliess --vnc)\n"
       );
  exit(1);
}

/* make global as VNC keyboard and mouse handlers require it */
static struct RISC *risc;
static SDL_Rect risc_rect;

int main (int argc, char *argv[]) {
  risc = risc_new();
  risc_set_serial(risc, &pclink);
  risc_set_clipboard(risc, &sdl_clipboard);

  rfbScreenInfoPtr rfbScreen = NULL;

  struct RISC_LED leds = {
    .write = show_leds
  };

  bool fullscreen = false;
  double zoom = 0;
  
  risc_rect.w = RISC_FRAMEBUFFER_WIDTH;
  risc_rect.h = RISC_FRAMEBUFFER_HEIGHT;
  
  bool size_option = false, rtc_option = false, color_option = false;
  int mem_option = 0;
  const char *serial_in = NULL;
  const char *serial_out = NULL;
  bool boot_from_serial = false;
  bool use_VNC = false;
  bool use_SDL = true;
  
  int opt;
  while ((opt = getopt_long(argc, argv, "z:fLrm:s:I:O:ScHvh:", long_options, NULL)) != -1) {
    switch (opt) {
      case 'z': {
        double x = strtod(optarg, 0);
        if (x > 0) {
          zoom = x;
        }
        break;
      }
      case 'f': {
        fullscreen = true;
        break;
      }
      case 'L': {
        risc_set_leds(risc, &leds);
        break;
      }
      case 'r': {
        rtc_option = true;
        break;
      }
      case 'm': {
        if (sscanf(optarg, "%d", &mem_option) != 1) {
          usage();
        }
        break;
      }
      case 's': {
        int w, h;
        if (sscanf(optarg, "%dx%d", &w, &h) != 2) {
          usage();
        }
        risc_rect.w = clamp(w, 32, MAX_WIDTH) & ~31;
        risc_rect.h = clamp(h, 32, MAX_HEIGHT);
        size_option = true;
        break;
      }
      case 'c': {
        color_option = true;
        break;
      }
      case 'I': {
        serial_in = optarg;
        break;
      }
      case 'O': {
        serial_out = optarg;
        break;
      }
      case 'S': {
        boot_from_serial = true;
        risc_set_switches(risc, 1);
        break;
      }
      case 'H': {
        risc_set_host_fs(risc, host_fs_new(optarg));
        break;
      }
      case 'v': {
        use_VNC = true;
        break;
      }
      case 'h': {
        use_VNC = true;
        use_SDL = false;
        break;
      }
      default: {
        usage();
      }
    }
  }

  if (mem_option || size_option || rtc_option || color_option) {
    risc_configure_memory(risc, mem_option, rtc_option, risc_rect.w, risc_rect.h, color_option);
  }

  if (optind == argc - 1) {
    risc_set_spi(risc, 1, disk_new(argv[optind]));
  } else if (optind == argc && boot_from_serial) {
    /* Allow diskless boot */
    risc_set_spi(risc, 1, disk_new(NULL));
  } else {
    usage();
  }

  if (serial_in || serial_out) {
    if (!serial_in) {
      serial_in = "/dev/null";
    }
    if (!serial_out) {
      serial_out = "/dev/null";
    }
    risc_set_serial(risc, raw_serial_new(serial_in, serial_out));
  }

  /* Define and set these to NULL here even if not used */
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  SDL_Texture *texture = NULL;
  SDL_Rect display_rect;
  double display_scale;
  
  if (use_SDL) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
      fail(1, "Unable to initialize SDL: %s", SDL_GetError());
    }
    atexit(SDL_Quit);
    SDL_EnableScreenSaver();
    SDL_ShowCursor(false);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

    int window_flags = SDL_WINDOW_HIDDEN;
    int display = 0;
    if (fullscreen) {
      window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
      display = best_display(&risc_rect);
    }
    if (zoom == 0) {
      SDL_Rect bounds;
      if (SDL_GetDisplayBounds(display, &bounds) == 0 &&
        bounds.h >= risc_rect.h * 2 && bounds.w >= risc_rect.w * 2) {
        zoom = 2;
      } else {
        zoom = 1;
      }
    }
    window = SDL_CreateWindow("Project Oberon",
                              SDL_WINDOWPOS_UNDEFINED_DISPLAY(display),
                              SDL_WINDOWPOS_UNDEFINED_DISPLAY(display),
                              (int)(risc_rect.w * zoom),
                              (int)(risc_rect.h * zoom),
                              window_flags);
    if (window == NULL) {
      fail(1, "Could not create window: %s", SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == NULL) {
      fail(1, "Could not create renderer: %s", SDL_GetError());
    }

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                risc_rect.w,
                                risc_rect.h);
    if (texture == NULL) {
      fail(1, "Could not create texture: %s", SDL_GetError());
    }

    
    display_scale = scale_display(window, &risc_rect, &display_rect);
    update_texture(risc, texture, &risc_rect, color_option);
    SDL_ShowWindow(window);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, &risc_rect, &display_rect);
    SDL_RenderPresent(renderer);
  }
  
  if (use_VNC) {
   
    rfbScreen = rfbGetScreen(&argc,argv,risc_rect.w,risc_rect.h,8,3,4);
    
    if(!rfbScreen)
      fail(1, "Could not create VNC server");
    
    rfbScreen->desktopName = "Oberon RISC Emulator";
    rfbScreen->frameBuffer = (char*)malloc(risc_rect.w*risc_rect.h*4);
    rfbScreen->alwaysShared = TRUE;
    rfbScreen->ptrAddEvent = doptr;
    rfbScreen->kbdAddEvent = dokey;
    rfbScreen->handleEventsEagerly = TRUE; // need this for good mouse cursor performance 
    /* initialize the server */
    rfbInitServer(rfbScreen);
  }

  bool done = false;
  bool mouse_was_offscreen = false;
  while (!done) {
    uint32_t frame_start = SDL_GetTicks();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT: {
          done = true;
          break;
        }

        case SDL_WINDOWEVENT: {
          if (use_SDL && (event.window.event == SDL_WINDOWEVENT_RESIZED)) {
            display_scale = scale_display(window, &risc_rect, &display_rect);
          }
          break;
        }

        case SDL_DROPFILE: {
          char *dropped_file = event.drop.file;
          char *dropped_file_name = strrchr(dropped_file, '/');
          if (dropped_file_name == NULL)
            dropped_file_name = strrchr(dropped_file, '\\');
          if (dropped_file_name != NULL)
            dropped_file_name++;
          else
            dropped_file_name = dropped_file;
          printf("Dropped %s [%s]\n", dropped_file, dropped_file_name);
          FILE *f = fopen("PCLink.REC", "w");
          fputs(dropped_file_name, f);
          fputs(" ", f);
          fputs(dropped_file, f);
          fclose(f);
          SDL_free(dropped_file);
          break;
        }

        case SDL_MOUSEMOTION: {
          int scaled_x, scaled_y;
          if (use_SDL) {
            scaled_x = (int)round((event.motion.x - display_rect.x) / display_scale);
            scaled_y = (int)round((event.motion.y - display_rect.y) / display_scale);
          } else {
            scaled_x = event.motion.x;
            scaled_y = event.motion.y;
          }
          int x = clamp(scaled_x, 0, risc_rect.w - 1);
          int y = clamp(scaled_y, 0, risc_rect.h - 1);
          bool mouse_is_offscreen = x != scaled_x || y != scaled_y;
          if (mouse_is_offscreen != mouse_was_offscreen) {
            if (use_SDL)
              SDL_ShowCursor(mouse_is_offscreen);
            mouse_was_offscreen = mouse_is_offscreen;
          }
          risc_mouse_moved(risc, x, risc_rect.h - y - 1);
          break;
        }

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
          bool down = event.button.state == SDL_PRESSED;
          risc_mouse_button(risc, event.button.button, down);
          break;
        }

        case SDL_KEYDOWN:
        case SDL_KEYUP: {
          bool down = event.key.state == SDL_PRESSED;
          switch (map_keyboard_event(&event.key)) {
            case ACTION_RESET: {
              risc_reset(risc);
              break;
            }
            case ACTION_TOGGLE_FULLSCREEN: {
              if (!use_SDL) break;
              fullscreen ^= true;
              if (fullscreen) {
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
              } else {
                SDL_SetWindowFullscreen(window, 0);
              }
              break;
            }
            case ACTION_QUIT: {
              SDL_PushEvent(&(SDL_Event){ .type=SDL_QUIT });
              break;
            }
            case ACTION_FAKE_MOUSE1: {
              risc_mouse_button(risc, 1, down);
              break;
            }
            case ACTION_FAKE_MOUSE2: {
              risc_mouse_button(risc, 2, down);
              break;
            }
            case ACTION_FAKE_MOUSE3: {
              risc_mouse_button(risc, 3, down);
              break;
            }
            case ACTION_OBERON_INPUT: {
              uint8_t ps2_bytes[MAX_PS2_CODE_LEN];
              int len = ps2_encode(event.key.keysym.scancode, down, ps2_bytes);
              risc_keyboard_input(risc, ps2_bytes, len);
              break;
            }
          }
        }
      }
    }
    
    risc_set_time(risc, frame_start);
    risc_run(risc, CPU_HZ / FPS);
    
    if (use_SDL) {
      update_texture(risc, texture, &risc_rect, color_option);
      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, &risc_rect, &display_rect);
      SDL_RenderPresent(renderer);
    }

    if (use_VNC) {
      update_rfb(risc, rfbScreen, color_option);
      rfbProcessEvents(rfbScreen,-1);
      if (!rfbIsActive(rfbScreen))
        done = true;
    }    

    uint32_t frame_end = SDL_GetTicks();
    int delay = frame_start + 1000/FPS - frame_end;
    if (delay > 0) {
      SDL_Delay(delay);
    }
#if 0
    if (delay < 10)
       printf("%x - %x: Ticks spent: %d Delay: %d\n", frame_start, frame_end, frame_end-frame_start, delay);
#endif
  }
  return 0;
}


static int best_display(const SDL_Rect *rect) {
  int best = 0;
  int display_cnt = SDL_GetNumVideoDisplays();
  for (int i = 0; i < display_cnt; i++) {
    SDL_Rect bounds;
    if (SDL_GetDisplayBounds(i, &bounds) == 0 &&
        bounds.h == rect->h && bounds.w >= rect->w) {
      best = i;
      if (bounds.w == rect->w) {
        break;  // exact match
      }
    }
  }
  return best;
}

static int clamp(int x, int min, int max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

static enum Action map_keyboard_event(SDL_KeyboardEvent *event) {
  for (size_t i = 0; i < sizeof(key_map) / sizeof(key_map[0]); i++) {
    if ((event->state == key_map[i].state) &&
        (event->keysym.sym == key_map[i].sym) &&
        ((key_map[i].mod1 == 0) || (event->keysym.mod & key_map[i].mod1)) &&
        ((key_map[i].mod2 == 0) || (event->keysym.mod & key_map[i].mod2))) {
      return key_map[i].action;
    }
  }
  return ACTION_OBERON_INPUT;
}

static void show_leds(const struct RISC_LED *leds, uint32_t value) {
  printf("LEDs: ");
  for (int i = 7; i >= 0; i--) {
    if (value & (1 << i)) {
      printf("%d", i);
    } else {
      printf("-");
    }
  }
  printf("\n");
}

static double scale_display(SDL_Window *window, const SDL_Rect *risc_rect, SDL_Rect *display_rect) {
  int win_w, win_h;
  SDL_GetWindowSize(window, &win_w, &win_h);
  double oberon_aspect = (double)risc_rect->w / risc_rect->h;
  double window_aspect = (double)win_w / win_h;

  double scale;
  if (oberon_aspect > window_aspect) {
    scale = (double)win_w / risc_rect->w;
  } else {
    scale = (double)win_h / risc_rect->h;
  }

  int w = (int)ceil(risc_rect->w * scale);
  int h = (int)ceil(risc_rect->h * scale);
  *display_rect = (SDL_Rect){
    .w = w, .h = h,
    .x = (win_w - w) / 2,
    .y = (win_h - h) / 2
  };
  return scale;
}

// Only used in update_texture(), but some systems complain if you
// allocate three megabyte on the stack.
static uint32_t pixel_buf[MAX_WIDTH * MAX_HEIGHT];

static void update_texture(struct RISC *risc, SDL_Texture *texture, const SDL_Rect *risc_rect, bool color) {
  struct Damage damage = risc_get_framebuffer_damage(risc);
  if (damage.y1 <= damage.y2) {
    uint32_t *in = risc_get_framebuffer_ptr(risc);
    uint32_t *pal = color ? risc_get_palette_ptr(risc) : NULL;
    uint32_t out_idx = 0;

    for (int line = damage.y2; line >= damage.y1; line--) {
      int line_start = line * (risc_rect->w / (color ? 8 : 32));
      for (int col = damage.x1; col <= damage.x2; col++) {
        uint32_t pixels = in[line_start + col];
        if (color) {
          for (int b = 0; b < 8; b++) {
            pixel_buf[out_idx] = pal[pixels & 0xF];
            pixels >>= 4;
            out_idx++;
          }
        } else {
          for (int b = 0; b < 32; b++) {
            pixel_buf[out_idx] = (pixels & 1) ? WHITE : BLACK;
            pixels >>= 1;
            out_idx++;
          }
        }
      }
    }

    SDL_Rect rect = {
      .x = damage.x1 * (color ? 8 : 32),
      .y = risc_rect->h - damage.y2 - 1,
      .w = (damage.x2 - damage.x1 + 1) * (color ? 8 : 32),
      .h = (damage.y2 - damage.y1 + 1)
    };
    SDL_UpdateTexture(texture, &rect, pixel_buf, rect.w * 4);
  }
}

static void update_rfb(struct RISC *risc, rfbScreenInfoPtr screen, bool color) {
  struct Damage damage = risc_get_framebuffer_damage(risc);
  
  if (damage.y1 <= damage.y2) {
    uint32_t *in = risc_get_framebuffer_ptr(risc);
    uint32_t *pal = color ? risc_get_palette_ptr(risc) : NULL;

   int rowstride = screen->paddedWidthInBytes;
   int bpp = screen->serverFormat.bitsPerPixel/8;

   int fbx1 = damage.x1 * (color ? 8 : 32);
   int fbx2 = damage.x2 * (color ? 8 : 32) + (color ? 8 : 32);
   int fby1 = risc_rect.h - damage.y1;
   int fby2 = risc_rect.h - damage.y2 - 1;
   
   for (int line = damage.y2; line >= damage.y1; line--) {
     int line_start = line * (risc_rect.w / (color ? 8 : 32));
     for (int col = damage.x1; col <= damage.x2; col++) {
       uint32_t pixels = in[line_start + col];
       if (color) {
         for (int b = 0; b < 8; b++) {
           rfbDrawPixel(screen, col*8+b, line , pal[pixels & 0xF]);
           pixels >>= 4;
         }
       } else {
         for (int b = 0; b < 32; b++) {
           uint32_t colorbuf = (pixels & 0x1) ? Swap24(WHITE) : Swap24(BLACK);
           memcpy(screen->frameBuffer+(risc_rect.h - line - 1)*rowstride+(col*32+b)*bpp,&colorbuf,bpp);
           pixels >>= 1;
         }
       }
     }
   }
   rfbMarkRectAsModified(screen, fbx1, fby1, fbx2, fby2);

  }  
}


static void doptr(int buttonMask,int x,int y,rfbClientPtr cl)
{
   if(x>=0 && y>=0 && x<risc_rect.w && y<risc_rect.h) {
      risc_mouse_moved(risc, x, risc_rect.h - y - 1);
   }
}

static void dokey(rfbBool down,rfbKeySym key,rfbClientPtr cl)
{
  static bool Control_down = false;
 

  if ((key==XK_Shift_L) || (key==XK_Shift_R))
    return;

  if ((key==XK_Control_L) || (key==XK_Control_R)) {
    Control_down = down;
    return;
  }
  
//  if(down) {
//    if(key==XK_Escape)
//      rfbCloseClient(cl);
//    else if(key==XK_x)
//      /* close down server, disconnecting clients */
//      rfbShutdownServer(cl->screen,TRUE);
//    else
//     if(key==XK_Control_L)
//       Control_down = true;
//  } else {
//     if(key==XK_Control_L)
//       Control_down = false;
//  }  
  

  printf("char: %c key: 0x%x  down: 0x%x\n", key, key, down);
  /* Fake Mouse buttons & other controls */
  if(Control_down && (key==XK_semicolon))
    risc_mouse_button(risc, 1, down);
  else if(Control_down && (key==XK_q))
    risc_mouse_button(risc, 2, down);
  else if(Control_down && (key==XK_j))
    risc_mouse_button(risc, 3, down);
  else if(Control_down && (key==XK_x))
    rfbShutdownServer(cl->screen,TRUE);
  else {
    uint8_t ps2_bytes[MAX_PS2_CODE_LEN];
    int len = rfb_ps2_encode(key, down, ps2_bytes);
    //for (int i = 0; i < len; i++) 
    //  printf("--> 0x%x, ", ps2_bytes[i]);
    //printf("<--\n");
    risc_keyboard_input(risc, ps2_bytes, len);
  }
}
