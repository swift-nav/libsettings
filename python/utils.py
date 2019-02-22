import win32process

def get_python_dll_path(version):
  for process in win32process.EnumProcessModules(-1):
    name = win32process.GetModuleFileNameEx(-1, process)
    if "python{}.dll".format(version) in name:
      print name
      break
