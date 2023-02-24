/*
 * Copyright (C) 2001 Dirk Jagdmann <doj@cubic.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Modified by Simon Newton (nomis52<AT>gmail.com) to use ola
 *
 * The (void) before attrset is due to a bug in curses. See
 * http://www.mail-archive.com/debian-bugs-dist@lists.debian.org/msg682294.html
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#ifdef HAVE_CURSES_H
#include <curses.h>
#elif defined(HAVE_NCURSES_CURSES_H)
#include <ncurses/curses.h>
#endif  // HAVE_CURSES_H

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#ifdef HAVE_FTIME
#include <sys/timeb.h>
#endif  // HAVE_FTIME
#include <termios.h>
#include <time.h>
#include <math.h>

#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/DmxBuffer.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/client/ClientWrapper.h>
#include <ola/client/OlaClient.h>
#include <ola/io/SelectServer.h>

#include <string>

using ola::client::OlaClient;
using ola::client::OlaClientWrapper;
using ola::io::SelectServer;
using std::string;

static const unsigned int DEFAULT_UNIVERSE = 0;
static const unsigned char CHANNEL_NUDGE_VALUE = 0x10;
static const unsigned char CHANNEL_DISPLAY_WIDTH = 4;
static const unsigned char ROWS_PER_CHANNEL_ROW = 2;

/* color names used */
enum {
  CHANNEL = 1,
  ZERO,
  NORM,
  FULL,
  HEADLINE,
  HEADEMPH,
  HEADERROR,
  MAXCOLOR
};

/* display modes */
enum {
  DISP_MODE_DMX = 0,
  DISP_MODE_HEX,
  DISP_MODE_DEC,
  DISP_MODE_MAX,
};

typedef struct {
  unsigned int universe;
  bool help;        // help
} options;

unsigned int MAXFKEY = 12;

unsigned int universe = 0;

typedef unsigned char dmx_t;

static dmx_t *dmx;
static dmx_t *dmxsave;
static dmx_t *dmxundo;

static int display_mode = DISP_MODE_DMX;
static int current_channel = 0;  // channel cursor is positioned on
static int first_channel = 0;  // channel in upper left corner
static int channels_per_line = 80 / CHANNEL_DISPLAY_WIDTH;
// Default chans/screen is 80x24, less a row for the header,
// and one at the bottom to get an even number of rows
static int channels_per_screen =
    (80 / CHANNEL_DISPLAY_WIDTH) * ((24 - 2) / ROWS_PER_CHANNEL_ROW);
static int undo_possible = 0;
static int current_cue = 0;  // select with F keys
static float fadetime = 1.0f;
static int fading = 0;  // percentage counter of fade process
static int palette_number = 0;
static int palette[MAXCOLOR];
static bool screen_to_small = false;
static int channels_offset = 1;

OlaClient *client;
SelectServer *ss;


void DMXsleep(int usec) {
  struct timeval tv;
  tv.tv_sec = usec / 1000000;
  tv.tv_usec = usec % 1000000;
  if (select(1, NULL, NULL, NULL, &tv) < 0)
    perror("could not select");
}

// returns the time in milliseconds
uint64_t timeGetTime() {
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (static_cast<uint64_t>(tv.tv_sec) * 1000UL +
          static_cast<uint64_t>(tv.tv_usec / 1000));

#else
# ifdef HAVE_FTIME
  struct timeb t;
  ftime(&t);
  return (static_cast<uint64_t>(t.time) * 1000UL +
          static_cast<uint64_t>(t.millitm);
# else

# endif  // HAVE_FTIME
#endif  // HAVE_GETTIMEOFDAY
}



/* set all DMX channels */
void setall() {
  ola::DmxBuffer buffer(dmx, ola::DMX_UNIVERSE_SIZE);
  client->SendDMX(universe, buffer, ola::client::SendDMXArgs());
}

/* set current DMX channel */
void set() {
  setall();
}


/* display the channels numbers */
void mask() {
  int i = 0;
  int x, y;
  int z = first_channel;

  erase();

  /* clear headline */
  (void) attrset(palette[HEADLINE]);
  move(0, 0);  // NOLINT(build/include_what_you_use) This is ncurses.h's move
  for (x = 0; x < COLS; x++)
    addch(' ');

  /* write channel numbers */
  (void) attrset(palette[CHANNEL]);
  for (y = 1;
       y < LINES && z < ola::DMX_UNIVERSE_SIZE && i < channels_per_screen;
       y += ROWS_PER_CHANNEL_ROW) {
    move(y, 0);  // NOLINT(build/include_what_you_use) This is ncurses.h's move
    for (x = 0;
         x < channels_per_line &&
         z < ola::DMX_UNIVERSE_SIZE &&
         i < channels_per_screen;
         x++, i++, z++) {
      switch (display_mode) {
        case DISP_MODE_DMX:
        case DISP_MODE_DEC:
        default:
          printw("%03d ", z + channels_offset);
          break;
        case DISP_MODE_HEX:
          printw("%03X ", z + channels_offset);
          break;
        }
    }
  }
}

/* update the screen */
void values() {
  int i = 0;
  int x, y;
  int z = first_channel;
  int universe_length = 0;
  int width_total = 0;

  if (universe > 0) {
    universe_length = floor(log10(universe)) + 1;
  } else {
    universe_length = 1;
  }

  /* headline */
  width_total += 25;
  if (COLS >= width_total) {
    time_t t = time(NULL);
    struct tm tt;
    localtime_r(&t, &tt);
    char s[32];
    asctime_r(&tt, s);
    s[strlen(s) - 1] = 0; /* strip newline at end of string */

    attrset(palette[HEADLINE]);
    mvprintw(0, 1, "%s", s);
  }
  width_total += (5 + universe_length);
  if (COLS >= width_total) {
    /* Max universe 4294967295 - see MAX_UNIVERSE in include/ola/Constants.h */
    attrset(palette[HEADLINE]);
    printw(" uni:");
    attrset(palette[HEADEMPH]);
    printw("%u", universe);
  }
  width_total += (5 + 2);
  if (COLS >= width_total) {
    attrset(palette[HEADLINE]);
    printw(" cue:");
    attrset(palette[HEADEMPH]);
    printw("%02i", current_cue + 1);
  }
  width_total += (10 + 3);
  if (COLS >= width_total) {
    attrset(palette[HEADLINE]);
    printw(" fadetime:");
    attrset(palette[HEADEMPH]);
    printw("%1.1f", fadetime);
  }
  width_total += (8 + 3);
  if (COLS >= width_total) {
    if (fading) {
      attrset(palette[HEADLINE]);
      printw(" fading:");
      attrset(palette[HEADEMPH]);
      printw("%02i%%", (fading < 100) ? fading: 99);
    } else {
      attrset(palette[HEADLINE]);
      printw("           ");
    }
  }
  /* Use 10 as error string length, rather than error_str.length(),
     as a safety feature to ensure it's shown */
  width_total += (6 + 10);
  if (COLS >= width_total && screen_to_small) {
    attrset(palette[HEADERROR]);
    printw("ERROR: screen too small, we need at least 3 lines");
  }

  /* values */
  for (y = ROWS_PER_CHANNEL_ROW;
       y < LINES && z < ola::DMX_UNIVERSE_SIZE && i < channels_per_screen;
       y += ROWS_PER_CHANNEL_ROW) {
    move(y, 0);  // NOLINT(build/include_what_you_use) This is ncurses.h's move
    for (x = 0;
         x < channels_per_line &&
         z < ola::DMX_UNIVERSE_SIZE &&
         i < channels_per_screen;
         x++, z++, i++) {
      const int d = dmx[z];
      switch (d) {
        case ola::DMX_MIN_SLOT_VALUE:
          attrset(palette[ZERO]);
          break;
        case ola::DMX_MAX_SLOT_VALUE:
          attrset(palette[FULL]);
          break;
        default:
          attrset(palette[NORM]);
      }
      if (z == current_channel)
        attron(A_REVERSE);
      switch (display_mode) {
        case DISP_MODE_HEX:
          if (d == 0)
            addstr("    ");
          else
            printw(" %02x ", d);
          break;
        case DISP_MODE_DEC:
          if (d == 0)
            addstr("    ");
          else if (d < 100)
            printw(" %02d ", d);
          else
            printw("%03d ", d);
          break;
        case DISP_MODE_DMX:
        default:
          switch (d) {
            case ola::DMX_MIN_SLOT_VALUE:
              addstr("    ");
              break;
            case ola::DMX_MAX_SLOT_VALUE:
              addstr(" FL ");
              break;
            default:
              printw(" %02d ", (d * 100) / ola::DMX_MAX_SLOT_VALUE);
          }
      }
    }
  }
}

/* save current cue into cuebuffer */
void savecue() {
  memcpy(&dmxsave[current_cue * ola::DMX_UNIVERSE_SIZE],
         dmx,
         ola::DMX_UNIVERSE_SIZE);
}

/* get new cue from cuebuffer */
void loadcue() {
  memcpy(dmx,
         &dmxsave[current_cue * ola::DMX_UNIVERSE_SIZE],
         ola::DMX_UNIVERSE_SIZE);
}

/* fade cue "new_cue" into current cue */
void crossfade(unsigned int new_cue) {
  dmx_t *dmxold;
  dmx_t *dmxnew;
  int i;
  int max = ola::DMX_UNIVERSE_SIZE;

  /* check parameter */
  if (new_cue > MAXFKEY)
    return;

  undo_possible = 0;

  /* don't crossfade for small fadetimes */
  if (fadetime < 0.1f) {
    savecue();
    current_cue = new_cue;
    loadcue();
    setall();
    return;
  }

  savecue();
  dmxold = &dmxsave[current_cue * ola::DMX_UNIVERSE_SIZE];
  dmxnew = &dmxsave[new_cue * ola::DMX_UNIVERSE_SIZE];

  /* try to find the last channel value > 0, so we don't have to
     crossfade large blocks of 0s */
  for (i = ola::DMX_UNIVERSE_SIZE - 1; i >= 0; max = i, i--)
    if (dmxold[i] || dmxnew[i])
      break;

  {
    const uint64_t tstart = timeGetTime();
    const uint64_t tend = tstart + static_cast<int>(fadetime * 1000.0);
    uint64_t t = tstart;
    while (t <= tend) {
    /* calculate new cue */
    t = timeGetTime();
    {
      const float p = static_cast<float>(t - tstart) / 1000.0f / fadetime;
      const float q = 1.0f - p;
      for (i = 0; i < max; i++)
        if (dmxold[i] || dmxnew[i]) /* avoid calculating with only 0 */
          dmx[i] = static_cast<int>(static_cast<float>(dmxold[i]) * q +
                                    static_cast<float>(dmxnew[i]) * p);
      setall();

      /* update screen */
      fading = static_cast<int>(p * 100.0f);
      values();
      refresh();
      DMXsleep(100000);

      // get current time, because the last time is too old (due to the sleep)
      t = timeGetTime();
    }
      }
    fading = 0;

    /* set the new cue */
    current_cue = new_cue;
    loadcue();
    setall();
  }
}


void undo() {
  if (undo_possible) {
    memcpy(dmx, dmxundo, ola::DMX_UNIVERSE_SIZE);
    undo_possible = 0;
  }
}

void undoprep() {
  memcpy(dmxundo, dmx, ola::DMX_UNIVERSE_SIZE);
  undo_possible = 1;
}

/* change palette to "p". If p is invalid new palette is number "0". */
void changepalette(int p) {
  /* COLOR_BLACK
     COLOR_RED
     COLOR_GREEN
     COLOR_YELLOW
     COLOR_BLUE
     COLOR_MAGENTA
     COLOR_CYAN
     COLOR_WHITE

     A_NORMAL
     A_ATTRIBUTES
     A_CHARTEXT
     A_COLOR
     A_STANDOUT
     A_UNDERLINE
     A_REVERSE
     A_BLINK
     A_DIM
     A_BOLD
     A_ALTCHARSET
     A_INVIS
  */
  switch (p) {
    default:
      palette_number = 0;
      // fall through, use 0 as default palette
      OLA_FALLTHROUGH
    case 0:
      init_pair(CHANNEL, COLOR_BLACK, COLOR_CYAN);
      init_pair(ZERO, COLOR_BLACK, COLOR_WHITE);
      init_pair(NORM, COLOR_BLUE, COLOR_WHITE);
      init_pair(FULL, COLOR_RED, COLOR_WHITE);
      init_pair(HEADLINE, COLOR_WHITE, COLOR_BLUE);
      init_pair(HEADEMPH, COLOR_YELLOW, COLOR_BLUE);
      init_pair(HEADERROR, COLOR_RED, COLOR_BLUE);
      break;
    case 2:
      init_pair(CHANNEL, COLOR_BLACK, COLOR_WHITE);
      init_pair(ZERO, COLOR_BLUE, COLOR_BLACK);
      init_pair(NORM, COLOR_GREEN, COLOR_BLACK);
      init_pair(FULL, COLOR_RED, COLOR_BLACK);
      init_pair(HEADLINE, COLOR_WHITE, COLOR_BLACK);
      init_pair(HEADEMPH, COLOR_CYAN, COLOR_BLACK);
      init_pair(HEADERROR, COLOR_RED, COLOR_BLACK);
      break;
    case 1:
      palette[CHANNEL] = A_REVERSE;
      palette[ZERO] = A_NORMAL;
      palette[NORM] = A_NORMAL;
      palette[FULL] = A_BOLD;
      palette[HEADLINE] = A_NORMAL;
      palette[HEADEMPH] = A_NORMAL;
      palette[HEADERROR] = A_BOLD;
      break;
  }

  if (p == 0 || p == 2) {
    palette[CHANNEL] = COLOR_PAIR(CHANNEL);
    palette[ZERO] = COLOR_PAIR(ZERO);
    palette[NORM] = COLOR_PAIR(NORM);
    palette[FULL] = COLOR_PAIR(FULL);
    palette[HEADLINE] = COLOR_PAIR(HEADLINE);
    palette[HEADEMPH] = COLOR_PAIR(HEADEMPH);
    palette[HEADERROR] = COLOR_PAIR(HEADERROR);
  }

  mask();
}

void CHECK(void *p) {
  if (p == NULL) {
    fprintf(stderr, "could not alloc\n");
    exit(1);
  }
}


/* calculate channels_per_line and channels_per_screen from LINES and COLS */
void calcscreengeometry() {
  int c = LINES;
  if (c < 3) {
    screen_to_small = true;
    exit(1);
  }
  c--;  // One line for headline
  if (c % ROWS_PER_CHANNEL_ROW == 1)
    c--;  // Need an even number of lines for data
  channels_per_line = COLS / CHANNEL_DISPLAY_WIDTH;
  channels_per_screen = channels_per_line * (c / ROWS_PER_CHANNEL_ROW);
}

/* signal handler for SIGWINCH */
void terminalresize(int sig) {
  struct winsize size;
  if (ioctl(0, TIOCGWINSZ, &size) < 0)
    return;

  resizeterm(size.ws_row, size.ws_col);
  calcscreengeometry();
  mask();
  (void) sig;
}

WINDOW *w = NULL;

/* cleanup handler for program exit. */
void cleanup() {
  if (w) {
    resetty();
    endwin();
  }

  if (screen_to_small)
    puts("screen too small, we need at least 3 lines");
}

void stdin_ready() {
  int n;
  int c = wgetch(w);
  switch (c) {
    case KEY_PPAGE:
      undoprep();
      if (dmx[current_channel] < ola::DMX_MAX_SLOT_VALUE - CHANNEL_NUDGE_VALUE)
        dmx[current_channel] += CHANNEL_NUDGE_VALUE;
      else
        dmx[current_channel] = ola::DMX_MAX_SLOT_VALUE;
      set();
      break;

    case '+':
      if (dmx[current_channel] < ola::DMX_MAX_SLOT_VALUE) {
        undoprep();
        dmx[current_channel]++;
      }
      set();
      break;

    case KEY_NPAGE:
      undoprep();
      if (dmx[current_channel] == ola::DMX_MAX_SLOT_VALUE) {
        // Smooth out the fade down
        dmx[current_channel] = (ola::DMX_MAX_SLOT_VALUE + 1) -
            CHANNEL_NUDGE_VALUE;
      } else if (dmx[current_channel] > CHANNEL_NUDGE_VALUE) {
        dmx[current_channel] -= CHANNEL_NUDGE_VALUE;
      } else {
        dmx[current_channel] = ola::DMX_MIN_SLOT_VALUE;
      }
      set();
      break;

    case '-':
      if (dmx[current_channel] > ola::DMX_MIN_SLOT_VALUE) {
        undoprep();
        dmx[current_channel]--;
      }
      set();
      break;

    case ' ':
      undoprep();
      if (dmx[current_channel] < ((ola::DMX_MAX_SLOT_VALUE + 1) / 2))
        dmx[current_channel] = ola::DMX_MAX_SLOT_VALUE;
      else
        dmx[current_channel] = ola::DMX_MIN_SLOT_VALUE;
      set();
      break;

    case '0' ... '9':
      fadetime = c -'0';
      break;

    case KEY_HOME:
      current_channel = 0;
      first_channel = 0;
      mask();
      break;

    case KEY_END:
      current_channel = ola::DMX_UNIVERSE_SIZE - 1;
      if (channels_per_screen >= ola::DMX_UNIVERSE_SIZE) {
        first_channel = 0;
      } else {
        first_channel = current_channel - (channels_per_screen - 1);
      }
      mask();
      break;

    case KEY_RIGHT:
      if (current_channel < ola::DMX_UNIVERSE_SIZE - 1) {
        current_channel++;
        if (current_channel >= first_channel + channels_per_screen) {
          first_channel += channels_per_line;
          mask();
        }
      }
      break;

    case KEY_LEFT:
      if (current_channel > 0) {
        current_channel--;
        if (current_channel < first_channel) {
          first_channel -= channels_per_line;
          if (first_channel < 0)
            first_channel = 0;
          mask();
        }
      }
      break;

    case KEY_DOWN:
      current_channel += channels_per_line;
      if (current_channel >= ola::DMX_UNIVERSE_SIZE)
        current_channel = ola::DMX_UNIVERSE_SIZE - 1;
      if (current_channel >= first_channel + channels_per_screen) {
        first_channel += channels_per_line;
        mask();
      }
      break;

    case KEY_UP:
      current_channel -= channels_per_line;
      if (current_channel < 0)
        current_channel = 0;
      if (current_channel < first_channel) {
        first_channel -= channels_per_line;
        if (first_channel < 0)
          first_channel = 0;
        mask();
      }
      break;

    case KEY_IC:
      undoprep();
      for (n = ola::DMX_UNIVERSE_SIZE - 1; n > current_channel && n > 0; n--)
        dmx[n] = dmx[n - 1];
      setall();
      break;

    case KEY_DC:
      undoprep();
      for (n = current_channel; n < ola::DMX_UNIVERSE_SIZE - 1; n++)
        dmx[n] = dmx[n + 1];
      setall();
      break;

    case 'B':
    case 'b':
      undoprep();
      memset(dmx, ola::DMX_MIN_SLOT_VALUE, ola::DMX_UNIVERSE_SIZE);
      setall();
      break;

    case 'F':
    case 'f':
      undoprep();
      memset(dmx, ola::DMX_MAX_SLOT_VALUE, ola::DMX_UNIVERSE_SIZE);
      setall();
      break;

    case 'M':
    case 'm':
      if (++display_mode >= DISP_MODE_MAX)
        display_mode = 0;
      mask();
      break;

    case 'N':
    case 'n':
      if (++channels_offset > 1)
        channels_offset = 0;
      mask();
      break;

    case 'P':
    case 'p':
      changepalette(++palette_number);
      break;

    case 'U':
    case 'u':
      undo();
      break;

    case 'Q':
    case 'q':
      ss->Terminate();
      break;

    default:
      if (c >= static_cast<int>(KEY_F(1)) &&
          c <= static_cast<int>(KEY_F(MAXFKEY)))
        crossfade(c - KEY_F(1));
      break;
  }
  values();
  refresh();
}

/*
 * parse our cmd line options
 */
void ParseOptions(int argc, char *argv[], options *opts) {
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"universe", required_argument, 0, 'u'},
      {0, 0, 0, 0}
    };

  opts->universe = DEFAULT_UNIVERSE;
  opts->help = false;

  int c;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "hu:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'h':
        opts->help = true;
        break;
      case 'u':
        opts->universe = strtoul(optarg, NULL, 0);
        break;
      case '?':
        break;
      default:
        break;
    }
  }
}


/*
 * Display the help message
 */
void DisplayHelpAndExit(char arg[]) {
  std::cout << "Usage: " << arg << " [--universe <universe_id>]\n"
  "\n"
  "Send data to a DMX512 universe.\n"
  "\n"
  "  -h, --help                   Display this help message and exit.\n"
  "  -u, --universe <universe_id> Id of universe to control (defaults to "
  << DEFAULT_UNIVERSE << ").\n"
  << std::endl;
  exit(ola::EXIT_OK);
}

int main(int argc, char *argv[]) {
  signal(SIGWINCH, terminalresize);
  atexit(cleanup);

  if (!ola::NetworkInit()) {
    std::cerr << "Network initialization failed." << std::endl;
    exit(ola::EXIT_UNAVAILABLE);
  }

  // 10 bytes security, for file IO routines, will be optimized and checked
  // later
  dmx = reinterpret_cast<dmx_t*>(calloc(ola::DMX_UNIVERSE_SIZE + 10,
                                 sizeof(dmx_t)));
  CHECK(dmx);

  dmxsave = reinterpret_cast<dmx_t*>(
      calloc(ola::DMX_UNIVERSE_SIZE * MAXFKEY, sizeof(dmx_t)));
  CHECK(dmxsave);

  dmxundo = reinterpret_cast<dmx_t*>(
      calloc(ola::DMX_UNIVERSE_SIZE, sizeof(dmx_t)));
  CHECK(dmxundo);

  options opts;

  ParseOptions(argc, argv, &opts);

  if (opts.help) {
    DisplayHelpAndExit(argv[0]);
  }

  universe = opts.universe;

  /* set up ola connection */
  OlaClientWrapper ola_client;
  ola::io::UnmanagedFileDescriptor stdin_descriptor(0);
  stdin_descriptor.SetOnData(ola::NewCallback(&stdin_ready));

  if (!ola_client.Setup()) {
    printf("error: %s", strerror(errno));
    exit(1);
  }

  client = ola_client.GetClient();
  ss = ola_client.GetSelectServer();
  ss->AddReadDescriptor(&stdin_descriptor);

  /* init curses */
  w = initscr();
  if (!w) {
    printf("unable to open main-screen\n");
    return 1;
  }

  savetty();
  start_color();
  noecho();
  raw();
  keypad(w, TRUE);

  calcscreengeometry();
  changepalette(palette_number);

  values();
  refresh();
  ss->Run();
  return 0;
}
