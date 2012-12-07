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

filesFromSource = [
  '\\AUTHORS',
  '\\COPYING',
  '\\HISTORY_EN',
  '\\HISTORY_RU',
  '\\README',
  '\\TODO'
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

def copySoundFromSource():
  print "---- Copying sound files..."
  shutil.copytree(quiterssSourceAbsPath + "\\sound", prepareAbsPath + "\\sound")
  print "Done"

def copyFilesFromSource():
  print "---- Copying files from source..."
  for file in filesFromSource:
    print "copying: " + quiterssSourceAbsPath+file
    shutil.copy(quiterssSourceAbsPath + file, prepareAbsPath + file)
  print "Done"

def main():
  print "QuiteRSS prepare-install"
  preparePath(prepareAbsPath)
  copyLangFiles()
  copyExeFile()
  copySoundFromSource()
  copyFilesFromSource()

if __name__ == '__main__':
  main()