#ifndef __FICCONF_H
#define __FICCONF_H

class Config;

class FicConf {
public:
	FicConf();
	~FicConf();

	Config*			GetConfig(uint32 inst=0);
	uint32			GetNumInstances();

	void			Read(string confName);
	void			Write(const string &confName);

protected:
	Config*			mConfig;
};

#endif // __FICCONF_H
