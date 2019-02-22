if [ -z "$1" ]
  then
    exit 1
fi
./scripts/bdist-wheel-win-msvc.bat $1
conda create -y -n py$1 python=$1 pip
source activate py$1
