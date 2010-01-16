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
#include <sys/timeb.h>

#include <ola/BaseTypes.h>
#include <ola/Closure.h>
#include <ola/OlaClient.h>
#include <ola/SimpleClient.h>
#include <ola/DmxBuffer.h>
#include <ola/network/SelectServer.h>

#include <string>
#include <iostream>

using ola::DmxBuffer;
using ola::OlaClient;
using ola::OlaClientObserver;
using ola::network::SelectServer;
using ola::SimpleClient;
using std::string;

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

class DmxMonitor *dmx_monitor;

static int display_mode = DISP_MODE_DMX;
static int current_channel = 0;    /* channel cursor is positioned on */
static int first_channel = 0;    /* channel in upper left corner */
static int channels_per_line = 80/4;
static int channels_per_screen = 80/4*24/2;
static int palette[MAXCOLOR];

/*
 * The observer class which repsonds to events
 */
class DmxMonitor: public OlaClientObserver {
  public:
    explicit DmxMonitor(unsigned int universe):
        m_universe(universe),
        m_counter(0),
        m_palette_number(0),
        m_stdin_socket(STDIN_FILENO),
        m_window(NULL),
        m_data_loss_window(NULL),
        m_channels_offset(false) {
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
    int StdinReady();
    int CheckDataLoss();
    void TerminalResized();

  private:
    unsigned int m_universe;
    unsigned int m_counter;
    int m_palette_number;
    ola::network::UnmanagedSocket m_stdin_socket;
    struct timeval m_last_data;
    WINDOW *m_window;
    WINDOW *m_data_loss_window;
    bool m_channels_offset;  // start from channel 1 rather than 0;
    SimpleClient m_client;

    void Mask();
    void Values(const DmxBuffer &buffer);
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
  client->SetObserver(this);
  client->RegisterUniverse(m_universe, ola::REGISTER);

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

  m_client.GetSelectServer()->AddSocket(&m_stdin_socket);
  m_stdin_socket.SetOnData(ola::NewClosure(this, &DmxMonitor::StdinReady));
  m_client.GetSelectServer()->RegisterRepeatingTimeout(
      500,
      ola::NewClosure(this, &DmxMonitor::CheckDataLoss));
  CalcScreenGeometry();
  ChangePalette(m_palette_number);

  DmxBuffer empty_buffer;
  empty_buffer.Blackout();
  NewDmx(m_universe, empty_buffer, "");

  timerclear(&m_last_data);
  return true;
}


/*
 * Called when there is new DMX data
 */
void DmxMonitor::NewDmx(unsigned int universe,
                      const DmxBuffer &buffer,
                      const string &error) {
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
  Values(buffer);
  refresh();
}


/*
 * Called when there is input from the keyboard
 */
int DmxMonitor::StdinReady() {
  int c = wgetch(m_window);

  switch (c) {
    case KEY_HOME:
      current_channel = 0;
      first_channel = 0;
      Mask();
      break;

    case KEY_RIGHT:
      if (current_channel < DMX_UNIVERSE_SIZE-1) {
        current_channel++;
        if (current_channel >= first_channel + channels_per_screen) {
            first_channel += channels_per_line;
            Mask();
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
          Mask();
        }
      }
      break;

    case KEY_DOWN:
      current_channel += channels_per_line;
      if (current_channel >= DMX_UNIVERSE_SIZE)
        current_channel = DMX_UNIVERSE_SIZE - 1;
      if (current_channel >= first_channel+channels_per_screen) {
        first_channel += channels_per_line;
        Mask();
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
        Mask();
      }
      break;

    case 'M':
    case 'm':
      if (++display_mode >= DISP_MODE_MAX)
        display_mode = 0;
      Mask();
      break;

    case 'N':
    case 'n':
      m_channels_offset = !m_channels_offset;
      Mask();
      break;

    case 'P':
    case 'p':
      ChangePalette(++m_palette_number);
      break;

    case 'Q':
    case 'q':
      m_client.GetSelectServer()->Terminate();
      break;
    default:
        break;
  }
  return 0;
}


/*
 * Check for data loss.
 * TODO(simon): move to the ola server
 */
int DmxMonitor::CheckDataLoss() {
  struct timeval now, diff;

  if (timerisset(&m_last_data)) {
    gettimeofday(&now, NULL);
    timersub(&now, &m_last_data, &diff);
    if (!m_data_loss_window &&
        (diff.tv_sec > 2 || (diff.tv_sec == 2 && diff.tv_usec > 500000))) {
      // loss of data
      m_data_loss_window = newwin(3, 14, (LINES - 14) / 2, (COLS - 3) / 2);
      mvwprintw(m_data_loss_window, 1, 2, "Data Loss!");
      wborder(m_data_loss_window, '|', '|', '-', '-', '+', '+', '+', '+');
      wrefresh(m_data_loss_window);
    }
  }
  return 0;
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
  Mask();
}


/* display the channels numbers */
void DmxMonitor::Mask() {
  int i = 0, x, y;
  int channel = first_channel;

  erase();

  /* clear headline */
  attrset(palette[HEADLINE]);
  move(0, 0);
  for (x = 0; x < COLS; x++)
    addch(' ');

  /* write channel numbers */
  attrset(palette[CHANNEL]);
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
void DmxMonitor::Values(const DmxBuffer &buffer) {
  int i = 0, x, y, z = first_channel;

  /* headline */
  if (COLS>24) {
      time_t t = time(NULL);
      struct tm *tt = localtime(&t);
      char *s = asctime(tt);
      s[strlen(s) - 1] = 0; /* strip newline at end of string */

      attrset(palette[HEADLINE]);
      mvprintw(0, 1, "%s", s);
    }
  if (COLS > 31) {
      attrset(palette[HEADLINE]);
      printw(" Universe:");
      attrset(palette[HEADEMPH]);
      printw("%02i", m_universe);
    }

  /* values */
  for (y = 2; y < LINES && z < DMX_UNIVERSE_SIZE && i < channels_per_screen;
       y+=2) {
    move(y, 0);
    for (x = 0; x < channels_per_line && z < DMX_UNIVERSE_SIZE &&
        i < channels_per_screen; x++, z++, i++) {
      const int d = buffer.Get(z);
      switch (d) {
        case 0: attrset(palette[ZERO]); break;
        case 255: attrset(palette[FULL]); break;
        default: attrset(palette[NORM]);
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

  Mask();
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
}


/* cleanup handler for program exit. */
void cleanup() {
  if (dmx_monitor)
    delete dmx_monitor;
}


int main(int argc, char *argv[]) {
  int optc;
  int universe = 0;
  atexit(cleanup);

  // parse options
  while ((optc = getopt(argc, argv, "u:")) != EOF) {
      switch (optc) {
          case 'u':
          universe = atoi(optarg);
          break;
      default:
          break;
      }
  }

  dmx_monitor = new DmxMonitor(universe);
  if (!dmx_monitor->Init())
    return 1;

  dmx_monitor->Run();
  delete dmx_monitor;
  dmx_monitor = NULL;
  return 0;
}
