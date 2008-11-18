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
#ifndef __PLANNER_H_
#define __PLANNER_H_


#define 	GETSTATEIND(stateid, mapid) StateID2IndexMapping[mapid][stateid]


//indices for the StateID2Index mapping
enum STATEID2IND {	STATEID2IND_SLOT0 = 0, //add more slots if necessary
				 NUMOFINDICES_STATEID2IND
};

//use the slots above for the mutually exclusive algorithms
#define VIMDP_STATEID2IND STATEID2IND_SLOT0
#define ARAMDP_STATEID2IND STATEID2IND_SLOT0
#define ADMDP_STATEID2IND STATEID2IND_SLOT0

//for example
//#define YYYPLANNER_STATEID2IND STATEID2IND_SLOT0
//#define YYYPLANNER_STATEID2IND STATEID2IND_SLOT1


typedef enum 
	{	//different state types if you have more than one type inside a single planner
		ABSTRACT_STATE = 0,
		ABSTRACT_STATEACTIONPAIR,
		ABSTRACT_GENERALSTATE
} AbstractSearchStateType_t; 

class AbstractSearchState
{

public:
	struct listelement* listelem[2];
	//index of the state in the heap
	int heapindex;
	AbstractSearchStateType_t StateType; 

public:
	AbstractSearchState(){StateType = ABSTRACT_GENERALSTATE;};
	~AbstractSearchState(){};
};

class DiscreteSpaceInformation;

class SBPLPlanner
{

public:

	//returns 1 if solution is found, 0 otherwise
    //will replan incrementally if possible (e.g., supported by the planner and not forced to replan from scratch)
	virtual int replan(double allocated_time_sec, vector<int>* solution_stateIDs_V) = 0;

    //sets the goal of search (planner will automatically decide whether it needs to replan from scratch)
    virtual int set_goal(int goal_stateID) = 0;

    //sets the start of search (planner will automatically decide whether it needs to replan from scratch)
    virtual int set_start(int start_stateID) = 0;

    //forgets previous planning efforts and starts planning from scratch next time replan is called
    virtual int force_planning_from_scratch() = 0; 

	//sets the mode for searching
	//if bSearchUntilFirstSolution is false, then planner searches for at most allocatime_time_sec, independently of whether it finds a solution or not (default mode)
	//if bSearchUntilFirstSolution is true, then planner searches until it finds the first solution. It may be faster than allocated_time or it may be longer
	//In other words, in the latter case, the planner does not spend time on improving the solution even if time permits, but may also take longer than allocated_time before returning
	//So, normally bSearchUntilFirstSolution should be set to false.
	virtual int set_search_mode(bool bSearchUntilFirstSolution) = 0;

    // Notifies the planner that costs have changed. May need to be specialized for different subclasses in terms of what to
    // do here
    virtual void costs_changed() {}

    virtual ~SBPLPlanner(){};

protected:
	DiscreteSpaceInformation *environment_;

};



#endif

