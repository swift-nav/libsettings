echo off
if NOT %1%==2.7 (
    echo This script is for Python version 2.7
    EXIT /B 1
)
rmdir /s /q build
CALL conda remove -y --name py%1 --all
CALL conda create -y -n py%1 python=%1 pip
CALL conda activate py%1
pip install virtualenvwrapper-win
CALL rmvirtualenv venv
CALL mkvirtualenv -r requirements-win-cp27-cp34.txt venv
md build
cd build
cmake  -D SKIP_UNIT_TESTS=true .. -G "MinGW Makefiles"
mingw32-make.exe
cd ..
python setup.py bdist_wheel
rmdir /s /q build
CALL deactivate
CALL rmvirtualenv venv
CALL conda deactivate
CALL conda remove -y --name py%1 --all
