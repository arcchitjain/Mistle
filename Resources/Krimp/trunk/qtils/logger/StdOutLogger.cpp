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

#include "StdOutLogger.h"

#define PROGRESS_BAR_LEN		30

StdOutLogger::StdOutLogger(uint32 maxLevel) : Logger(maxLevel) {
	mProgress = false;
	mProgressMax = 0;
	mMessage = "";
}
StdOutLogger::~StdOutLogger() {

}
void StdOutLogger::LogMessage(const string &msg) {
	if(!mEnabled) return;
	if(mProgress)
		mMessage = msg;
	else
		puts(msg.c_str());
}
void StdOutLogger::LogMessage(const string &msg, uint32 level) {
	if(!mEnabled) return;
	if(mProgress)
		mMessage = msg;
	else if(level <= mMaxLevel) {
		for(uint32 i=0; i<level; i++)
			printf("\t");
		puts(msg.c_str());
	}
}
void StdOutLogger::InitProgress(uint32 max, string message) {
	if(!mEnabled) return;
	mProgressMax = max;
	mProgress = true;
	mMessage = message;
}
void StdOutLogger::ShowProgress(uint32 current) {
	if(!mEnabled) return;
	uint32 num = (PROGRESS_BAR_LEN * current) / mProgressMax;
	printf("[");
	for(uint32 i=0; i<num; i++)
		printf("%C",240);
	for(uint32 i=num; i<PROGRESS_BAR_LEN; i++)
		printf(" ");
	uint32 charsLeft = 78 - PROGRESS_BAR_LEN;
	charsLeft -= printf("] %d/%d : ", current, mProgressMax); 
	printf("%s\r", mMessage.substr(0,charsLeft).c_str());
}
void StdOutLogger::EndProgress() {
	if(!mEnabled) return;
	printf("\n");
	mProgressMax = 0;
	mProgress = false;
	mMessage = "";
}
