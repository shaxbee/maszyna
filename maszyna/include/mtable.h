#ifndef mtableH
#define mtableH

#pragma hdrstop


struct TMTableLine
{
	double km;
	double vmax;
	std::string StationName;
    std::string StationWare;
	int TrackNo;
	int Ah;
	int Am;
	int Dh;
	int Dm;
	double tm;
	int WaitTime;
};

typedef TMTableLine TMTable[101];

class TTrainParameters;
typedef TTrainParameters* *PTrainParameters;



class TTrainParameters 
{
public:
	std::string TrainName;
	double TTVmax;
	std::string Relation1;
	std::string Relation2;
	double BrakeRatio;
	std::string LocSeries;
	double LocLoad;
	TMTableLine TimeTable[101];
	int StationCount;
	int StationIndex;
	std::string NextStationName;
	double LastStationLatency;
	int Direction;
	double __fastcall CheckTrainLatency(void);
	std::string __fastcall ShowRelation();
	double __fastcall WatchMTable(double DistCounter);
	std::string __fastcall NextStop();
	bool __fastcall IsStop(void);
	bool __fastcall IsTimeToGo(double hh, double mm);
	bool __fastcall UpdateMTable(double hh, double mm, std::string NewName);
	__fastcall TTrainParameters(std::string NewTrainName);
	void __fastcall NewName(std::string NewTrainName);
	bool __fastcall LoadTTfile(std::string scnpath);
	bool __fastcall DirectionChange(void);
	void __fastcall StationIndexInc(void);
public:

	/* TObject.Create */ inline __fastcall TTrainParameters(void) { }

	/* TObject.Destroy */ inline __fastcall ~TTrainParameters(void) { }

	
};


class TMTableTime 
{

public:
	double GameTime;
	int dd;
	int hh;
	int mm;
	int ss;
	int srh;
	int srm;
	int ssh;
	int ssm;
	int h1;
	int h2;
	int m1;
	int m2;
	int s1;
	int s2;
	//AnsiString shh;
	//AnsiString smm;
	//AnsiString sss;
	float mr;
	double bysec;
	void __fastcall UpdateMTableTime(double deltaT);
	void __fastcall Init(int InitH, int InitM, int InitSRH, int InitSRM, int InitSSH, int InitSSM);
public:

__fastcall TMTableTime(void)  { }

 __fastcall ~TMTableTime(void) { }

	
};
extern TMTableTime* GlobalTime;

//---------------------------------------------------------------------------
#endif
