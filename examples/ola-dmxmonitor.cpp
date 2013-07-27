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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Modified by Simon Newton (nomis52<AT>gmail.com) to use ola
 *
 * The (void) before attrset is due to a bug in curses. See
 * http://www.mail-archive.com/debian-bugs-dist@lists.debian.org/msg682294.html
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <curses.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
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
#endif
#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/OlaCallbackClient.h>
#include <ola/OlaClientWrapper.h>
#include <ola/DmxBuffer.h>
#include <ola/base/SysExits.h>
#include <ola/io/SelectServer.h>

#include <string>
#include <iostream>

using ola::DmxBuffer;
using ola::OlaCallbackClient;
using ola::OlaCallbackClientWrapper;
using ola::io::SelectServer;
using std::string;

static const unsigned int DEFAULT_UNIVERSE = 0;

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

static int display_mode = DISP_MODE_DMX;
static int current_channel = 0;    /* channel cursor is positioned on */
static int first_channel = 0;    /* channel in upper left corner */
static int channels_per_line = 80/4;
static int channels_per_screen = 80/4*24/2;
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
    };

    ~DmxMonitor() {
      if (m_window) {
        resetty();
        endwin();
      }
    }

    bool Init();
    void Run() { m_client.GetSelectServer()->Run(); }
    void NewDmx(unsigned int universe,
                const DmxBuffer &buffer,
                const string &error);
    void RegisterComplete(const string &error);
    void StdinReady();
    bool CheckDataLoss();
    void DrawDataLossWindow();
    void TerminalResized();

  private:
    unsigned int m_universe;
    unsigned int m_counter;
    int m_palette_number;
    ola::io::UnmanagedFileDescriptor m_stdin_descriptor;
    struct timeval m_last_data;
    WINDOW *m_window;
    WINDOW *m_data_loss_window;
    bool m_channels_offset;  // start from channel 1 rather than 0;
    OlaCallbackClientWrapper m_client;
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

  OlaCallbackClient *client = m_client.GetClient();
  client->SetDmxCallback(ola::NewCallback(this, &DmxMonitor::NewDmx));
  client->RegisterUniverse(
      m_universe,
      ola::REGISTER,
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
  timerclear(&m_last_data);
  DrawScreen();
  return true;
}


/*
 * Called when there is new DMX data
 */
void DmxMonitor::NewDmx(unsigned int universe,
                        const DmxBuffer &buffer,
                        const string &error) {
  m_buffer.Set(buffer);

  if (m_data_loss_window) {
    // delete the window
    wborder(m_data_loss_window, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(m_data_loss_window);
    delwin(m_data_loss_window);
    m_data_loss_window = NULL;
    Mask();
  }
  move(0, COLS - 1);
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
  gettimeofday(&m_last_data, NULL);
  Values();
  refresh();
  (void) universe;
  (void) error;
}


void DmxMonitor::RegisterComplete(const string &error) {
  if (!error.empty()) {
    std::cerr << "Register command failed with " << errno <<
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

    case 'l':
    case 'L':
    case KEY_RIGHT:
      if (current_channel < DMX_UNIVERSE_SIZE - 1) {
        current_channel++;
        if (current_channel >= first_channel + channels_per_screen) {
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
      if (current_channel >= DMX_UNIVERSE_SIZE)
        current_channel = DMX_UNIVERSE_SIZE - 1;
      if (current_channel >= first_channel+channels_per_screen) {
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
  struct timeval now, diff;

  if (timerisset(&m_last_data)) {
    gettimeofday(&now, NULL);
    timersub(&now, &m_last_data, &diff);
    if (diff.tv_sec > 2 || (diff.tv_sec == 2 && diff.tv_usec > 500000)) {
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
  int i = 0, x, y;
  int channel = first_channel;

  /* clear headline */
  (void) attrset(palette[HEADLINE]);
  move(0, 0);
  for (x = 0; x < COLS; x++)
    addch(' ');

  if (COLS > 15) {
    mvprintw(0 , 0, "Universe: ");
    printw("%u", m_universe);
  }

  /* write channel numbers */
  (void) attrset(palette[CHANNEL]);
  for (y = 1; y < LINES && channel < DMX_UNIVERSE_SIZE &&
       i < channels_per_screen; y+=2) {
    move(y, 0);
    for (x = 0; x < channels_per_line && channel < DMX_UNIVERSE_SIZE &&
         i < channels_per_screen; x++, i++, channel++) {
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
  for (y = 2; y < LINES && z < DMX_UNIVERSE_SIZE && i < channels_per_screen;
       y+=2) {
    move(y, 0);
    for (x = 0; x < channels_per_line && z < DMX_UNIVERSE_SIZE &&
        i < channels_per_screen; x++, z++, i++) {
      const int d = m_buffer.Get(z);
      switch (d) {
        case 0:
          (void) attrset(palette[ZERO]);
          break;
        case 255:
          (void) attrset(palette[FULL]);
          break;
        default:
          (void) attrset(palette[NORM]);
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
            case 0: addstr("    "); break;
            case 255: addstr(" FL "); break;
            default: printw(" %02d ", (d * 100) / 255);
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
      palette[CHANNEL]=COLOR_PAIR(CHANNEL);
      palette[ZERO]=COLOR_PAIR(ZERO);
      palette[NORM]=COLOR_PAIR(NORM);
      palette[FULL]=COLOR_PAIR(FULL);
      palette[HEADLINE]=COLOR_PAIR(HEADLINE);
      palette[HEADEMPH]=COLOR_PAIR(HEADEMPH);
      palette[HEADERROR]=COLOR_PAIR(HEADERROR);
      break;

    case 1:
      palette[CHANNEL]=A_REVERSE;
      palette[ZERO]=A_NORMAL;
      palette[NORM]=A_NORMAL;
      palette[FULL]=A_BOLD;
      palette[HEADLINE]=A_NORMAL;
      palette[HEADEMPH]=A_NORMAL;
      palette[HEADERROR]=A_BOLD;
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
  if (c % 2 == 1)
    c--;
  channels_per_line = COLS / 4;
  channels_per_screen = channels_per_line * c / 2;
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
