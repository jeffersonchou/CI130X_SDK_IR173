#!/usr/bin/python
# -*- coding: UTF-8 -*-
 
import time
import os
import sys
import re
from shutil import copyfile


class TypeFiles(object):
	"""docstring for TypeFiles"""
	def __init__(self, path):
		super(TypeFiles, self).__init__()
		self.path = path
		self.AllTypeFiles = []


	def GetDirExcelFiles(self, dir):
		if os.path.isdir(dir):
			filelist = os.listdir(dir)
			for file in filelist:
				dirfil = dir + "\\" + file
				if os.path.isdir(dirfil):
					self.GetDirExcelFiles(dirfil)
					continue
				abs_name = os.path.abspath(file)#文件名
				file_ex = os.path.splitext(file)[1]
				if self.file_type != file_ex:
					continue
				self.AllTypeFiles.append(dirfil)


	def GetAllFilesInDir(self, file_type):
		self.AllTypeFiles.clear()
		self.file_type = file_type

		filelist = os.listdir(self.path)
		for file in filelist:
			dirfil = self.path + "\\" + file
			if os.path.isdir(dirfil):
				self.GetDirExcelFiles(dirfil)
				continue
			abs_name = os.path.abspath(file)#文件名
			fileEx = os.path.splitext(file)[1]   #文件名
			if self.file_type != fileEx:
				continue
			self.AllTypeFiles.append(dirfil)
		return self.AllTypeFiles
		

def GetAllTypeFiles(path, file_name_ex):
	class_files = TypeFiles(path)
	return class_files.GetAllFilesInDir(file_name_ex)


class AllDatFilesClass(object):
	"""docstring for AllDatFilesClass"""
	def __init__(self, path):
		super(AllDatFilesClass, self).__init__()
		self.path = path
		self.TypeFiles = GetAllTypeFiles(path, ".dat")
		self.NormalFiles = []

	def GetAllNormalFiles(self):
		self.NormalFiles.clear()
		for file in self.TypeFiles:
			filename = os.path.basename(file)
			if "normal" in filename:
				self.NormalFiles.append(file)

	def GetAllIndexDatFiles(self):
		for file in self.NormalFiles:
			self.GetIndexDatFile(file)
		pass

	def GetIndexDatFile(self, file):
		file_name = os.path.basename(file)
		dir_name = os.path.dirname(file)
		father_dir_name = os.path.basename(dir_name)

		if (True != father_dir_name.isdigit()):
			return

		target_file = "["+father_dir_name+"]"+file_name;
		print("file:" + file + " target:"+ target_file)
		copyfile(file, target_file)
		pass


def main(argv):
	DatFiles = AllDatFilesClass(os.getcwd())
	DatFiles.GetAllNormalFiles()
	DatFiles.GetAllIndexDatFiles()



	pass

if __name__ == '__main__':
	main(sys.argv)