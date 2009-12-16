# Licensed to Cloudkick, Inc under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# libcloud.org licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


EnsureSConsVersion(1, 1, 0)

import os
import re
from os.path import join as pjoin
from site_scons import ac

opts = Variables('build.py')

opts.Add(PathVariable('CURL', 'Path to curl-config', WhereIs('curl-config')))

env = Environment(options=opts,
                  ENV = os.environ.copy())

#TODO: convert this to a configure builder, so it gets cached
def read_version(prefix, path):
  version_re = re.compile("(.*)%s_VERSION_(?P<id>MAJOR|MINOR|PATCH)(\s+)(?P<num>\d)(.*)" % prefix)
  versions = {}
  fp = open(path, 'rb')
  for line in fp.readlines():
    m = version_re.match(line)
    if m:
      versions[m.group('id')] = int(m.group('num'))
  fp.close()
  return (versions['MAJOR'], versions['MINOR'], versions['PATCH'])

env['version_major'], env['version_minor'], env['version_patch'] = read_version('CKL', 'src/ckl_version.h')
env['version_string'] = "%d.%d.%d"  % (env['version_major'], env['version_minor'], env['version_patch'])

conf = Configure(env, custom_tests = {'CheckCurlPrefix': ac.CheckCurlPrefix, 'CheckCurlLibs': ac.CheckCurlLibs})

cc = conf.env.WhereIs('/Developer/usr/bin/clang')
if os.environ.has_key('CC'):
  cc = os.environ['CC']

if cc:
  conf.env['CC'] = cc


if not conf.CheckFunc('floor'):
  conf.env.AppendUnique(LIBS=['m'])

cprefix = conf.CheckCurlPrefix()
if not cprefix[0]:
  Exit("Error: Unable to detect curl prefix")

clibs = conf.CheckCurlLibs()
if not clibs[0]:
  Exit("Error: Unable to detect curl libs")

conf.env.AppendUnique(CPPPATH = [pjoin(cprefix[1], "include")])

# TOOD: this is less than optimal, since curl-config polutes this quite badly :(
d = conf.env.ParseFlags(clibs[1])
conf.env.MergeFlags(d)
conf.env.AppendUnique(CPPFLAGS = ["-Wall"])
# this is needed on solaris because of its dumb library path issues
conf.env.AppendUnique(RPATH = conf.env.get('LIBPATH'))
env = conf.Finish()

Export("env")

ckl = SConscript("src/SConscript")

targets = [ckl]

env.Default(targets)
