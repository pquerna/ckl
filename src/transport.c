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
#include "ckl_version.h"

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

  if (m->script_log != NULL) {
    curl_formadd(&t->formpost,
                 &t->lastptr,
                 CURLFORM_COPYNAME, "scriptlog",
                 CURLFORM_FILE, m->script_log,
                 CURLFORM_FILENAME, "script.log",
                 CURLFORM_CONTENTTYPE, "text/plain", CURLFORM_END);
  }
  
  curl_easy_setopt(t->curl, CURLOPT_HTTPPOST, t->formpost);
  
  return 0;
}


int ckl_transport_msg_send(ckl_transport_t *t,
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

int ckl_transport_init(ckl_transport_t *t, ckl_conf_t *conf)
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

void ckl_transport_free(ckl_transport_t *t)
{
  curl_easy_cleanup(t->curl);
  curl_formfree(t->formpost);
  curl_slist_free_all(t->headerlist);
  free(t);
}


