echo off
rmdir /s /q build
CALL conda remove -y --name py%1 --all
CALL conda create -y -n py%1 python=%1 pip
CALL conda activate py%1
for /f %%i in ('python -c "import struct;print( 8 * struct.calcsize('P'))"') do set PYARCH=%%i
CALL "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvars%PYARCH%.bat"
pip install virtualenvwrapper-win
CALL rmvirtualenv venv
CALL mkvirtualenv -r requirements-win.txt venv
md build
cd build
cmake ..
CALL msbuild libsettings.sln /p:Configuration="Release" /p:Platform="Win32"
cd ..
python setup.py bdist_wheel
pip install --no-index --find-links=./dist libsettings
pip install sbp
for /f %%i in ('python -c "from libsettings import Settings"') do set RET=%%i
CALL deactivate
CALL rmvirtualenv venv
CALL conda deactivate
CALL conda remove -y --name py%1 --all
EXIT /B %RET%
