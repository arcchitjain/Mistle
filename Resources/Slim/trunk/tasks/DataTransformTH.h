#ifndef __DATABASETRANSFORMATIONTASKHANDLER_H
#define __DATABASETRANSFORMATIONTASKHANDLER_H

#include "TaskHandler.h"

class DataTransformTH : public TaskHandler {
public:
	DataTransformTH (Config *conf);
	virtual ~DataTransformTH ();

	virtual void HandleTask();

protected:
	void	ConvertDB();
	void	BinDB();
	void	UnBinDB();
	void	TransformDB();
	void	SwapDB();
	void	SwapRandomiseDB();
	
	void	ConvertISC();
	void	Isc2Bisc();

	void	MergeChunks();	// for emergency chunk-merging

	void	SplitDB();
	void	SampleDB();


	//void	ConvertFIS();
};

#endif // __DATABASETRANSFORMATIONTASKHANDLER_H
