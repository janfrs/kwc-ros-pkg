/*
 * Copyright (c) 2008, Maxim Likhachev
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of Pennsylvania nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __ENVIRONMENT_NAV2D_H_
#define __ENVIRONMENT_NAV2D_H_


#define ENVNAV2D_COSTMULT 1000


//-1, 0, 1 per each dX and dY
#define ENVNAV2D_ACTIONSWIDTH 8

#define ENVNAV2D_DEFAULTOBSTHRESH 253 //253-for willow garage	//see explanation of the value below

//configuration parameters
typedef struct ENV_NAV2D_CONFIG
{
	int EnvWidth_c;
	int EnvHeight_c;
	int StartX_c;
	int StartY_c;
	int EndX_c;
	int EndY_c;
	unsigned char** Grid2D;
	//the value at which and above which cells are obstacles in the maps sent from outside
	//the default is defined above
	unsigned char obsthresh; 

	int dXY[ENVNAV2D_ACTIONSWIDTH][2];


} EnvNAV2DConfig_t;

typedef struct ENVHASHENTRY
{
	int stateID;
	int X;
	int Y;
} EnvNAV2DHashEntry_t;


typedef struct NAV2D_CELL
{
	int x;
	int y;
}nav2dcell_t;

//variables that dynamically change (e.g., array of states, ...)
typedef struct
{

	int startstateid;
	int goalstateid;

	//hash table of size x_size*y_size. Maps from coords to stateId	
	int HashTableSize;
	vector<EnvNAV2DHashEntry_t*>* Coord2StateIDHashTable;

	//vector that maps from stateID to coords	
	vector<EnvNAV2DHashEntry_t*> StateID2CoordTable;

	//any additional variables

}EnvironmentNAV2D_t;



class EnvironmentNAV2D : public DiscreteSpaceInformation
{

public:

	bool InitializeEnv(const char* sEnvFile);


	bool InitializeMDPCfg(MDPConfig *MDPCfg);
	int  GetFromToHeuristic(int FromStateID, int ToStateID);
	int  GetGoalHeuristic(int stateID);
	int  GetStartHeuristic(int stateID);
	void SetAllActionsandAllOutcomes(CMDPSTATE* state);
	void SetAllPreds(CMDPSTATE* state);
	void GetSuccs(int SourceStateID, vector<int>* SuccIDV, vector<int>* CostV);
	void GetPreds(int TargetStateID, vector<int>* PredIDV, vector<int>* CostV);

	int	 SizeofCreatedEnv();
	void PrintState(int stateID, bool bVerbose, FILE* fOut=NULL);
	void PrintEnv_Config(FILE* fOut);
    
    bool InitializeEnv(int width, int height,
		       /** if mapdata is NULL the grid is initialized to all freespace */
                       const unsigned char* mapdata,
                       int startx, int starty,
                       int goalx, int goaly, unsigned char obsthresh);
    int SetStart(int x, int y);
    int SetGoal(int x, int y);
    bool UpdateCost(int x, int y, unsigned char newcost);
	void GetPredsofChangedEdges(vector<nav2dcell_t> const * changedcellsV, vector<int> *preds_of_changededgesIDV);

	void SetConfiguration(int width, int height,
			      /** if mapdata is NULL the grid is initialized to all freespace */
			      const unsigned char* mapdata,
			      int startx, int starty,
			      int goalx, int goaly);
	
	bool InitGeneral();

	void GetCoordFromState(int stateID, int& x, int& y) const;

	int GetStateFromCoord(int x, int y);

	bool IsObstacle(int x, int y);
	unsigned char GetMapCost(int x, int y);
	void GetEnvParms(int *size_x, int *size_y, int* startx, int* starty, int* goalx, int* goaly, unsigned char* obsthresh);

	const EnvNAV2DConfig_t* GetEnvNavConfig();

	EnvironmentNAV2D();
    ~EnvironmentNAV2D();

    void PrintTimeStat(FILE* fOut);
	 
	bool IsWithinMapCell(int X, int Y);

private:

	//member data
	EnvNAV2DConfig_t EnvNAV2DCfg;
	EnvironmentNAV2D_t EnvNAV2D;


	void ReadConfiguration(FILE* fCfg);

	void InitializeEnvConfig();

	unsigned int GETHASHBIN(unsigned int X, unsigned int Y);

	void PrintHashTableHist();


	EnvNAV2DHashEntry_t* GetHashEntry(int X, int Y);

	EnvNAV2DHashEntry_t* CreateNewHashEntry(int X, int Y);


	void CreateStartandGoalStates();

	void InitializeEnvironment();

	void ComputeHeuristicValues();

	bool IsValidCell(int X, int Y);


};

#endif

