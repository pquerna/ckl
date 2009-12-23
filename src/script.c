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

int ckl_script_init(ckl_script_t *s, ckl_conf_t *conf)
{
  int rv;
  FILE *fd;

  rv = ckl_tmp_file(&s->path, &fd);
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

int ckl_script_record(ckl_script_t *s, ckl_msg_t *msg)
{
  return 0;
}

void ckl_script_free(ckl_script_t *s)
{
  if (s->path) {
    unlink(s->path);
    free(s->path);
  }
  if (s->shell) {
    free((char*)s->shell);
  }
  free(s);
}

