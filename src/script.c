/*
 * Licensed to Cloudkick, Inc under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * libcloud.org licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ckl.h"

/* TODO: detect headers in sconsript */
#ifdef __linux__
#include <pty.h>
#include <sys/wait.h>
#include <utmp.h>
#else
#ifdef __FreeBSD__
#include <termios.h>
#include <libutil.h>
#else
#include <util.h>
#endif
#endif

#include <sys/ioctl.h>

#ifndef BUFSIZ
#define BUFSIZ 8096
#endif

int ckl_script_init(ckl_script_t *s, ckl_conf_t *conf)
{
  int rv;

  rv = ckl_tmp_file(&s->path, &s->fd);
  if (rv < 0) {
    ckl_error_out("failed to create script temp file");
    return rv;
  }

  const char *sh = getenv("SHELL");

  if (!sh) {
    sh = "/bin/sh";
  }

  s->shell = strdup(sh);

  return 0;
}

#define USE_SIMPLE_SCRIPT

#ifdef USE_SIMPLE_SCRIPT

int ckl_script_record(ckl_script_t *s, ckl_msg_t *msg)
{
  int rv;
  char buf[2048];
  snprintf(buf, sizeof(buf), "script '%s'", s->path);

  rv = system(buf);
  if (rv < 0) {
    fprintf(stderr, "os.system failed for cmd '%s'\n", buf);
    perror("system(): ");
    return -1;
  }

  msg->script_log = strdup(s->path);

  return 0;
}

#else

static int g_script_done = 0;
static int g_script_child_pid = 0;
static void script_child_finished(int signo)
{
  int rv = 0;
  union wait status;

  do {
    rv = wait3((int *)&status, WNOHANG, 0);
    if (rv == g_script_child_pid) {
      g_script_done = 1;
      break;
    }
  } while (rv > 0);
}

static void script_write_output(ckl_script_t *s, int mpty, int spty)
{
  int rv;
  int i = 0;
  close(STDIN_FILENO);

  char buf[BUFSIZ];
  while (1) {
    i++;
    rv = read(mpty, buf, sizeof(buf));
    if (rv > 0) {
      write(1, buf, rv);
      fwrite(buf, 1, rv, s->fd);
      if (i % 10 == 0) {
        fflush(s->fd);
      }
    }
    else {
      break;
    }
  }

  fclose(s->fd);
  close(mpty);
  exit(0);
}

static void script_start_shell(ckl_script_t *s, int mpty, int spty)
{
  int rv = 0;
  close(mpty);

  rv = login_tty(spty);
  if (rv) {
    perror("login_tty(slave) failed:");
    exit(EXIT_FAILURE);
  }

  execl(s->shell, s->shell, "-i", NULL);
  perror("execl of shell failed!");
  exit(EXIT_FAILURE);
}

int ckl_script_record(ckl_script_t *s, ckl_msg_t *msg)
{
  int rv;
  int mpty = 0;
  int spty = 0;
  struct termios parent_term;

  /**
   * Rough Flow:
   * - Open Sub PTY <http://www.gnu.org/software/hello/manual/libc/Pseudo_002dTerminals.html#Pseudo_002dTerminals>
   * - Setup it like the current one
   *      See: man TCSETATTR(3) <http://www.freebsd.org/cgi/man.cgi?query=tcsetattr>
   * - Double fork()
   *    - Spawn Shell on it (via fork/execl)
   *    - read/write from master process
   * - Record all input/output to the shell to our file.
   * - on exit of child:
   *    - store file in ckl_msg_t
   *    - cleanup
   */

  {
    struct winsize parent_win;
    struct termios child_term;

    rv = tcgetattr(STDIN_FILENO, &parent_term);
    if (rv != 0) {
      perror("tcgetattr(STDIN_FILENO) failed:");
      return rv;
    }

    rv = ioctl(STDIN_FILENO, TIOCGWINSZ, &parent_win);
    if (rv != 0) {
      perror("ioctl(TIOCGWINSZ on parent) failed:");
      return rv;
    }

    rv = openpty(&mpty, &spty, NULL, &parent_term, &parent_win);
    if (rv != 0) {
      perror("openpty() failed:");
      return rv;
    }

    child_term = parent_term;

    cfmakeraw(&child_term);
    child_term.c_lflag &= ~ECHO;

    rv = tcsetattr(STDIN_FILENO, TCSAFLUSH, &child_term);
    if (rv != 0) {
      perror("tcsetattr(TCSAFLUSH on child) failed:");
      return rv;
    }
  }

  {
    int child = 0;
    signal(SIGCHLD, script_child_finished);

    g_script_child_pid = child = fork();
    if (child < 0) {
      perror("fork() to child failed:");
      return child;
    }

    if (child == 0) {
      int subchild = child = fork();
      if (child < 0) {
        perror("fork() to 2nd child failed:");
        return child;
      }

      if (child) {
        script_write_output(s, mpty, spty);
      }
      else {
        script_start_shell(s, mpty, spty);
      }
    }
  }

  /* main process ! */
  {
    char buf[BUFSIZ];
    while (g_script_done == 0) {
      rv = read(STDIN_FILENO, buf, sizeof(buf));
      if (rv > 0) {
        write(mpty, buf, rv);
      }
      else {
        break;
      }
    }
  }

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &parent_term);

  msg->script_log = strdup(s->path);
  close(mpty);
  close(spty);
  return 0;
}

#endif

void ckl_script_free(ckl_script_t *s)
{
  if (s->fd != NULL){
    fclose(s->fd);
  }
  if (s->path) {
    unlink(s->path);
    free(s->path);
  }
  if (s->shell) {
    free((char*)s->shell);
  }
  free(s);
}

