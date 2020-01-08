if [ -z "$1" ]
  then
    choco install miniconda3
elif [ "$1" == "--x86" ]
  then
    choco install miniconda3 --x86 --checksum 7C30778941D2BBA03531BA269A78A108B01FA366530290376E7C3B467F3C66BA
else
  exit 1
fi
export PATH="/c/Tools/miniconda3:/c/Tools/miniconda3/Library/mingw-w64/bin:/c/Tools/miniconda3/Library/usr/bin:/c/Tools/miniconda3/Library/bin:/c/Tools/miniconda3/Scripts:/c/Tools/miniconda3/bin:/c/Tools/miniconda3/condabin:$PATH"
conda info -s
conda update --yes conda
