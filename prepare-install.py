# -*- coding: utf-8 -*-
'''
Подготовка файлов перед выпуском новой версии
 
@file prepare-install.py
@author aleksey.hohryakov@gmail.com
'''

import hashlib
import os
import shutil
from subprocess import call

qtsdkAbsPath = 'c:\\QtSDK\\Desktop\\Qt\\4.8.0\\mingw'
quiterssSourceAbsPath = "e:\\Work\\_Useful\\QtProjects\\QuiteRSS"
quiterssReleaseAbsPath = "e:\\Work\\_Useful\\QtProjects\\QuiteRSS-build-desktop_Release\\release\\target"
prepareAbsPath  = "e:\\Work\\_Useful\\QtProjects\\QuiteRSS_prepare-install"
quiterssFileRepoPath = 'e:\\Work\\_Useful\\QtProjects\\QuiteRss.File - copy'
packerPath = 'e:\\Work\\_Utilities\\7za\\7za.exe'

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

filesFromRelease = [
  ['', 'QuiteRSS.exe'],
  ['', '7za.exe'],
  ['', 'Updater.exe']
]

filesFromQtSDKPlugins = [
  ['\\sqldrivers', 'qsqlite4.dll'],
  ['\\iconengines', 'qsvgicon4.dll'],
  ['\\imageformats', 'qgif4.dll'],
  ['\\imageformats', 'qico4.dll'],
  ['\\imageformats', 'qjpeg4.dll'],
  ['\\imageformats', 'qmng4.dll'],
  ['\\imageformats', 'qsvg4.dll'],
  ['\\imageformats', 'qtga4.dll'],
  ['\\imageformats', 'qtiff4.dll'],
  ['\\phonon_backend', 'phonon_ds94.dll']
]

filesFromQtSDKBin = [
  ['', 'libeay32.dll'],
  ['', 'libgcc_s_dw2-1.dll'],
  ['', 'libssl32.dll'],
  ['', 'mingwm10.dll'],
  ['', 'phonon4.dll'],
  ['', 'QtCore4.dll'],
  ['', 'QtGui4.dll'],
  ['', 'QtNetwork4.dll'],
  ['', 'QtSql4.dll'],
  ['', 'QtSvg4.dll'],
  ['', 'QtWebKit4.dll'],
  ['', 'QtXml4.dll'],
  ['', 'ssleay32.dll']
]

prepareFileList = []

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
  
  global prepareFileList
  langFiles = os.listdir(prepareAbsPath + "\\lang")
  for langFile in langFiles:
    langPath = '\\lang\\' + langFile
    print langPath
    prepareFileList.append(langPath)
    
  print "Done"

def copyFileList(fileList, src):
  '''
  Копирование файлов из списка fileList из папки src
  '''
  print "---- Copying files from " + src
  
  global prepareFileList
  
  # Перебираем список файлов
  for file in fileList:
    print file[idDir] + '\\' + file[idName]
    
    # Если есть имя папки, то создаём её
    if file[idDir] and (not os.path.exists(prepareAbsPath + file[idDir])):
      os.makedirs(prepareAbsPath + file[idDir])
      
    # Копируем файл, обрабатывая ошибки
    try:
      shutil.copy(src + file[idDir] + '\\' + file[idName], prepareAbsPath + file[idDir] + '\\' + file[idName])
    except (IOError, os.error), why:
      print str(why)
      
    prepareFileList.append(file[idDir] + '\\' + file[idName])
    
  print "Done"

def createMD5(fileList, path):
  '''
  Формирование md5-файла
  '''
  print "---- Create md5-file for all files in " + path
  
  f = open(path + '\\file_list.md5', 'w')
  for file in fileList:
    fileName = path + file
    fileHash = hashlib.md5(open(fileName, 'r').read()).hexdigest()
    line = fileHash + ' *' + file[1:]
    f.write(line + '\n')
    print line
    
  f.close()
  print "Done"

def copyMD5():
  print "---- Copying md5-file to quiterss.file-repo"
  shutil.copy(prepareAbsPath + '\\file_list.md5', quiterssFileRepoPath + '\\file_list.md5')
  print "Done"

def packFiles(fileList, path):
  '''
  Пакуем каждый файл в индивидуальный архив
  '''
  print '---- Pack files'
  for file in fileList:
    packCmdLine = packerPath + ' a "' + path + file + '.7z" "' + path + file + '"'
    print 'subprocess.call(' + packCmdLine + ')'
    call(packCmdLine);
  
  print "Done"

def copyPackedFiles():
  print '---- Copying packed files to quiterss.file-repo.windows'
  
  prepareFileList7z = []
  for file in prepareFileList:
    prepareFileList7z.append(file + '.7z')
  
  for file in prepareFileList7z:
    print 'copying: ' + file
    shutil.copy(prepareAbsPath + file, quiterssFileRepoPath + '\\windows' + file)
  
  print 'Done'

def updateFileRepo():
  print '---- Updating repo: ' + quiterssFileRepoPath
  
  callLine = 'hg commit --cwd "' + quiterssFileRepoPath + '"' \
      + ' --addremove --subrepos --message "update windows install files"'
  print 'call(' + callLine + ')'
  call(callLine)
  print ""
  
  callLine = 'hg log --cwd "' + quiterssFileRepoPath + '"' \
      + ' --verbose --limit 1'
  print 'call(' + callLine + ')'
  call(callLine)
  print ""
  
  # callLine = 'hg push --cwd "' + quiterssFileRepoPath + '"'
  # print 'call(' + callLine + ')'
  # call(callLine)
  # print ""
  
  print 'Done'

def main():
  print "QuiteRSS prepare-install"
  preparePath(prepareAbsPath)
  copyLangFiles()
  copyFileList(filesFromRelease, quiterssReleaseAbsPath)
  copyFileList(filesFromSource, quiterssSourceAbsPath)
  copyFileList(filesFromQtSDKPlugins, qtsdkAbsPath + '\\plugins')
  copyFileList(filesFromQtSDKBin, qtsdkAbsPath + '\\bin')
  createMD5(prepareFileList, prepareAbsPath)
  copyMD5()
  packFiles(prepareFileList, prepareAbsPath)
  copyPackedFiles()
  updateFileRepo()

if __name__ == '__main__':
  main()
