rm -rf build
mkdir build
cd build
cmake ../
make
cd ..
python setup.py sdist
