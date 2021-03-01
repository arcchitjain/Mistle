//
// Copyright (c) 2005-2012, Matthijs van Leeuwen, Jilles Vreeken, Koen Smets
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials // provided with the distribution.
// Neither the name of the Universiteit Utrecht, Universiteit Antwerpen, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#include "../qlobal.h"

#include "FileLogger.h"

FileLogger::FileLogger(const string &dir, const string &filename, uint32 maxLevel) : Logger(maxLevel), mDir(dir), mFilename(filename), mFileOpen(false), mFile(NULL) {
}
FileLogger::~FileLogger() {
	if(mFileOpen)
		fclose(mFile);
}
void FileLogger::SetDirectory(const string &dir) {
	if(!mFileOpen)
		mDir = dir;
}
void FileLogger::OpenFile() {
	if(mFileOpen == true)
		throw "FileLogger::FileLogger -- Logfile already opened.";
	string fullPath = mDir + mFilename;
	if(fopen_s(&mFile, fullPath.c_str(), "a+") != 0)
		throw "FileLogger::FileLogger -- Cannot open logfile for writing.";
	mFileOpen = true;
}
void FileLogger::LogMessage(const string &msg) {
	if(!mEnabled) return;
	if(!mFileOpen) OpenFile();
	fprintf(mFile, "%s\n", msg.c_str());
	fflush(mFile);
}
void FileLogger::LogMessage(const string &msg, uint32 level) {
	if(!mEnabled) return;
	if(!mFileOpen) OpenFile();
	if(level <= mMaxLevel) {
		for(uint32 i=0; i<level; i++)
			fputs("\t", mFile);
		fprintf(mFile, "%s\n", msg.c_str());
		fflush(mFile);
	}
}
