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

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>


typedef struct ckl_conf_t {
  const char *endpoint;
  const char *secret;
} ckl_conf_t;

typedef struct ckl_msg_t {
  time_t ts;
  const char *username;
  const char *hostname;
  const char *msg;
} ckl_msg_t;


static int msg_to_post_data(CURL *curl, ckl_conf_t *conf, ckl_msg_t* m)
{
  struct curl_httppost *formpost = NULL;
  struct curl_httppost *lastptr = NULL;

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "username",
               CURLFORM_COPYCONTENTS, m->username,
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "hostname",
               CURLFORM_COPYCONTENTS, m->hostname,
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "msg",
               CURLFORM_COPYCONTENTS, m->msg,
               CURLFORM_END);

  curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
 
  return 0;
}

int main(int argc, const char *argv[])
{
  curl_global_init(CURL_GLOBAL_ALL);

  return 0;
}