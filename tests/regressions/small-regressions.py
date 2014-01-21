#!/usr/bin/env python

import os
import proc
import platform
import sys
from subprocess import CalledProcessError, PIPE, Popen, call


#
# main function receives
#   - argv[1] : list of executables from which the bitcode is to be extracted
#
def main():
  executables_name = sys.argv[1]
  executables = open(executables_name, 'r')

  # setting some variables
  llvm = os.getenv("GOLD_LLVM_BIN")
  instrumentorpath = os.getenv("INSTRUMENTOR_PATH")
  monitorpass = instrumentorpath + "/MonitorPass/MonitorPass.so"
  sourcepath = instrumentorpath + "/src"

  logfile = "log.out"
  log = open(logfile, "w") 

  for executable in executables:
      executable = executable.strip()
      print executable

      ##########################################
      # compiling file
      bitcodefile = executable + '.bc' 
      bitcode = open(bitcodefile, 'w')

      command = ['clang', '-c', '-emit-llvm', executable + '.c', '-o', bitcodefile]
      retval = call(command, stdin=None, stdout=None, stderr=None)

      # return -1 if running LLVM passes fails
      if retval <> 0:
        log.write("[FAILED INSTRUMENTATION]: i_" + executable + ".bc\n")
        continue 

      
      ##########################################
      # instrumented bitcode file
      ibitcodefile = 'i_' + executable + '.bc' 
      ibitcode = open(ibitcodefile, 'w')

      command = [llvm + '/opt', '-load', monitorpass, '--instrument', executable + '.bc', "-o", ibitcodefile]
      retval = call(command, stdin=None, stdout=None, stderr=None)

      # return -1 if running LLVM passes fails
      if retval <> 0:
        log.write("[FAILED INSTRUMENTATION]: i_" + executable + ".bc\n")
        continue 

      ###########################################
      # instrumented executable file
      iassemblyfile = 'i_' + executable + '.s' 
      iassembly = open(iassemblyfile, 'w')

      command = [llvm + '/llc', ibitcodefile]
      retval = call(command, stdin=None, stdout=None, stderr=None)

      # return -1 if running LLVM passes fails
      if retval <> 0:
        log.write("[FAILED ASSEMBLY]: " + executable + ".s\n")
        continue 

      
      ############################################
      # creating instrumented executable file
      iexecutablefile = executable + '.out' 
      iexecutable = open(iexecutablefile, 'w')

      # added -lgmp for expr and other core utility
      command = [llvm + '/clang', '-use-gold-plugin', iassemblyfile, '-L' + sourcepath, '-lmonitor', '-lpthread', '-lm', '-lrt', '-lgmp', '-o', iexecutablefile]
      retval = call(command, stdin=None, stdout=iexecutable, stderr=None)

      # return -1 if running LLVM passes fails
      if retval <> 0:
        log.write("[FAILED CREATING EXECUTABLE]: " + executable + ".out\n")

      iexecutable.close()
         

      ############################################
      # running executable file        
      command = ['./' + executable + '.out']
      retval = call(command, stdin=None, stdout=None, stderr=None)

      # return -1 if running LLVM passes fails
      if retval <> 0:
        log.write("[FAILED RUNNING EXECUTABLE]: " + iexecutablefile + "\n")




if __name__ == "__main__":
  main()