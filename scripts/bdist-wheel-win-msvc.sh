if [ -z "$1" ]
  then
    exit 1
fi
./scripts/bdist-wheel-win-msvc.bat $1 || exit 1
