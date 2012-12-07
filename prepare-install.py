# -*- coding: utf-8 -*-
'''
Подготовка файлов перед выпуском новой версии
 
@file prepare-install.py
@author aleksey.hohryakov@gmail.com
'''

import os
import shutil

quiterssSourceAbsPath = "e:\\Work\\_Useful\\QtProjects\\QuiteRSS"
quiterssReleaseAbsPath = "e:\\Work\\_Useful\\QtProjects\\QuiteRSS-build-desktop_Release\\release\\target"
prepareAbsPath  = "e:\\Work\\_Useful\\QtProjects\\QuiteRSS_prepare-install"

# Список файлов состоит из относительного пути папки, содержащей файл,
# и имени файла, который необходимо скопировать
idDir  = 0
idName = 1
filesFromSource = [
  ['\\sound', 'notification.wav'],
  ['', 'AUTHORS'],
  ['', 'COPYING'],
  ['', 'HISTORY_EN'],
  ['', 'HISTORY_RU'],
  ['', 'README'],
  ['', 'TODO']
]

def preparePath(path):
  print "---- Preparing path: " + path
  if (os.path.exists(path)):
    print "Path exists. Remove it"
    shutil.rmtree(path)
  
  os.makedirs(path)
  print "Path created"
    
def copyLangFiles():
  print "---- Copying language files..."
  shutil.copytree(quiterssReleaseAbsPath + "\\lang", prepareAbsPath + "\\lang")
  print "Done"

def copyExeFile():
  print "---- Copying exe-file..."
  shutil.copy(quiterssReleaseAbsPath + "\\QuiteRSS.exe", prepareAbsPath + "\\QuiteRSS.exe")
  print "Done"

def copyFilesFromSource():
  print "---- Copying files from source..."
  
  # Перебираем список файлов
  for file in filesFromSource:
    print file[idDir] + '\\' + file[idName]
    
    # Если есть имя папки, то создаём её
    if file[idDir]:
      shutil.copytree(quiterssSourceAbsPath + file[idDir], prepareAbsPath + file[idDir])
      
    # Копируем файл
    shutil.copy(quiterssSourceAbsPath + file[idDir] + '\\' + file[idName], prepareAbsPath + file[idDir] + '\\' + file[idName])
    
  print "Done"

def main():
  print "QuiteRSS prepare-install"
  preparePath(prepareAbsPath)
  copyLangFiles()
  copyExeFile()
  copyFilesFromSource()

if __name__ == '__main__':
  main()