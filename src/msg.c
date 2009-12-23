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

int ckl_msg_init(ckl_msg_t *msg)
{
  {
    const char *user = getenv("SUDO_USER");
    
    if (user == NULL) {
      user = getlogin();
    }
    
    if (user == NULL) {
      ckl_error_out("Unknown user: getlogin(2) and SUDO_USER both returned NULL.");
    }
    
    msg->username = strdup(user);
  }
  
  {
    char buf[HOST_NAME_MAX+1];
    buf[HOST_NAME_MAX] = '\0';
    int rv;
    
    rv = gethostname(&buf[0], HOST_NAME_MAX);
    if (rv < 0) {
      ckl_error_out("gethostname returned -1.  Is your hostname set?");
      return -1;
    }
    
    msg->hostname = strdup(buf);
  }
  
  msg->ts = time(NULL);
  
  return 0;
}

void ckl_msg_free(ckl_msg_t *m)
{
  free((char*)m->username);
  free((char*)m->hostname);
  free((char*)m->msg);
  free(m);
}

