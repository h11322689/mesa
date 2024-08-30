   #!/usr/bin/env python3
   import os
   import socket
   import sys
   import select
   from select import EPOLLIN, EPOLLPRI, EPOLLERR
   import time
   from collections import namedtuple
   import argparse
      TIMEOUT = 1.0 # seconds
      VERSION_HEADER = bytearray('MesaOverlayControlVersion', 'utf-8')
   DEVICE_NAME_HEADER = bytearray('DeviceName', 'utf-8')
   MESA_VERSION_HEADER = bytearray('MesaVersion', 'utf-8')
      DEFAULT_SERVER_ADDRESS = "\0mesa_overlay"
      class Connection:
      def __init__(self, path):
      # Create a Unix Domain socket and connect
   sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
   try:
         except socket.error as msg:
                           # initialize poll interface and register socket
   epoll = select.epoll()
   epoll.register(sock, EPOLLIN | EPOLLPRI | EPOLLERR)
         def recv(self, timeout):
      '''
   timeout as float in seconds
   returns:
         - None on error or disconnection
            events = self.epoll.poll(timeout)
   for ev in events:
         (fd, event) = ev
                  # check for socket error
                                 # socket disconnected
                           def send(self, msg):
         class MsgParser:
      MSGBEGIN = bytes(':', 'utf-8')[0]
   MSGEND = bytes(';', 'utf-8')[0]
            def __init__(self, conn):
      self.cmdpos = 0
   self.parampos = 0
   self.bufferpos = 0
   self.reading_cmd = False
   self.reading_param = False
   self.buffer = None
   self.cmd = bytearray(4096)
                  def readCmd(self, ncmds, timeout=TIMEOUT):
      '''
   returns:
         - None on error or disconnection
                              while remaining > 0 and ncmds > 0:
                  if self.buffer == None:
                  # disconnected or error
                  for i in range(self.bufferpos, len(self.buffer)):
      c = self.buffer[i]
   self.bufferpos += 1
   if c == self.MSGBEGIN:
      self.cmdpos = 0
   self.parampos = 0
   self.reading_cmd = True
      elif c == self.MSGEND:
      if not self.reading_cmd:
                        cmd = self.cmd[0:self.cmdpos]
                        parsed.append((cmd, param))
   ncmds -= 1
   if ncmds == 0:
      elif c == self.MSGSEP:
      if self.reading_cmd:
      else:
      if self.reading_param:
         self.param[self.parampos] = c
                  # if we read the entire buffer and didn't finish the command,
                  # check if we have time for another iteration
            # timeout
      def control(args):
      if args.socket:
         else:
            conn = Connection(address)
            version = None
   name = None
                     for m in msgs:
      cmd, param = m
   if cmd == VERSION_HEADER:
         elif cmd == DEVICE_NAME_HEADER:
         elif cmd == MESA_VERSION_HEADER:
         if version != 1 or name == None or mesa_version == None:
      print('ERROR: invalid protocol')
            if args.info:
      info = "Protocol Version: {}\n"
   info += "Device Name: {}\n"
   info += "Mesa Version: {}"
         if args.cmd == 'start-capture':
         elif args.cmd == 'stop-capture':
         if __name__ == '__main__':
      parser = argparse.ArgumentParser(description='MESA_overlay control client')
   parser.add_argument('--info', action='store_true', help='Print info from socket')
            commands = parser.add_subparsers(help='commands to run', dest='cmd')
   commands.add_parser('start-capture')
                     control(args)
