#ifndef _WS_PROPS_H
#define _WS_PROPS_H

class Props
{
private:
    char *firmwareVersion;
    char *gitRevision;
public:
    Props(char* fwVer, char* gitRev);
    char* getFirmwareVersion();
    char* getGitRevision();
    int getDoSleep();
    int getSleepTimeSec();
    int getMeasureIntervalMs();
    int getMeasureBatchSize();
    int getLedPin();

};

#endif