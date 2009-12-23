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
      ckl_nuke_newlines(p);
      conf->endpoint = strdup(p);
      continue;
    }
    
    if (strncmp("secret", p, 6) == 0) {
      p += 6;
      while (isspace(p[0])) { p++;};
      if (conf->secret) {
        free((char*)conf->secret);
      }
      ckl_nuke_newlines(p);
      conf->secret = strdup(p);
      continue;
    }
  }
  
  return 0;
}

int ckl_conf_init(ckl_conf_t *conf)
{
  int rv;
  FILE *fp;
  
  /* TODO: respect prefix */
  fp = fopen("/etc/ckl.conf", "r");
  if (fp == NULL) {
    char buf[2048];
    const char *home = getenv("HOME");
    
    if (home == NULL) {
      ckl_error_out("HOME is not set");
      return -1;
    }
    
    snprintf(buf, sizeof(buf), "%s/.ckl", home);
    
    fp = fopen(buf, "r");
    
    if (fp == NULL) {
      fprintf(stderr, "Unable to read configuration file.\n");
      fprintf(stderr, "Please create '/etc/ckl.conf' or '~/.ckl' with the following:\n");
      fprintf(stderr, "  endpoint https://example.com/ckl\n");
      fprintf(stderr, "  secret ExampleSecret\n");
      ckl_error_out("No configuration file available.");
      return -1;
    }
  }
  
  rv = conf_parse(conf, fp);
  if (rv < 0) {
    ckl_error_out("parsing config file failed");
    return rv;
  }
  
  fclose(fp);
  
  if (!conf->endpoint || strlen(conf->endpoint) < 8 /* len(http://a) */) {
    ckl_error_out("Configuration file is missing endpoint");
    return -1;
  }
  
  if (!conf->secret || strlen(conf->secret) < 1) {
    ckl_error_out("Configuration file is missing secret");
    return -1;
  }
  
  return 0;
}

void ckl_conf_free(ckl_conf_t *conf)
{
  free((char*)conf->endpoint);
  free((char*)conf->secret);
  free(conf);
}

