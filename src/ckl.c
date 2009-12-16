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
#include <ctype.h>

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
  char buf[128];

  snprintf(buf, sizeof(buf), "%d", m->ts);

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

  curl_formadd(&t->formpost,
               &t->lastptr,
               CURLFORM_COPYNAME, "secret",
               CURLFORM_COPYCONTENTS, conf->secret,
               CURLFORM_END);

  curl_formadd(&t->formpost,
               &t->lastptr,
               CURLFORM_COPYNAME, "ts",
               CURLFORM_COPYCONTENTS, buf,
               CURLFORM_END);

  curl_easy_setopt(t->curl, CURLOPT_HTTPPOST, t->formpost);
 
  return 0;
}


static int msg_send(ckl_transport_t *t,
                    ckl_conf_t *conf,
                    ckl_msg_t* m)
{
  long httprc = -1;
  CURLcode res;
  int rv = msg_to_post_data(t, conf, m);

  if (rv < 0) {
    return rv;
  }

  res = curl_easy_perform(t->curl);

  if (res != 0) {
    fprintf(stderr, "Failed talking to endpoint %s: (%d) %s\n\n",
            conf->endpoint, res, curl_easy_strerror(res));
    return -1;
  }

  curl_easy_getinfo(t->curl, CURLINFO_RESPONSE_CODE, &httprc);

  if (httprc >299 || httprc <= 199) {
    fprintf(stderr, "Endpoint %s returned HTTP %d\n",
            conf->endpoint, httprc);
    if (httprc == 403) {
      fprintf(stderr, "Are you sure your secret is correct?\n");
    }
    return -1;
  }

  return 0;
}


static void nuke_newlines(char *p)
{
  size_t i;
  size_t l = strlen(p);
  for (i = 0; i < l; i++) {
    if (p[i] == '\n') {
      p[i] = '\0';
    }
    if (p[i] == '\r') {
      p[i] = '\0';
    }
  }
}

static int conf_parse(ckl_conf_t *conf, FILE *fp)
{
  char buf[8096];
  char *p = NULL;
  while ((p = fgets(buf, sizeof(buf), fp)) != NULL) {
    /* comment lines */
    if (p[0] == '#') {
      continue;
    }

    while (isspace(p[0])) { p++;};

    if (strncmp("endpoint", p, 8) == 0) {
      p += 8;
      while (isspace(p[0])) { p++;};
      if (conf->endpoint) {
        free((char*)conf->endpoint);
      }
      nuke_newlines(p);
      conf->endpoint = strdup(p);
      continue;
    }

    if (strncmp("secret", p, 6) == 0) {
      p += 6;
      while (isspace(p[0])) { p++;};
      if (conf->secret) {
        free((char*)conf->secret);
      }
      nuke_newlines(p);
      conf->secret = strdup(p);
      continue;
    }
  }

  return 0;
}


static int conf_init(ckl_conf_t *conf)
{
  int rv;
  FILE *fp;

  /* TODO: respect prefix */
  fp = fopen("/etc/ckl.conf", "r");
  if (fp == NULL) {
    char buf[2048];
    const char *home = getenv("HOME");

    if (home == NULL) {
      error_out("HOME is not set");
      return -1;
    }

    snprintf(buf, sizeof(buf), "%s/.ckl", home);

    fp = fopen(buf, "r");
    
    if (fp == NULL) {
      fprintf(stderr, "Unable to read configuration file.\n");
      fprintf(stderr, "Please create '/etc/ckl.conf' or '~/.ckl' with the following:\n");
      fprintf(stderr, "  endpoint https://example.com/ckl\n");
      fprintf(stderr, "  secret ExampleSecret\n");
      error_out("No configuration file available.");
      return -1;
    }
  }

  rv = conf_parse(conf, fp);
  if (rv < 0) {
    error_out("parsing config file failed");
    return rv;
  }

  fclose(fp);

  if (!conf->endpoint || strlen(conf->endpoint) < 8 /* len(http://a) */) {
    error_out("Configuration file is missing endpoint");
    return -1;
  }

  if (!conf->secret || strlen(conf->secret) < 1) {
    error_out("Configuration file is missing secret");
    return -1;
  }

  return 0;
}

static void conf_free(ckl_conf_t *conf)
{
  free((char*)conf->endpoint);
  free((char*)conf->secret);
  free(conf);
}

static int msg_init(ckl_msg_t *msg)
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
      return -1;
    }

    msg->hostname = strdup(buf);
  }

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
  char uabuf[255];
  static const char buf[] = "Expect:";

  t->curl = curl_easy_init();

  snprintf(uabuf, sizeof(uabuf), "ckl/%d.%d.%d (Changelog Client)",
           CKL_VERSION_MAJOR, CKL_VERSION_MINOR, CKL_VERSION_PATCH);

  curl_easy_setopt(t->curl, CURLOPT_URL, conf->endpoint);
  curl_easy_setopt(t->curl, CURLOPT_USERAGENT, uabuf);

#ifdef CKL_DEBUG
  curl_easy_setopt(t->curl, CURLOPT_VERBOSE, 1);
#endif

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

static int editor_setup_file(char **path, FILE **fd)
{
  char buf[128];

  strncpy(buf, "/tmp/ckl.XXXXXX", sizeof(buf));

  int fx = mkstemp(buf);
  if (fx < 0) {
    perror("Failed to create tempfile");
    return -1;
  }

  *fd = fdopen(fx, "r+");

  *path = strdup(buf);

  return 0;
}

static int editor_fill_file(ckl_conf_t *conf, ckl_msg_t *m, FILE *fd)
{
  fprintf(fd, "\n");
  fprintf(fd, "# changelog entry:\n");
  fprintf(fd, "#    host:   %s\n", m->hostname);
  fprintf(fd, "#    user:   %s\n", m->username);
  fprintf(fd, "#    endp:   %s\n", conf->endpoint);
  fprintf(fd, "# (lines starting with # are ignored)");
  fflush(fd);
  return 0;
}

static int editor_edit(const char* editor, const char *path)
{
  int rv;
  char buf[2048];
  /* TODO: proper quoting */
  snprintf(buf, sizeof(buf), "%s '%s'", editor, path);

  rv = system(buf);
  if (rv < 0) {
    fprintf(stderr, "os.system failed for cmd '%s'\n", buf);
    perror("system(): ");
    return -1;
  }
  else if (rv != 0) {
    fprintf(stderr, "os.system('%s') returned %d\n", buf, rv);
    return -1;
  }
  return 0;
}

static int editor_read_file(ckl_msg_t *m, const char *path)
{
  FILE *fp = fopen(path, "r");

  char *out = strdup("");

  if (!fp) {
    error_out("Unable to read editted file?");
    return -1;
  }

  char buf[8096];
  char *p = NULL;

  while ((p = fgets(buf, sizeof(buf), fp)) != NULL) {
    /* comment lines */
    if (p[0] == '#') {
      continue;
    }

    nuke_newlines(p);

    char *t = calloc(1, strlen(out) + strlen(p) + 1);
    strncpy(t, out, strlen(out));
    strncpy(t+strlen(out), p, strlen(p));
    free(out);
    out = t;
  }

  m->msg = out;

  return 0;
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

  rv = conf_init(conf);
  if (rv < 0) {
    error_out("conf_init failed");
  }

  rv = msg_init(msg);
  if (rv < 0) {
    error_out("msg_init failed.");
  }

  if (usermsg == NULL) {
    rv = editor_find(&editor);
    if (rv < 0) {
      error_out("unable to find text editor. Set EDITOR or use -m");
    }
    char *path;
    FILE *fd;

    rv = editor_setup_file(&path, &fd);
    if (rv < 0) {
      error_out("Failed to setup tempfile for editting.");
    }
    
    rv = editor_fill_file(conf, msg, fd);
    if (rv < 0) {
      error_out("Failed to prefile file");
    }

    rv = editor_edit(editor, path);
    if (rv < 0) {
      error_out("editor broke?");
    }

    rv = editor_read_file(msg, path);
    if (rv < 0) {
      error_out("Failed to read editted file");
    }

    fclose(fd);
    unlink(path);
  }
  else {
    msg->msg = strdup(usermsg);
  }


  if (msg->msg == NULL) {
    error_out("no message specified");
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
