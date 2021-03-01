#ifndef __TASKHANDLER_H
#define __TASKHANDLER_H

class Config;

class TaskHandler {
public:
	TaskHandler(Config *conf);
	virtual ~TaskHandler();

	virtual void HandleTask()=0;

	// Return false if no logs should be written to disk for a particular task.
	virtual bool ShouldWriteTaskLogs();

	// returns the *suggested* working directory for the experiment (or other task at hand)
	// default working dir: [taskClass]\[command]\[timestamp]\
		// (can be overridden by specific TaskHandlers)
	virtual string BuildWorkingDir();

protected:
	Config*			mConfig;
};

#endif // __TASKHANDLER_H
