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
  struct curl_slist *headerlist;
  struct curl_httppost *formpost;
  struct curl_httppost *lastptr;
} ckl_transport_t;

typedef struct ckl_conf_t {
  int quiet;
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


static int msg_send(ckl_transport_t *t,
                    ckl_conf_t *conf,
                    ckl_msg_t* m)
{
  CURLcode res;
  int rv = msg_to_post_data(t, conf, m);

  if (rv < 0) {
    return rv;
  }

  if (!conf->quiet) {
    fprintf(stdout, "Sending.... ");
    fflush(stdout);
  }

  res = curl_easy_perform(t->curl);

  if (res != 0) {
    fprintf(stderr, "Error pushing to %s: (%d) %s\n\n",
            conf->endpoint, res, curl_easy_strerror(res));
    return -1;
  }

  if (!conf->quiet) {
    fprintf(stdout, " Done!\n");
    fflush(stdout);
  }

  return 0;
}

static int conf_init(ckl_conf_t *conf)
{
  /* TODO: parse /etc/ckl.conf, ~/.ckl */
  conf->endpoint = strdup("http://127.0.0.1/ckl");
  conf->secret = strdup("super-secret");
  return 0;
}

static void conf_free(ckl_conf_t *conf)
{
  free((char*)conf->endpoint);
  free((char*)conf->secret);
  free(conf);
}

static int msg_init(ckl_msg_t *msg, const char *usermsg)
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

  msg->msg = strdup(usermsg);

  msg->ts = time(NULL);

  return 0;
}

static void msg_free(ckl_msg_t *m)
{
  free((char*)m->username);
  free((char*)m->hostname);
  free((char*)m->msg);
  free(m);
}


static int transport_init(ckl_transport_t *t, ckl_conf_t *conf)
{
  static const char buf[] = "Expect:";

  t->curl = curl_easy_init();

  curl_easy_setopt(t->curl, CURLOPT_URL, conf->endpoint);
  
  /* TODO: this is less than optimal */
  curl_easy_setopt(t->curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(t->curl, CURLOPT_SSL_VERIFYHOST, 0L);

  t->headerlist = curl_slist_append(t->headerlist, buf);
  curl_easy_setopt(t->curl, CURLOPT_HTTPHEADER, t->headerlist);

  return 0;
}

static void transport_free(ckl_transport_t *t)
{
  curl_easy_cleanup(t->curl);
  curl_formfree(t->formpost);
  curl_slist_free_all(t->headerlist);
  free(t);
}

/* Tries to find the first available editor to create a log message.
 * Attemps to use one of the following variables:
 *  - CKL_EDITOR
 *  - VISUAL
 *  - EDITOR
 *
 * returns 0 on succes, <0 if failed */
static int editor_find(const char **output)
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

static void show_version()
{
  fprintf(stdout, "ckl - %d.%d.%d\n", CKL_VERSION_MAJOR, CKL_VERSION_MINOR, CKL_VERSION_PATCH);
  exit(EXIT_SUCCESS);
}

static void show_help()
{
  fprintf(stdout, "ckl - Configuration Changelog tool\n");
  fprintf(stdout, "  Usage:  ckl [-h] [-V] [-m message]\n\n");
  fprintf(stdout, "     -h          Show Help message\n");
  fprintf(stdout, "     -V          Show Version number\n");
  fprintf(stdout, "     -m (msg)    Set the log message, if none is set, an editor will be invoked.\n");
  fprintf(stdout, "See `man ckl` for more details\n");
  exit(EXIT_SUCCESS);
}

int main(int argc, char *const *argv)
{
  int c;
  int rv;
  const char *editor;
  const char *usermsg = NULL;
  ckl_msg_t *msg = calloc(1, sizeof(ckl_msg_t));
  ckl_conf_t *conf = calloc(1, sizeof(ckl_conf_t));
  ckl_transport_t *transport = calloc(1, sizeof(ckl_transport_t));

  curl_global_init(CURL_GLOBAL_ALL);

  while ((c = getopt(argc, argv, "hVm:")) != -1) {
    switch (c) {
      case 'V':
        show_version();
        break;
      case 'h':
        show_help();
        break;
      case 'm':
        usermsg = optarg;
        break;
      case '?':
        error_out("See -h for correct options");
        break;
    }
  }

  if (usermsg == NULL) {
    rv = editor_find(&editor);
    if (rv < 0) {
      error_out("unable to find text editor. Set EDITOR or use -m");
    }
  }

  if (usermsg == NULL) {
    error_out("no message specified");
  }

  rv = conf_init(conf);
  if (rv < 0) {
    error_out("conf_init failed");
  }

  rv = msg_init(msg, usermsg);
  if (rv < 0) {
    error_out("msg_init failed.");
  }

  rv = transport_init(transport, conf);
  if (rv < 0) {
    error_out("transport_init failed.");
  }

  rv = msg_send(transport, conf, msg);
  if (rv < 0) {
    error_out("msg_send failed.");
  }

  transport_free(transport);
  conf_free(conf);
  msg_free(msg);
  curl_global_cleanup();

  return 0;
}
