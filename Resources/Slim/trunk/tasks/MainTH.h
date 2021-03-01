#ifndef __MAINTASKHANDLER_H
#define __MAINTASKHANDLER_H

#include "TaskHandler.h"

class MainTH : public TaskHandler {
public:
	MainTH (Config *conf);
	virtual ~MainTH ();

	virtual void HandleTask();
	virtual string BuildWorkingDir();

protected:
	//void	NormaleDGen();		-- disabled by mvl for blocks, because it needs dgen

	//void	ConvertFIS();

	void	CreateDependentDB();
	void	AnalyseDB();
	//void	AnalyseCT();		 -- disabled by mvl for blocks, because it needs dgen
	void	CreateFISHistogram();

	void	BuildCTTree();

	void	DailyDiyahSpecial();
	void	AnotherDiyahSpecial();

	void	GetLastCTAndWriteToDisk();

	// SwapRandomise Krimp experiments
	void	SwpRndKrmpXps();
};

#endif // __MAINTASKHANDLER_H
