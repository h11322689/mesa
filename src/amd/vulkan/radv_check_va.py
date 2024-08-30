   #!/usr/bin/python3
      import re
   import sys
      def main():
      if len(sys.argv) != 3:
      print("Missing arguments: ./radv_check_va.py <bo_history> <64-bit VA>")
         bo_history = str(sys.argv[1])
            va_found = False
   with open(bo_history) as f:
      for line in f:
         p = re.compile('timestamp=(.*), VA=(.*)-(.*), destroyed=(.*), is_virtual=(.*)')
   m = p.match(line)
                                 # Check if the given VA was ever valid and print info.
   if va >= va_start and va < va_end:
      if not va_found:
         if __name__ == '__main__':
      main()
