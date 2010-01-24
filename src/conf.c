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

static char *next_chunk(char **x_p)
{
  char *p = *x_p;

  while (isspace(p[0])) { p++;};

  ckl_nuke_newlines(p);

  *x_p = p;
  return strdup(p);
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
    
    if (strncmp("ckl_endpoint", p, 12) == 0) {
      p += 12;
      if (conf->endpoint) {
        free((char*)conf->endpoint);
      }
      conf->endpoint = next_chunk(&p);
      continue;
    }
    
    /* Deprecated: 'secret' based authentication */
    if (strncmp("secret", p, 6) == 0) {
      p += 6;
      if (conf->secret) {
        free((char*)conf->secret);
      }
      conf->secret = next_chunk(&p);
      continue;
    }

    if (strncmp("oauth_secret", p, 12) == 0) {
      p += 12;
      if (conf->oauth_secret) {
        free((char*)conf->oauth_secret);
      }
      conf->oauth_secret = next_chunk(&p);
      continue;
    }

    if (strncmp("oauth_key", p, 9) == 0) {
      p += 9;
      if (conf->oauth_key) {
        free((char*)conf->oauth_key);
      }
      conf->oauth_key = next_chunk(&p);
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
  fp = fopen("/etc/cloudkick.conf", "r");
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
      fprintf(stderr, "Please run cloudkick-config, or visit https://support.cloudkick.com/Ckl/Installation\n");
      ckl_error_out("No configuration file available.");
      return -1;
    }
  }
  
  rv = conf_parse(conf, fp);
  if (rv < 0) {
    ckl_error_out("parsing config file failed. \nFor help go to https://support.cloudkick.com/Ckl/Installation");
    return rv;
  }
  
  fclose(fp);
  
  if (!conf->endpoint) {
    conf->endpoint = strdup("https://api.cloudkick.com/changelog/1.0");
  }

  if (strlen(conf->endpoint) < 8 /* len(http://a) */) {
    ckl_error_out("Configuration file has invalid ckl_endpoint. \nFor help go to https://support.cloudkick.com/Ckl/Installation");
    return -1;
  }
  
  if (!conf->oauth_key && !conf->oauth_secret) {
    if (!conf->secret || strlen(conf->secret) < 1) {
      ckl_error_out("Configuration file is missing secret, oauth_key and oauth_secret. \nFor help go to https://support.cloudkick.com/Ckl/Installation\n");
      return -1;
    }
  }
  
  return 0;
}

void ckl_conf_free(ckl_conf_t *conf)
{
  free((char*)conf->endpoint);
  free((char*)conf->secret);
  free(conf);
}

