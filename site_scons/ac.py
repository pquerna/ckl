import os
import hashlib


def CheckCurl(context, args):
  prog = context.env['CURL']
  context.Message("Checking %s %s ...." % (prog, args))
  m = hashlib.md5()
  m.update(args)
  
  output = context.sconf.confdir.File(os.path.basename(prog)+ '-'+ m.hexdigest() +'.out') 
  node = context.sconf.env.Command(output, prog, [ [ prog, args, ">", "${TARGET}"] ]) 
  ok = context.sconf.BuildNodes(node) 
  if ok: 
    outputStr = output.get_contents().strip()
    context.Result(" "+ outputStr)
    return (1, outputStr)
  else:
    context.Result("error running curl-config")
    return (0, "")

def CheckCurlPrefix(context):
  return CheckCurl(context, '--prefix')

def CheckCurlLibs(context):
  return CheckCurl(context, '--libs')


def CheckDpkgArch(context):
  args = "-qDEB_BUILD_ARCH"
  prog = context.env.WhereIs("dpkg-architecture")
  if not prog:
    context.Message("Error: `dpkg-architecture` not found. Install dpkg-dev?")
    return (0, "")
  context.Message("Checking %s %s ...." % (prog, args))
  output = context.sconf.confdir.File(os.path.basename(prog)+'.out') 
  node = context.sconf.env.Command(output, prog, [ [ prog, args, ">", "${TARGET}"] ]) 
  ok = context.sconf.BuildNodes(node) 
  if ok: 
    outputStr = output.get_contents().strip()
    context.Result(" "+ outputStr)
    return (1, outputStr)
  else:
    context.Result("error running dpkg-architecture")
    return (0, "")

