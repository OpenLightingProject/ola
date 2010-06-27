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
 *
 * Modified by Simon Newton (nomis52<AT>gmail.com) to use ola
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <curses.h>
#include <errno.h>
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
#include <sys/timeb.h>

#include <string>
#include <ola/Closure.h>
#include <ola/DmxBuffer.h>
#include <ola/OlaClient.h>
#include <ola/SimpleClient.h>
#include <ola/network/SelectServer.h>

using ola::SimpleClient;
using ola::OlaClient;
using ola::network::SelectServer;
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

int MAXCHANNELS=512;
unsigned int MAXFKEY=12;

int universe = 0;

typedef unsigned char dmx_t ;

static dmx_t *dmx;
static dmx_t *dmxsave;
static dmx_t *dmxundo;

static int display_mode = DISP_MODE_DMX;
static int current_channel = 0;    /* channel cursor is positioned on */
static int first_channel = 0;    /* channel in upper left corner */
static int channels_per_line=80/4;
static int channels_per_screen=80/4*24/2;
static int undo_possible=0;
static int current_cue=0;    /* select with F keys */
static float fadetime=1.0f;
static int fading=0;        /* percentage counter of fade process */
static int palette_number=0;
static int palette[MAXCOLOR];
string error_str;
static int channels_offset=1;

OlaClient *client;
SelectServer *ss;


void DMXsleep(int usec) {
  struct timeval tv;
  tv.tv_sec = usec/1000000;
  tv.tv_usec = usec%1000000;
  if(select(1, NULL, NULL, NULL, &tv) < 0)
    perror("could not select");
}

// returns the time in milliseconds
unsigned long timeGetTime() {
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv ;
  gettimeofday(&tv, NULL) ;
  return (unsigned long)tv.tv_sec*1000UL+ (unsigned long)tv.tv_usec/1000;

#else
# ifdef HAVE_FTIME
  struct timeb t;
  ftime(&t);
  return (unsigned long)t.time*1000UL+(unsigned long)t.millitm;
# else

# endif
#endif
}



/* set all DMX channels */
void setall() {
  ola::DmxBuffer buffer(dmx, MAXCHANNELS);
  client->SendDmx(universe, buffer);
}

/* set current DMX channel */
void set() {
  setall() ;
}


/* display the channels numbers */
void mask() {
  int i=0,x,y,z=first_channel;

  erase();

  /* clear headline */
  attrset(palette[HEADLINE]);
  move(0,0);
  for (x=0; x<COLS; x++)
    addch(' ');

  /* write channel numbers */
  attrset(palette[CHANNEL]);
  for (y=1; y<LINES && z<MAXCHANNELS && i<channels_per_screen; y+=2) {
      move(y,0);
      for(x=0; x<channels_per_line && z<MAXCHANNELS && i<channels_per_screen; x++, i++, z++)
    switch(display_mode) {
      case DISP_MODE_DMX:
      case DISP_MODE_DEC:
      default:
        printw("%03d ",z+channels_offset); break;

      case DISP_MODE_HEX:
        printw("%03X ",z+channels_offset); break;
      }
    }

}

/* update the screen */
void values() {
  int i=0,x,y,z=first_channel;

  /* headline */
  if(COLS>24) {
      time_t t=time(NULL);
      struct tm *tt=localtime(&t);
      char *s=asctime(tt);
      s[strlen(s)-1]=0; /* strip newline at end of string */

      attrset(palette[HEADLINE]);
      mvprintw(0,1,"%s", s);
    }
  if(COLS>31) {
      attrset(palette[HEADLINE]);
      printw(" cue:");
      attrset(palette[HEADEMPH]);
      printw("%02i", current_cue+1);
    }
  if(COLS>44) {
      attrset(palette[HEADLINE]);
      printw(" fadetime:");

      attrset(palette[HEADEMPH]);
      printw("%1.1f", fadetime);
    }
  if(COLS>55) {
    if(fading) {
      attrset(palette[HEADLINE]);
      printw(" fading:");

      attrset(palette[HEADEMPH]);
      printw("%02i%%", (fading<100)?fading:99);
    } else {
      attrset(palette[HEADLINE]);
      printw("           ");
    }
  }

  if(COLS>80)
    if(!error_str.empty()) {
      attrset(palette[HEADERROR]);
      printw("ERROR:%s", error_str.data());
    }

  /* values */
  for(y=2; y<LINES && z<MAXCHANNELS && i<channels_per_screen; y+=2) {
      move(y,0);
      for(x=0; x<channels_per_line && z<MAXCHANNELS && i<channels_per_screen; x++, z++, i++) {
      const int d=dmx[z];
      switch(d) {
        case 0: attrset(palette[ZERO]); break;
        case 255: attrset(palette[FULL]); break;
        default: attrset(palette[NORM]);
        }
      if(z==current_channel)
        attron(A_REVERSE);
      switch(display_mode) {
        case DISP_MODE_HEX:
          if(d==0)
        addstr("    ");
          else
        printw(" %02x ", d);
          break;
        case DISP_MODE_DEC:
          if(d==0)
        addstr("    ");
          else if(d<100)
        printw(" %02d ", d);
          else
        printw("%03d ", d);
          break;
        case DISP_MODE_DMX:
        default:
          switch(d)
        {
        case 0: addstr("    "); break;
        case 255: addstr(" FL "); break;
        default: printw(" %02d ", (d*100)/255);
        }
        }
    }
    }
}

/* save current cue into cuebuffer */
void savecue() {
  memcpy(&dmxsave[current_cue*MAXCHANNELS], dmx, MAXCHANNELS);
}

/* get new cue from cuebuffer */
void loadcue() {
  memcpy(dmx, &dmxsave[current_cue*MAXCHANNELS], MAXCHANNELS);
}

/* fade cue "new_cue" into current cue */
void crossfade(unsigned int new_cue) {
  dmx_t *dmxold;
  dmx_t *dmxnew;
  int i;
  int max=MAXCHANNELS;

  /* check parameter */
  if(new_cue>MAXFKEY || new_cue<0)
    return;

  undo_possible=0;

  /* don't crossfade for small fadetimes */
  if(fadetime<0.1f) {
      savecue();
      current_cue=new_cue;
      loadcue();
      setall();
      return;
    }

  savecue();
  dmxold=&dmxsave[current_cue*MAXCHANNELS];
  dmxnew=&dmxsave[new_cue*MAXCHANNELS];

  /* try to find the last channel value > 0, so we don't have to
     crossfade large blocks of 0s */
  for(i=MAXCHANNELS-1; i>=0; max=i, i--)
    if(dmxold[i]||dmxnew[i])
      break;

  {
    const unsigned long tstart=timeGetTime();
    const unsigned long tend=tstart+(int)(fadetime*1000.0);
    unsigned long t=tstart;
    while(t<=tend)
      {
    /* calculate new cue */
    t=timeGetTime();
    {
      const float p=(float)(t-tstart)/1000.0f/fadetime;
      const float q=1.0f-p;
      for(i=0; i<max; i++)
        if(dmxold[i] || dmxnew[i]) /* avoid calculating with only 0 */
          dmx[i]=(int)((float)dmxold[i]*q + (float)dmxnew[i]*p);
      setall();

      /* update screen */
      fading=(int)(p*100.0f);
      values();
      refresh();
      DMXsleep(100000);

      t=timeGetTime();        /* get current time, because the last time is too old (due to the sleep) */
    }
      }
    fading=0;

    /* set the new cue */
    current_cue=new_cue;
    loadcue();
    setall();
  }
}


void undo() {
  if(undo_possible) {
      memcpy(dmx, dmxundo, MAXCHANNELS);
      undo_possible = 0;
    }
}

void undoprep() {
  memcpy(dmxundo, dmx, MAXCHANNELS);
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
  switch(p) {
    default:
      palette_number=0;
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
  int c=LINES;
  if (c < 3) {
      error_str ="screen to small, we need at least 3 lines";
      exit(1);
    }
  c--;                /* one line for headline */
  if (c % 2 == 1)
    c--;
  channels_per_line=COLS/4;
  channels_per_screen=channels_per_line*c/2;
}

/* signal handler for SIGWINCH */
void terminalresize(int sig) {
  struct winsize size;
  if (ioctl(0, TIOCGWINSZ, &size) < 0)
    return;

  resizeterm(size.ws_row, size.ws_col);
  calcscreengeometry();
  mask();
}

WINDOW  *w=NULL;

/* cleanup handler for program exit. */
void cleanup() {
  if(w) {
    resetty();
    endwin();
  }

  if (!error_str.empty())
    puts(error_str.data());
}

void stdin_ready() {
  int n;
  int c = wgetch(w);
  switch (c) {
    case KEY_PPAGE:
      undoprep();
      if(dmx[current_channel] < 255-0x10)
        dmx[current_channel]+=0x10;
      else
        dmx[current_channel]=255;
      set();
      break;

    case '+':
      if(dmx[current_channel] < 255)
        {
          undoprep();
          dmx[current_channel]++;
        }
      set();
      break;

    case KEY_NPAGE:
      undoprep();
      if(dmx[current_channel]==255)
        dmx[current_channel]=0xe0;
      else if(dmx[current_channel] > 0x10)
        dmx[current_channel]-=0x10;
      else
        dmx[current_channel] = 0;
      set();
      break;

    case '-':
      if(dmx[current_channel] > 0) {
        undoprep();
        dmx[current_channel]--;
      }
      set();
      break;

    case ' ':
      undoprep();
      if(dmx[current_channel]<128)
        dmx[current_channel]=255;
      else
        dmx[current_channel]=0;
      set();
      break;

    case '0' ... '9':
      fadetime=c-'0';
      break;

    case KEY_HOME:
      current_channel=0;
      first_channel=0;
      mask();
      break;

    case KEY_RIGHT:
      if(current_channel < MAXCHANNELS-1) {
        current_channel++;
        if(current_channel >= first_channel+channels_per_screen) {
          first_channel+=channels_per_line;
          mask();
        }
      }
      break;

    case KEY_LEFT:
      if(current_channel > 0) {
        current_channel--;
        if(current_channel < first_channel) {
          first_channel-=channels_per_line;
          if(first_channel<0)
            first_channel=0;
          mask();
        }
      }
      break;

    case KEY_DOWN:
      current_channel+=channels_per_line;
      if(current_channel>=MAXCHANNELS)
        current_channel=MAXCHANNELS-1;
      if(current_channel >= first_channel+channels_per_screen) {
        first_channel+=channels_per_line;
        mask();
      }
      break;

    case KEY_UP:
      current_channel-=channels_per_line;
      if(current_channel<0)
        current_channel=0;
      if(current_channel < first_channel) {
        first_channel-=channels_per_line;
        if(first_channel<0)
          first_channel=0;
        mask();
      }
      break;

    case KEY_IC:
      undoprep();
      for (n = MAXCHANNELS-1; n>current_channel && n>0; n--)
        dmx[n]=dmx[n-1];
      setall();
      break;
    case KEY_DC:
      undoprep();
      for(n=current_channel; n<MAXCHANNELS-1; n++)
        dmx[n]=dmx[n+1];
      setall();
      break;
    case 'B':
    case 'b':
      undoprep();
      memset(dmx, 0, MAXCHANNELS);
      setall();
      break;
    case 'F':
    case 'f':
      undoprep();
      memset(dmx, 0xff, MAXCHANNELS);
      setall();
      break;
    case 'M':
    case 'm':
      if(++display_mode>=DISP_MODE_MAX)
        display_mode=0;
      mask();
      break;
    case 'N':
    case 'n':
      if(++channels_offset>1)
        channels_offset=0;
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
    default:
      if (c >= (int) KEY_F(1) && c <= (int) KEY_F(MAXFKEY))
        crossfade(c - KEY_F(1));
      break;
  }
  values();
  refresh();
}

int main (int argc, char *argv[]) {
  int optc;

  signal(SIGWINCH, terminalresize);
  atexit(cleanup);

  /* alloc */
  dmx = (dmx_t*) calloc(MAXCHANNELS+10, sizeof(dmx_t)); /* 10 bytes security, for file IO routines, will be optimized and checked later */
  CHECK(dmx);

  dmxsave=(dmx_t*)  calloc(MAXCHANNELS*MAXFKEY, sizeof(dmx_t));
  CHECK(dmxsave);

  dmxundo=(dmx_t*) calloc(MAXCHANNELS, sizeof(dmx_t));
  CHECK(dmxundo);

  // parse options
  while ((optc = getopt (argc, argv, "u:")) != EOF) {
    switch (optc) {
      case 'u':
         universe = atoi(optarg) ;
         break ;
      default:
         break;
    }
  }

  /* set up ola connection */
  SimpleClient ola_client;
  ola::network::UnmanagedSocket stdin_socket(0);
  stdin_socket.SetOnData(ola::NewClosure(&stdin_ready));

  if (!ola_client.Setup()) {
    printf("error: %s", strerror(errno));
    exit(1);
  }

  client = ola_client.GetClient();
  ss = ola_client.GetSelectServer();
  ss->AddSocket(&stdin_socket);

  /* init curses */
  w = initscr();
  if (!w) {
    printf ("unable to open main-screen\n");
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
