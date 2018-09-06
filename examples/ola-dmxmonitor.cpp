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
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#ifdef HAVE_FTIME
#include <sys/timeb.h>
#endif  // HAVE_FTIME
#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/Constants.h>
#include <ola/DmxBuffer.h>
#include <ola/base/Init.h>
#include <ola/base/Macro.h>
#include <ola/base/SysExits.h>
#include <ola/client/ClientWrapper.h>
#include <ola/client/OlaClient.h>
#include <ola/io/SelectServer.h>

#include <string>
#include <iostream>

using ola::Clock;
using ola::DmxBuffer;
using ola::client::OlaClient;
using ola::client::OlaClientWrapper;
using ola::client::Result;
using ola::TimeInterval;
using ola::TimeStamp;
using ola::io::SelectServer;
using std::string;

static const unsigned int DEFAULT_UNIVERSE = 0;
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

class DmxMonitor *dmx_monitor;

static unsigned int display_mode = DISP_MODE_DMX;
static int current_channel = 0;    /* channel cursor is positioned on */
static int first_channel = 0;    /* channel in upper left corner */
static unsigned int channels_per_line = 80 / CHANNEL_DISPLAY_WIDTH;
// Default chans/screen is 80x24, less a row for the header,
// and one at the bottom to get an even number of rows
static unsigned int channels_per_screen =
    (80 / CHANNEL_DISPLAY_WIDTH) * ((24 - 2) / ROWS_PER_CHANNEL_ROW);
static int palette[MAXCOLOR];

/*
 * The observer class which responds to events
 */
class DmxMonitor {
 public:
    explicit DmxMonitor(unsigned int universe)
        : m_universe(universe),
          m_counter(0),
          m_palette_number(0),
          m_stdin_descriptor(STDIN_FILENO),
          m_window(NULL),
          m_data_loss_window(NULL),
          m_channels_offset(true) {
    }

    ~DmxMonitor() {
      if (m_window) {
        resetty();
        endwin();
      }
    }

    bool Init();
    void Run() { m_client.GetSelectServer()->Run(); }
    void NewDmx(const ola::client::DMXMetadata &meta,
                const DmxBuffer &buffer);
    void RegisterComplete(const Result &result);
    void StdinReady();
    bool CheckDataLoss();
    void DrawDataLossWindow();
    void TerminalResized();

 private:
    unsigned int m_universe;
    unsigned int m_counter;
    int m_palette_number;
    ola::io::UnmanagedFileDescriptor m_stdin_descriptor;
    TimeStamp m_last_data;
    WINDOW *m_window;
    WINDOW *m_data_loss_window;
    bool m_channels_offset;  // start from channel 1 rather than 0;
    OlaClientWrapper m_client;
    DmxBuffer m_buffer;

    void DrawScreen(bool include_values = true);
    void Mask();
    void Values();
    void ChangePalette(int p);
    void CalcScreenGeometry();
};


/*
 * Setup the monitoring console
 */
bool DmxMonitor::Init() {
  /* set up ola connection */
  if (!m_client.Setup()) {
    printf("error: %s", strerror(errno));
    return false;
  }

  OlaClient *client = m_client.GetClient();
  client->SetDMXCallback(ola::NewCallback(this, &DmxMonitor::NewDmx));
  client->RegisterUniverse(
      m_universe,
      ola::client::REGISTER,
      ola::NewSingleCallback(this, &DmxMonitor::RegisterComplete));

  /* init curses */
  m_window = initscr();
  if (!m_window) {
    printf("unable to open main-screen\n");
    return false;
  }

  savetty();
  start_color();
  noecho();
  raw();
  keypad(m_window, TRUE);

  m_client.GetSelectServer()->AddReadDescriptor(&m_stdin_descriptor);
  m_stdin_descriptor.SetOnData(
      ola::NewCallback(this, &DmxMonitor::StdinReady));
  m_client.GetSelectServer()->RegisterRepeatingTimeout(
      500,
      ola::NewCallback(this, &DmxMonitor::CheckDataLoss));
  CalcScreenGeometry();
  ChangePalette(m_palette_number);

  m_buffer.Blackout();
  DrawScreen();
  return true;
}


/*
 * Called when there is new DMX data
 */
void DmxMonitor::NewDmx(OLA_UNUSED const ola::client::DMXMetadata &meta,
                        const DmxBuffer &buffer) {
  m_buffer.Set(buffer);

  if (m_data_loss_window) {
    // delete the window
    wborder(m_data_loss_window, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(m_data_loss_window);
    delwin(m_data_loss_window);
    m_data_loss_window = NULL;
    Mask();
  }
  move(0, COLS - 1);  // NOLINT(build/include_what_you_use) This is ncurses.h's
  switch (m_counter % 4) {
    case 0:
      printw("/");
      break;
    case 1:
      printw("-");
      break;
    case 2:
      printw("\\");
      break;
    default:
      printw("|");
      break;
  }
  m_counter++;

  Clock clock;
  clock.CurrentTime(&m_last_data);
  Values();
  refresh();
}


void DmxMonitor::RegisterComplete(const Result &result) {
  if (!result.Success()) {
    std::cerr << "Register command failed with " << result.Error() <<
      std::endl;
    m_client.GetSelectServer()->Terminate();
  }
}

/*
 * Called when there is input from the keyboard
 */
void DmxMonitor::StdinReady() {
  int c = wgetch(m_window);

  switch (c) {
    case KEY_HOME:
      current_channel = 0;
      first_channel = 0;
      DrawScreen();
      break;

    case KEY_END:
      current_channel = ola::DMX_UNIVERSE_SIZE - 1;
      if (channels_per_screen >= ola::DMX_UNIVERSE_SIZE) {
        first_channel = 0;
      } else {
        first_channel = current_channel - (channels_per_screen - 1);
      }
      DrawScreen();
      break;

    case 'l':
    case 'L':
    case KEY_RIGHT:
      if (current_channel < ola::DMX_UNIVERSE_SIZE - 1) {
        current_channel++;
        if (current_channel >=
            static_cast<int>(first_channel + channels_per_screen)) {
          first_channel += channels_per_line;
        }
        DrawScreen();
      }
      break;

    case 'h':
    case 'H':
    case KEY_LEFT:
      if (current_channel > 0) {
        current_channel--;
        if (current_channel < first_channel) {
          first_channel -= channels_per_line;
          if (first_channel < 0)
            first_channel = 0;
        }
        DrawScreen();
      }
      break;

    case 'j':
    case 'J':
    case KEY_DOWN:
      current_channel += channels_per_line;
      if (current_channel >= ola::DMX_UNIVERSE_SIZE)
        current_channel = ola::DMX_UNIVERSE_SIZE - 1;
      if (current_channel >=
          static_cast<int>(first_channel + channels_per_screen)) {
        first_channel += channels_per_line;
      }
      DrawScreen();
      break;

    case 'k':
    case 'K':
    case KEY_UP:
      current_channel -= channels_per_line;
      if (current_channel < 0)
        current_channel = 0;
      if (current_channel < first_channel) {
        first_channel -= channels_per_line;
        if (first_channel < 0)
          first_channel = 0;
      }
      DrawScreen();
      break;

    case 'M':
    case 'm':
      if (++display_mode >= DISP_MODE_MAX)
        display_mode = 0;
      DrawScreen();
      break;

    case 'N':
    case 'n':
      m_channels_offset = !m_channels_offset;
      DrawScreen(false);
      break;

    case 'P':
    case 'p':
      ChangePalette(++m_palette_number);
      DrawScreen();
      break;

    case 'Q':
    case 'q':
      m_client.GetSelectServer()->Terminate();
      break;
    default:
        break;
  }
}


/*
 * Check for data loss.
 * TODO(simon): move to the ola server
 */
bool DmxMonitor::CheckDataLoss() {
  if (m_last_data.IsSet()) {
    TimeStamp now;
    Clock clock;
    clock.CurrentTime(&now);
    TimeInterval diff = now - m_last_data;
    if (diff > TimeInterval(2, 5000000)) {
      // loss of data
      DrawDataLossWindow();
    }
  }
  return true;
}


void DmxMonitor::DrawDataLossWindow() {
  if (!m_data_loss_window) {
    m_data_loss_window = newwin(3, 14, (LINES - 3) / 2, (COLS - 14) / 2);
  }
  mvwprintw(m_data_loss_window, 1, 2, "Data Loss!");
  wborder(m_data_loss_window, '|', '|', '-', '-', '+', '+', '+', '+');
  wrefresh(m_data_loss_window);
}


/*
 * Called when the terminal is resized
 */
void DmxMonitor::TerminalResized() {
  struct winsize size;
  if (ioctl(0, TIOCGWINSZ, &size) < 0)
      return;

  resizeterm(size.ws_row, size.ws_col);
  CalcScreenGeometry();
  DrawScreen();
}


void DmxMonitor::DrawScreen(bool include_values) {
  if (include_values)
    erase();
  Mask();

  if (include_values)
    Values();
  refresh();

  if (m_data_loss_window)
    DrawDataLossWindow();
}


/* display the channels numbers */
void DmxMonitor::Mask() {
  unsigned int i = 0, x, y;
  unsigned int channel = first_channel;

  /* clear headline */
  (void) attrset(palette[HEADLINE]);
  move(0, 0);  // NOLINT(build/include_what_you_use) This is ncurses.h's move
  for (x = 0; static_cast<int>(x) < COLS; x++)
    addch(' ');

  if (COLS > 15) {
    mvprintw(0 , 0, "Universe: ");
    printw("%u", m_universe);
  }

  /* write channel numbers */
  (void) attrset(palette[CHANNEL]);
  for (y = 1;
       static_cast<int>(y) < LINES &&
       static_cast<int>(channel) < ola::DMX_UNIVERSE_SIZE &&
       i < channels_per_screen;
       y += ROWS_PER_CHANNEL_ROW) {
    move(y, 0);  // NOLINT(build/include_what_you_use) This is ncurses.h's move
    for (x = 0;
         static_cast<int>(x) < static_cast<int>(channels_per_line) &&
         static_cast<int>(channel) < ola::DMX_UNIVERSE_SIZE &&
         static_cast<int>(i < channels_per_screen);
         x++, i++, channel++) {
      switch (display_mode) {
        case DISP_MODE_HEX:
          printw("%03X ", channel + (m_channels_offset ? 1 : 0));
          break;
        case DISP_MODE_DMX:
        case DISP_MODE_DEC:
        default:
          printw("%03d ", channel + (m_channels_offset ? 1 : 0));
          break;
      }
    }
  }
}


/*
 * Update the screen with new values
 */
void DmxMonitor::Values() {
  int i = 0, x, y, z = first_channel;

  /* values */
  for (y = ROWS_PER_CHANNEL_ROW;
       y < LINES && z < ola::DMX_UNIVERSE_SIZE &&
       i < static_cast<int>(channels_per_screen);
       y += ROWS_PER_CHANNEL_ROW) {
    move(y, 0);  // NOLINT(build/include_what_you_use) This is ncurses.h's move
    for (x = 0;
         x < static_cast<int>(channels_per_line) &&
         z < ola::DMX_UNIVERSE_SIZE &&
         i < static_cast<int>(channels_per_screen);
         x++, z++, i++) {
      const int d = m_buffer.Get(z);
      switch (d) {
        case ola::DMX_MIN_SLOT_VALUE:
          (void) attrset(palette[ZERO]);
          break;
        case ola::DMX_MAX_SLOT_VALUE:
          (void) attrset(palette[FULL]);
          break;
        default:
          (void) attrset(palette[NORM]);
      }
      if (static_cast<int>(z) == current_channel)
        attron(A_REVERSE);
      switch (display_mode) {
        case DISP_MODE_HEX:
          if (d == 0) {
            if (static_cast<int>(m_buffer.Size()) <= z) {
              addstr("--- ");
            } else {
              addstr("    ");
            }
          } else {
            printw(" %02x ", d);
          }
          break;
        case DISP_MODE_DEC:
          if (d == 0) {
            if (static_cast<int>(m_buffer.Size()) <= z) {
              addstr("--- ");
            } else {
              addstr("    ");
            }
          } else if (d < 100) {
            printw(" %02d ", d);
          } else {
            printw("%03d ", d);
          }
          break;
        case DISP_MODE_DMX:
        default:
          switch (d) {
            case ola::DMX_MIN_SLOT_VALUE:
              if (static_cast<int>(m_buffer.Size()) <= z) {
                addstr("--- ");
              } else {
                addstr("    ");
              }
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


/* change palette to "p". If p is invalid new palette is number "0". */
void DmxMonitor::ChangePalette(int p) {
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
      m_palette_number = 0;
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
      goto color;

    case 2:
      init_pair(CHANNEL, COLOR_BLACK, COLOR_WHITE);
      init_pair(ZERO, COLOR_BLUE, COLOR_BLACK);
      init_pair(NORM, COLOR_GREEN, COLOR_BLACK);
      init_pair(FULL, COLOR_RED, COLOR_BLACK);
      init_pair(HEADLINE, COLOR_WHITE, COLOR_BLACK);
      init_pair(HEADEMPH, COLOR_CYAN, COLOR_BLACK);
      init_pair(HEADERROR, COLOR_RED, COLOR_BLACK);
      goto color;

    color:
      palette[CHANNEL] = COLOR_PAIR(CHANNEL);
      palette[ZERO] = COLOR_PAIR(ZERO);
      palette[NORM] = COLOR_PAIR(NORM);
      palette[FULL] = COLOR_PAIR(FULL);
      palette[HEADLINE] = COLOR_PAIR(HEADLINE);
      palette[HEADEMPH] = COLOR_PAIR(HEADEMPH);
      palette[HEADERROR] = COLOR_PAIR(HEADERROR);
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
}


/* calculate channels_per_line and channels_per_screen from LINES and COLS */
void DmxMonitor::CalcScreenGeometry() {
  int c = LINES;
  if (c < 3) {
    std::cerr << "Terminal must be more than 3 lines" << std::endl;
    exit(1);
  }
  c--;   // one line for headline
  if (c % ROWS_PER_CHANNEL_ROW == 1)
    c--;  // Need an even number of lines for data
  channels_per_line = COLS / CHANNEL_DISPLAY_WIDTH;
  channels_per_screen = channels_per_line * (c / ROWS_PER_CHANNEL_ROW);
}


/* signal handler for SIGWINCH */
void terminalresize(int sig) {
  dmx_monitor->TerminalResized();
  (void) sig;
}


/* cleanup handler for program exit. */
void cleanup() {
  if (dmx_monitor)
    delete dmx_monitor;
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
  "Monitor the values on a DMX512 universe.\n"
  "\n"
  "  -h, --help                   Display this help message and exit.\n"
  "  -u, --universe <universe_id> Id of universe to monitor (defaults to "
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

  options opts;

  ParseOptions(argc, argv, &opts);

  if (opts.help) {
    DisplayHelpAndExit(argv[0]);
  }

  dmx_monitor = new DmxMonitor(opts.universe);
  if (!dmx_monitor->Init())
    return 1;

  dmx_monitor->Run();
  delete dmx_monitor;
  dmx_monitor = NULL;
  return 0;
}
