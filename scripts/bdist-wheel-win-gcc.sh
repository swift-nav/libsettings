if [ -z "$1" ]
  then
    exit 1
fi
# For MinGW make to work correctly sh.exe must NOT be in your path.
mv "/c/Program Files/Git/usr/bin/sh.exe" "/c/Program Files/Git/usr/bin/hidden_sh.exe"
./scripts/bdist-wheel-win-gcc.bat $1 || exit 1
mv "/c/Program Files/Git/usr/bin/hidden_sh.exe" "/c/Program Files/Git/usr/bin/sh.exe"
