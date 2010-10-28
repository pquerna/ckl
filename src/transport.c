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

#define _GNU_SOURCE
#include "ckl.h"
#include "ckl_version.h"
#include "extern/liboauth/src/oauth.h"

static void base_post_data(ckl_transport_t *t,
                           ckl_conf_t *conf,
                           const char *hostname)
{
  curl_formadd(&t->formpost,
               &t->lastptr,
               CURLFORM_COPYNAME, "hostname",
               CURLFORM_COPYCONTENTS, hostname,
               CURLFORM_END);

  if (conf->secret) {
    curl_formadd(&t->formpost,
                 &t->lastptr,
                 CURLFORM_COPYNAME, "secret",
                 CURLFORM_COPYCONTENTS, conf->secret,
                 CURLFORM_END);
  }
}

static int msg_to_post_data(ckl_transport_t *t,
                            ckl_conf_t *conf,
                            ckl_msg_t* m)
{
  char buf[128];

  snprintf(buf, sizeof(buf), "%d", (int)m->ts);

  base_post_data(t, conf, m->hostname);

  curl_formadd(&t->formpost,
               &t->lastptr,
               CURLFORM_COPYNAME, "username",
               CURLFORM_COPYCONTENTS, m->username,
               CURLFORM_END);

  curl_formadd(&t->formpost,
               &t->lastptr,
               CURLFORM_COPYNAME, "msg",
               CURLFORM_COPYCONTENTS, m->msg,
               CURLFORM_END);
  
  curl_formadd(&t->formpost,
               &t->lastptr,
               CURLFORM_COPYNAME, "ts",
               CURLFORM_COPYCONTENTS, buf,
               CURLFORM_END);

  return 0;
}

static int list_to_post_data(ckl_transport_t *t,
                            ckl_conf_t *conf,
                            int count)
{
  char buf[128];
  const char *hostname = ckl_hostname();
  snprintf(buf, sizeof(buf), "%d", count);

  base_post_data(t, conf, hostname);

  free((char*)hostname);
  curl_formadd(&t->formpost,
               &t->lastptr,
               CURLFORM_COPYNAME, "count",
               CURLFORM_COPYCONTENTS, buf,
               CURLFORM_END);

  return 0;
}

static int motd_to_post_data(ckl_transport_t *t,
                            ckl_conf_t *conf,
                            char* node_id)
{
  char buf[128];
  snprintf(buf, sizeof(buf), "%s", node_id);
  const char *hostname = ckl_hostname();

  base_post_data(t, conf, hostname);

  free((char*)hostname);
  curl_formadd(&t->formpost,
               &t->lastptr,
               CURLFORM_COPYNAME, "node_id",
               CURLFORM_COPYCONTENTS, buf,
               CURLFORM_END);

  return 0;
}

static int detail_to_post_data(ckl_transport_t *t,
                               ckl_conf_t *conf,
                               const char *slug)
{
  const char *hostname = ckl_hostname();

  base_post_data(t, conf, hostname);

  free((char*)hostname);
  curl_formadd(&t->formpost,
               &t->lastptr,
               CURLFORM_COPYNAME, "id",
               CURLFORM_COPYCONTENTS, slug,
               CURLFORM_END);

  return 0;
}

static char *strappend(const char *a, const char *b)
{
  size_t la = strlen(a);
  size_t lb = strlen(b);
  char *c = malloc(la + lb + 1);
  memcpy(c, a, la);
  memcpy(c+la, b, lb);
  c[la+lb] = '\0';
  return c;
}

static int ckl_transport_run(ckl_transport_t *t, ckl_conf_t *conf, ckl_msg_t* m)
{
  long httprc = -1;
  CURLcode res;
  char *url = strdup(conf->endpoint);
  char *postarg = NULL;

  if (t->append_url) {
    free(url);
    url = strappend(conf->endpoint, t->append_url);
  }

  if (conf->oauth_key && conf->oauth_secret) {
    int i;
    int  argc;
    char **argv = NULL;
    char *url2;
    struct curl_httppost *tmp;
    argc = oauth_split_post_paramters(url, &argv, 0);

    for (tmp = t->formpost; tmp != NULL; tmp = tmp->next) {
      char *p = NULL;
      if (asprintf(&p, "%s=%s", tmp->name, tmp->contents) < 0) {
        fprintf(stderr, "Broken asprintf: %s = %s\n",
                 tmp->name, tmp->contents);
        return -1;
      }
      oauth_add_param_to_array(&argc, &argv, p);
      free(p);
    }

    url2 = oauth_sign_array2(&argc, &argv, NULL, 
                             OA_HMAC, "POST",
                             conf->oauth_key, conf->oauth_secret,
                             "", "");

    //fprintf(stderr, "url  = %s\n", url2);
    free(url2);
    curl_formfree(t->formpost);
    t->formpost = NULL;
    t->lastptr = NULL;
    for (i = 1; i < argc; i++) {
      char *s = strdup(argv[i]);
      char *p = strchr(s, '=');
      if (p == NULL) {
        fprintf(stderr, "Broken argv: %s = %s\n",
                tmp->name, tmp->contents);
        return -1;
      }

      *p = '\0';

      p++;
      
      //fprintf(stderr, "argv[%d]: %s = %s\n", i, s, p);

      curl_formadd(&t->formpost,
                   &t->lastptr,
                   CURLFORM_COPYNAME, s,
                   CURLFORM_COPYCONTENTS, p,
                   CURLFORM_END);
    }

    oauth_free_array(&argc, &argv);
  }

  if (m && m->script_log != NULL) {
    curl_formadd(&t->formpost,
                 &t->lastptr,
                 CURLFORM_COPYNAME, "scriptlog",
                 CURLFORM_FILE, m->script_log,
                 CURLFORM_FILENAME, "script.log",
                 CURLFORM_CONTENTTYPE, "text/plain", CURLFORM_END);
  }
  
  
  curl_easy_setopt(t->curl, CURLOPT_HTTPPOST, t->formpost);

  curl_easy_setopt(t->curl, CURLOPT_URL, url);

  res = curl_easy_perform(t->curl);

  if (res != 0) {
    fprintf(stderr, "Failed talking to endpoint %s: (%d) %s\n\n",
            conf->endpoint, res, curl_easy_strerror(res));
    return -1;
  }

  curl_easy_getinfo(t->curl, CURLINFO_RESPONSE_CODE, &httprc);

  if (httprc >299 || httprc <= 199) {
    fprintf(stderr, "Endpoint %s returned HTTP %d\n",
            conf->endpoint, (int)httprc);
    if (httprc == 403) {
      fprintf(stderr, "Are you sure your secret is correct?\n");
    }
    return -1;
  }

  free(url);
  return 0;
}

int ckl_transport_msg_send(ckl_transport_t *t,
                           ckl_conf_t *conf,
                           ckl_msg_t* m)
{
  int rv = msg_to_post_data(t, conf, m);

  if (rv < 0) {
    return rv;
  }

  return ckl_transport_run(t, conf, m);
}


int ckl_transport_list(ckl_transport_t *t,
                       ckl_conf_t *conf,
                       int count)
{
  int rv = list_to_post_data(t, conf, count);

  if (rv < 0) {
    return rv;
  }

  t->append_url = "/list";

  return ckl_transport_run(t, conf, NULL);
}

int ckl_transport_motd(ckl_transport_t *t,
                       ckl_conf_t *conf,
                       char* node_id)
{
  int rv = motd_to_post_data(t, conf, node_id);

  if (rv < 0) {
    return rv;
  }

  t->append_url = "/motd";

  return ckl_transport_run(t, conf, NULL);
}

int ckl_transport_detail(ckl_transport_t *t,
                         ckl_conf_t *conf,
                         const char *slug)
{
  int rv = detail_to_post_data(t, conf, slug);

  if (rv < 0) {
    return rv;
  }

  t->append_url = "/detail";

  return ckl_transport_run(t, conf, NULL);
}

int ckl_transport_init(ckl_transport_t *t, ckl_conf_t *conf)
{
  char uabuf[255];
  static const char buf[] = "Expect:";
  
  t->curl = curl_easy_init();
  
  snprintf(uabuf, sizeof(uabuf), "ckl/%d.%d.%d (Changelog Client)",
           CKL_VERSION_MAJOR, CKL_VERSION_MINOR, CKL_VERSION_PATCH);
  
  curl_easy_setopt(t->curl, CURLOPT_USERAGENT, uabuf);
  t->append_url = "/";

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

void ckl_transport_free(ckl_transport_t *t)
{
  curl_easy_cleanup(t->curl);
  curl_formfree(t->formpost);
  curl_slist_free_all(t->headerlist);
  free(t);
}


