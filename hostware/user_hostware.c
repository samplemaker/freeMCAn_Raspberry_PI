/** \file hostware/user_hostware.c
 * \brief Freemcan-gc hostware
 *
 * \author Copyright (C) 2014 samplemaker
 * \author Copyright (C) 2010 Hans Ulrich Niedermann <hun@n-dimensional.de>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include "common_defs.h"


#ifndef max
  #define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

/* \TODO: Must be greater than maximum data sent by the kernel mod */
#define BUFMAX 4096

/* #define PRINT_VERBOSE */


static int fd_chardev;
static int fd_stdin;
static struct termios orig_term_attr;
static struct termios new_term_attr;

enum HOTKEYS{
  KEY_STOPMSRMNT = 'a',
  KEY_STARTMSRMNT = 's',
  KEY_SLOW = '1',
  KEY_FAST = '9',
  KEY_QUIT = 'q'
};


/** Get a filename for the logfile */
char *export_get_filename(const time_t time)
{
  const struct tm *tm_ = localtime(&time);
  char reason = 'X';
  char *prefix = "data";
  char date[128];
  strftime(date, sizeof(date), "%Y-%m-%d.%H:%M:%S", tm_);
  static char fname[256];
  snprintf(fname, sizeof(fname), "%s.%s.%c.%s", prefix, date, reason, "csv");
  return fname;
}


/** Plot the raw data */
void
print_buf(char *buffer, int len){
  for (int i = 0; i < len; i++){
     printf("%c",buffer[i]);
  }
  fflush(stdout);
}


/** The user state machine */
int
hostware_ctrl(char * ch){
  int ret_val = 0;
  switch (*ch) {
    case KEY_STARTMSRMNT:
      ret_val = ioctl(fd_chardev, IOCTL_START_MEASUREMENT, NULL);
      if (ret_val < 0)
        printf("ioctl failed:%d\n", ret_val);
      else
        printf("start measurement\n");
    break;
    case KEY_STOPMSRMNT:
      ret_val = ioctl(fd_chardev, IOCTL_STOP_MEASUREMENT, NULL);
      if (ret_val < 0)
        printf("ioctl failed:%d\n", ret_val);
      else
        printf("measurement stopped\n");
    break;
    case KEY_SLOW:
      ioctl(fd_chardev, IOCTL_SET_SLOW, NULL);
      if (ret_val < 0)
        printf("ioctl failed:%d\n", ret_val);
      else
        printf("set slow\n");
    break;
    case KEY_FAST:
      ioctl(fd_chardev, IOCTL_SET_FAST, NULL);
      if (ret_val < 0)
        printf("ioctl failed:%d\n", ret_val);
      else
        printf("set fast\n");
    break;
    case KEY_QUIT:
      return -1;
    break;
    default:
    break;
  }
  return 0;
}


/** Keyboard configuration to get rid of "return key" */
void keyboard_init(){
  /* set the terminal to raw mode */
  tcgetattr(fileno(stdin), &orig_term_attr);
  memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
  new_term_attr.c_lflag &= ~(ECHO|ICANON);
  new_term_attr.c_cc[VTIME] = 0;
  new_term_attr.c_cc[VMIN] = 0;
  tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);
}


/** Restore original keyboard configuration */
void keyboard_exit(){
  tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);
}


int
main (int argc, char *argv[])
{
  char buffer[BUFMAX];

  time_t now = time(NULL);
  const char * logfile_name = export_get_filename(now);
  FILE *fd_out = fopen(logfile_name, "w");
  if (fd_out == NULL) return -1;
  rewind(fd_out);

  fd_chardev = open("/dev/"DEVICE_NAME, O_RDONLY);
  printf("open character device: %d\n", fd_chardev);
  if (fd_chardev < 0) goto exit_nochardevice;

  /* fcntl(fd_chardev, F_SETFL, O_NONBLOCK); */

  fd_stdin = 0;
  keyboard_init();

  printf("hotkeys are: '%c':stop '%c':start '%c' slow '%c' fast '%c':quit\n",
         KEY_STOPMSRMNT,
         KEY_STARTMSRMNT,
         KEY_SLOW,
         KEY_FAST,
         KEY_QUIT);

  for (;;) {
    /* struct holding information about the  file descriptors
       to be supervised (a set) */
    fd_set rfds;
    /* nfds is the highest-numbered file descriptor
       in any of the three sets. argument select -> plus 1 */
    int nfds = 0;
    /* reset the set */
    FD_ZERO(&rfds);
    /* add the file descriptor */
    FD_SET(fd_stdin, &rfds);
    nfds = max(nfds, fd_stdin);
    FD_SET(fd_chardev, &rfds);
    nfds = max(nfds, fd_chardev);
    /* timeout */
    /* struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int retval = select(nfds + 1, &rfds, NULL, NULL, &tv); */
    int retval = select(nfds + 1, &rfds, NULL, NULL, NULL);
    #ifdef PRINT_VERBOSE
    printf("main loop:\n");
    #endif
    if (retval == -1)
      printf("select() error\n");
    else if (retval){
      if (FD_ISSET(fd_stdin, &rfds)){
        printf("\t2 (stdin): ");
        const int num_read = read(fd_stdin, buffer, BUFMAX);
        print_buf(buffer, num_read);
        printf("\n");
        if (hostware_ctrl(&buffer[0]) < 0)
          goto exit_normal;
      }
      if (FD_ISSET(fd_chardev, &rfds)){
        #ifdef PRINT_VERBOSE
        printf("\t3 (character device):\n");
        #endif
        const int num_read = read(fd_chardev, buffer, BUFMAX);
        fwrite(buffer, 1, num_read, fd_out);
        print_buf(buffer, num_read);
      }
    }
    #ifdef PRINT_VERBOSE
    else
      printf("\t4 (timeout)\n");
    #endif
  }

exit_normal:
  printf("close\n");
  keyboard_exit();
  close(fd_chardev);
exit_nochardevice:
  fclose(fd_out);
  exit(EXIT_SUCCESS);
}
