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

static void show_version()
{
  fprintf(stdout, "ck_motd - %d.%d.%d\n", CKL_VERSION_MAJOR, CKL_VERSION_MINOR, CKL_VERSION_PATCH);
  exit(EXIT_SUCCESS);
}

static void show_help()
{
  fprintf(stdout, "ck_motd - Cloudkick Changelog tool\n");
  fprintf(stdout, "  Usage:  \n");
  fprintf(stdout, "    ckl [-h|-V]\n");
  fprintf(stdout, "    ckl [-s] [-m message]\n");
  fprintf(stdout, "    ckl [-l]\n");
  fprintf(stdout, "    ckl [-d number]\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "     -h          Show Help message\n");
  fprintf(stdout, "     -V          Show Version number\n");
  fprintf(stdout, "     -l          List recent actions on this host\n");
  fprintf(stdout, "     -d (n)      Show details about session N, listed from -l\n");
  fprintf(stdout, "     -m (msg)    Set the log message, if none is set, an editor will be invoked.\n");
  fprintf(stdout, "     -s          Run in script recording mode.\n");
  fprintf(stdout, "See `man ckl` for more details\n");
  exit(EXIT_SUCCESS);
}


static int do_motd(ckl_conf_t *conf, const char* node_id)
{
  int rv;
  ckl_transport_t *transport = calloc(1, sizeof(ckl_transport_t));

  rv = ckl_transport_init(transport, conf);
  if (rv < 0) {
    ckl_error_out("transport_init failed.");
    return rv;
  }

  rv = ckl_transport_motd(transport, conf, node_id);
  if (rv < 0) {
    ckl_error_out("ckl_transport_list failed.");
    return rv;
  }

  ckl_transport_free(transport);

  return 0;
}


int main(int argc, char *const *argv)
{
  int c;
  int rv;
  int count = 10;
  ckl_conf_t *conf = calloc(1, sizeof(ckl_conf_t));

  curl_global_init(CURL_GLOBAL_ALL);

  while ((c = getopt(argc, argv, "hVslm:d:")) != -1) {
    switch (c) {
      case 'V':
        show_version();
        break;
      case 'h':
        show_help();
        break;
      case 's':
        conf->script_mode = 1;
        break;
      case '?':
        ckl_error_out("See -h for correct options");
        break;
    }
  }

  rv = ckl_conf_init(conf);

  if (rv < 0) {
    ckl_error_out("conf_init failed");
  }

  do_motd(conf, "n267d7a22c");

  ckl_conf_free(conf);

  curl_global_cleanup();

  return rv;
}
