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

void ckl_error_out(const char *msg)
{
  fprintf(stderr, "ERROR: %s\n", msg);
  exit(EXIT_FAILURE);
}

void ckl_nuke_newlines(char *p)
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

int ckl_tmp_file(char **path, FILE **fd)
{
  char buf[128];
  
  strncpy(buf, "/tmp/ckl.XXXXXX", sizeof(buf));
  
  int fx = mkstemp(buf);
  if (fx < 0) {
    perror("Failed to create tempfile");
    return -1;
  }
  
  if (fd != NULL) {
    *fd = fdopen(fx, "r+");
  }
  
  *path = strdup(buf);
  
  return 0;
}

const char *ckl_hostname()
{
  char buf[HOST_NAME_MAX+1];
  buf[HOST_NAME_MAX] = '\0';
  int rv;
  
  rv = gethostname(&buf[0], HOST_NAME_MAX);
  if (rv < 0) {
    ckl_error_out("gethostname returned -1.  Is your hostname set?");
    return NULL;
  }
  
  return strdup(buf);
}
