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
  fprintf(stdout, "ckl - %d.%d.%d\n", CKL_VERSION_MAJOR, CKL_VERSION_MINOR, CKL_VERSION_PATCH);
  exit(EXIT_SUCCESS);
}

static void show_help()
{
  fprintf(stdout, "ckl - Cloudkick Changelog tool\n");
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
  fprintf(stdout, "     -s          Run in script recrding mode.\n");
  fprintf(stdout, "See `man ckl` for more details\n");
  exit(EXIT_SUCCESS);
}

static int do_send_msg(ckl_conf_t *conf, const char *usermsg)
{
  int rv;
  const char *editor;
  ckl_msg_t *msg = calloc(1, sizeof(ckl_msg_t));
  ckl_transport_t *transport = calloc(1, sizeof(ckl_transport_t));
  ckl_script_t *script = calloc(1, sizeof(ckl_script_t));

  rv = ckl_msg_init(msg);
  if (rv < 0) {
    ckl_error_out("msg_init failed.");
    return rv;
  }

  if (usermsg == NULL) {
    rv = ckl_editor_find(&editor);
    if (rv < 0) {
      fprintf(stderr, "Warning: no EDITOR found, using `vi`\n");
      editor = "vi";
    }

    char *path;
    FILE *fd;

    rv = ckl_tmp_file(&path, &fd);
    if (rv < 0) {
      ckl_error_out("Failed to setup tempfile for editting.");
      return rv;
    }

    rv = ckl_editor_fill_file(conf, msg, fd);
    if (rv < 0) {
      ckl_error_out("Failed to prefile file");
      return rv;
    }

    rv = ckl_editor_edit(editor, path);
    if (rv < 0) {
      ckl_error_out("editor broke?");
      return rv;
    }

    rv = ckl_editor_read_file(msg, path);
    if (rv < 0) {
      ckl_error_out("Failed to read editted file");
      return rv;
    }

    fclose(fd);
    unlink(path);
  }
  else {
    msg->msg = strdup(usermsg);
  }

  if (msg->msg == NULL) {
    ckl_error_out("no message specified");
  }

  if (conf->script_mode) {
    rv = ckl_script_init(script, conf);
    if (rv < 0) {
      ckl_error_out("script_init failed.");
      return rv;
    }

    rv = ckl_script_record(script, msg);
    if (rv < 0) {
      ckl_error_out("script_record failed.");
      return rv;
    }
  }

  rv = ckl_transport_init(transport, conf);
  if (rv < 0) {
    ckl_error_out("transport_init failed.");
    return rv;
  }

  rv = ckl_transport_msg_send(transport, conf, msg);
  if (rv < 0) {
    ckl_error_out("msg_send failed.");
    return rv;
  }

  ckl_transport_free(transport);
  ckl_msg_free(msg);
  ckl_script_free(script);

  return 0;
}

static int do_list(ckl_conf_t *conf)
{
  int rv;
  ckl_transport_t *transport = calloc(1, sizeof(ckl_transport_t));

  rv = ckl_transport_init(transport, conf);
  if (rv < 0) {
    ckl_error_out("transport_init failed.");
    return rv;
  }

  rv = ckl_transport_list(transport, conf, 5);
  if (rv < 0) {
    ckl_error_out("ckl_transport_list failed.");
    return rv;
  }

  ckl_transport_free(transport);

  return 0;
}

static int do_detail(ckl_conf_t *conf, const char *slug)
{
  int rv;
  ckl_transport_t *transport = calloc(1, sizeof(ckl_transport_t));

  rv = ckl_transport_init(transport, conf);
  if (rv < 0) {
    ckl_error_out("transport_init failed.");
    return rv;
  }

  rv = ckl_transport_detail(transport, conf, slug);
  if (rv < 0) {
    ckl_error_out("ckl_transport_detail failed.");
    return rv;
  }

  ckl_transport_free(transport);

  return 0;
}

enum {
  MODE_SEND_MSG,
  MODE_LIST,
  MODE_DETAIL
};

int main(int argc, char *const *argv)
{
  int mode = MODE_SEND_MSG;
  int c;
  int rv;
  const char *detail = NULL;
  const char *usermsg = NULL;
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
      case 'l':
        mode = MODE_LIST;
        break;
      case 'd':
        mode = MODE_DETAIL;
        detail = optarg;
        break;
      case 'm':
        usermsg = optarg;
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

  switch (mode) {
    case MODE_SEND_MSG:
      rv = do_send_msg(conf, usermsg);
      break;
    case MODE_LIST:
      rv = do_list(conf);
      break;
    case MODE_DETAIL:
      rv = do_detail(conf, detail);
      break;
  }

  ckl_conf_free(conf);

  curl_global_cleanup();

  return rv;
}
