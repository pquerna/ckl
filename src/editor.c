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

int ckl_editor_setup_file(char **path, FILE **fd)
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

int ckl_editor_fill_file(ckl_conf_t *conf, ckl_msg_t *m, FILE *fd)
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

int ckl_editor_edit(const char* editor, const char *path)
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

int ckl_editor_read_file(ckl_msg_t *m, const char *path)
{
  FILE *fp = fopen(path, "r");
  
  char *out = strdup("");
  
  if (!fp) {
    ckl_error_out("Unable to read editted file?");
    return -1;
  }
  
  char buf[8096];
  char *p = NULL;
  
  while ((p = fgets(buf, sizeof(buf), fp)) != NULL) {
    /* comment lines */
    if (p[0] == '#') {
      continue;
    }
    
    ckl_nuke_newlines(p);
    
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
int ckl_editor_find(const char **output)
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

