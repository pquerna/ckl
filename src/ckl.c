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

#include "ckl_version.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

typedef struct ckl_transport_t {
  CURL *curl;
  struct curl_httppost *formpost;
  struct curl_httppost *lastptr;
} ckl_transport_t;

typedef struct ckl_conf_t {
  const char *endpoint;
  const char *secret;
} ckl_conf_t;

typedef struct ckl_msg_t {
  time_t ts;
  const char *username;
  const char *hostname;
  const char *msg;
} ckl_msg_t;


static void error_out(const char *msg)
{
  fprintf(stderr, "ERROR: %s\n", msg);
  exit(EXIT_FAILURE);
}

static int msg_to_post_data(ckl_transport_t *t,
                            ckl_conf_t *conf,
                            ckl_msg_t* m)
{
  curl_formadd(&t->formpost,
               &t->lastptr,
               CURLFORM_COPYNAME, "username",
               CURLFORM_COPYCONTENTS, m->username,
               CURLFORM_END);

  curl_formadd(&t->formpost,
               &t->lastptr,
               CURLFORM_COPYNAME, "hostname",
               CURLFORM_COPYCONTENTS, m->hostname,
               CURLFORM_END);

  curl_formadd(&t->formpost,
               &t->lastptr,
               CURLFORM_COPYNAME, "msg",
               CURLFORM_COPYCONTENTS, m->msg,
               CURLFORM_END);

  curl_easy_setopt(t->curl, CURLOPT_HTTPPOST, t->formpost);
 
  return 0;
}


static int read_config(ckl_conf_t *conf)
{
  /* TODO: parse /etc/ckl.conf, ~/.ckl */
  conf->endpoint = strdup("http://127.0.0.1/ckl");
  conf->secret = strdup("super-secret");
}

/* Tries to find the first available editor to create a log message.
 * Attemps to use one of the following variables:
 *  - CKL_EDITOR
 *  - VISUAL
 *  - EDITOR
 *
 * returns 0 on succes, <0 if failed */
static int find_editor(const char **output)
{
  const char *c;

  c = getenv("CKL_EDITOR");
  if (c) {
    *output = c;
    return 0;
  }

  c = getenv("VISUAL");
  if (c) {
    *output = c;
    return 0;
  }

  c = getenv("EDITOR");
  if (c) {
    *output = c;
    return 0;
  }

  return -1;
}

static int build_msg(ckl_msg_t *msg)
{
  {
    const char *user = getenv("SUDO_USER");

    if (user == NULL) {
      user = getlogin();
    }

    if (user == NULL) {
      error_out("Unknown user: getlogin(2) and SUDO_USER both returned NULL.");
    }

    msg->username = strdup(user);
  }

  {
    char buf[HOST_NAME_MAX+1];
    buf[HOST_NAME_MAX] = '\0';
    int rv;

    rv = gethostname(&buf[0], HOST_NAME_MAX);
    if (rv < 0) {
      error_out("gethostname returned -1.  Is your hostname set?");
    }

    msg->hostname = strdup(buf);
  }

  return 0;
}

static void show_help()
{
  fprintf(stdout, "ckl - %d.%d.%d\n", CKL_VERSION_MAJOR, CKL_VERSION_MINOR, CKL_VERSION_PATCH);
  fprintf(stdout, "ckl logs a message about an operation on a server to an HTTP endpoint\n");
  fprintf(stdout, "  Usage:  ckl [-h] [-m message]\n\n");
  fprintf(stdout, "See `man ckl` for more details\n");
  exit(EXIT_SUCCESS);
}

int main(int argc, char *const *argv)
{
  int c;
  int rv;
  const char *editor;
  ckl_msg_t msg;
  ckl_conf_t conf;
  ckl_transport_t transport;

  curl_global_init(CURL_GLOBAL_ALL);

  while ((c = getopt (argc, argv, "hm:")) != -1) {
    switch (c) {
      case 'h':
        show_help();
        break;
      case '?':
        break;
    }
  }

  rv = find_editor(&editor);
  if (rv < 0) {
    error_out("unable to find text editor. Set EDITOR or use -m");
  }

  rv = build_msg(&msg);
  if (rv < 0) {
    error_out("build_msg failed.");
  }

  return 0;
}