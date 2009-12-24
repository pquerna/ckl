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

#ifndef _ckl_h_
#define _ckl_h_

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
  int script_mode;
  int quiet;
  const char *endpoint;
  const char *secret;
} ckl_conf_t;

typedef struct ckl_msg_t {
  time_t ts;
  const char *username;
  const char *hostname;
  const char *msg;
  const char *script_log;
} ckl_msg_t;

typedef struct ckl_script_t {
  const char *shell;
  FILE *fd;
  char *path;
} ckl_script_t;

/* util functions */
void ckl_error_out(const char *msg);
void ckl_nuke_newlines(char *p);
int ckl_tmp_file(char **path, FILE **fd);
const char *ckl_hostname();

/* transport fucntions */
int ckl_transport_init(ckl_transport_t *t, ckl_conf_t *conf);
void ckl_transport_free(ckl_transport_t *t);
int ckl_transport_msg_send(ckl_transport_t *t,
                       ckl_conf_t *conf,
                       ckl_msg_t* m);
int ckl_transport_list(ckl_transport_t *t,
                       ckl_conf_t *conf,
                       int count);

/* script functions */
int ckl_script_init(ckl_script_t *s, ckl_conf_t *conf);
int ckl_script_record(ckl_script_t *s, ckl_msg_t *msg);
void ckl_script_free(ckl_script_t *s);

/* configuration functions */
int ckl_conf_init(ckl_conf_t *conf);
void ckl_conf_free(ckl_conf_t *conf);

/* msg functions */
int ckl_msg_init(ckl_msg_t *msg);
void ckl_msg_free(ckl_msg_t *m);

/* editor functions */
int ckl_editor_find(const char **output);
int ckl_editor_setup_file(char **path, FILE **fd);
int ckl_editor_fill_file(ckl_conf_t *conf, ckl_msg_t *m, FILE *fd);
int ckl_editor_edit(const char* editor, const char *path);
int ckl_editor_read_file(ckl_msg_t *m, const char *path);
#endif

