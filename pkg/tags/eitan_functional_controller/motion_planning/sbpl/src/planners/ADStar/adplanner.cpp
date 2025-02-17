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
#include "../../headers.h"




//-----------------------------------------------------------------------------------------------------

ADPlanner::ADPlanner(DiscreteSpaceInformation* environment)
{
    environment_ = environment;
    
    finitial_eps = AD_DEFAULT_INITIAL_EPS;
    searchexpands = 0;
    MaxMemoryCounter = 0;
    
    fDeb = fopen("debug.txt", "w");
    
    pSearchStateSpace_ = new ADSearchStateSpace_t;
    
    
    //create the AD planner
    if(CreateSearchStateSpace(pSearchStateSpace_) != 1)
        {
            printf("ERROR: failed to create statespace\n");
            return;
        }
    
    //set the start and goal states
    if(InitializeSearchStateSpace(pSearchStateSpace_) != 1)
        {
            printf("ERROR: failed to create statespace\n");
            return;
        }    
}

ADPlanner::~ADPlanner()
{

    //delete the statespace
    DeleteSearchStateSpace(pSearchStateSpace_);


}


void ADPlanner::Initialize_searchinfo(CMDPSTATE* state, ADSearchStateSpace_t* pSearchStateSpace)
{

	ADState* searchstateinfo = (ADState*)state->PlannerSpecificData;

	searchstateinfo->MDPstate = state;
	InitializeSearchStateInfo(searchstateinfo, pSearchStateSpace); 
}


CMDPSTATE* ADPlanner::CreateState(int stateID, ADSearchStateSpace_t* pSearchStateSpace)
{	
	CMDPSTATE* state = NULL;

#if DEBUG
	if(environment_->StateID2IndexMapping[stateID][ADMDP_STATEID2IND] != -1)
	{
		printf("ERROR in CreateState: state already created\n");
		exit(1);
	}
#endif

	//adds to the tail a state
	state = pSearchStateSpace->searchMDP.AddState(stateID);

	//remember the index of the state
	environment_->StateID2IndexMapping[stateID][ADMDP_STATEID2IND] = pSearchStateSpace->searchMDP.StateArray.size()-1;

#if DEBUG
	if(state != pSearchStateSpace->searchMDP.StateArray[environment_->StateID2IndexMapping[stateID][ADMDP_STATEID2IND]])
	{
		printf("ERROR in CreateState: invalid state index\n");
		exit(1);
	}
#endif


	//create search specific info
	state->PlannerSpecificData = (ADState*)malloc(sizeof(ADState));	
	Initialize_searchinfo(state, pSearchStateSpace);
	MaxMemoryCounter += sizeof(ADState);

	return state;

}

CMDPSTATE* ADPlanner::GetState(int stateID, ADSearchStateSpace_t* pSearchStateSpace)
{	

	if(stateID >= (int)environment_->StateID2IndexMapping.size())
	{
		printf("ERROR int GetState: stateID is invalid\n");
		exit(1);
	}

	if(environment_->StateID2IndexMapping[stateID][ADMDP_STATEID2IND] == -1)
		return CreateState(stateID, pSearchStateSpace);
	else
		return pSearchStateSpace->searchMDP.StateArray[environment_->StateID2IndexMapping[stateID][ADMDP_STATEID2IND]];

}



//-----------------------------------------------------------------------------------------------------


CKey ADPlanner::ComputeKey(ADState* state)
{
	CKey key;

	if(state->v >= state->g)
	{
		key.key[0] = state->g + (int)(pSearchStateSpace_->eps*state->h);
		key.key[1] = 1;
	}
	else
	{
		key.key[0] = state->v + state->h;
		key.key[1] = 0;
	}

	return key;
}


int ADPlanner::ComputeHeuristic(CMDPSTATE* MDPstate, ADSearchStateSpace_t* pSearchStateSpace)
{
	//compute heuristic for search
#if AD_SEARCH_FORWARD == 1

#if MEM_CHECK == 1
	//int WasEn = DisableMemCheck();
#endif

    //forward search: heur = distance from state to searchgoal which is Goal ADState
	int retv =  environment_->GetFromToHeuristic(MDPstate->StateID, pSearchStateSpace->searchgoalstate->StateID);

#if MEM_CHECK == 1
	//if (WasEn)
	//	EnableMemCheck();
#endif

	return retv;

#else
	//backward search: heur = distance from searchgoal to state
	return environment_->GetFromToHeuristic(pSearchStateSpace->searchgoalstate->StateID, MDPstate->StateID);
#endif

}


//initialization of a state
void ADPlanner::InitializeSearchStateInfo(ADState* state, ADSearchStateSpace_t* pSearchStateSpace)
{
	state->g = INFINITECOST;
	state->v = INFINITECOST;
	state->iterationclosed = 0;
	state->callnumberaccessed = pSearchStateSpace->callnumber;
	state->bestnextstate = NULL;
	state->costtobestnextstate = INFINITECOST;
	state->heapindex = 0;
	state->listelem[AD_INCONS_LIST_ID] = NULL;
	state->numofexpands = 0;

#if AD_SEARCH_FORWARD == 1
	state->bestpredstate = NULL;
#endif

	//compute heuristics
#if USE_HEUR
	if(pSearchStateSpace->searchgoalstate != NULL)
		state->h = ComputeHeuristic(state->MDPstate, pSearchStateSpace); 
	else 
		state->h = 0;
#else
	state->h = 0;
#endif


}



//re-initialization of a state
void ADPlanner::ReInitializeSearchStateInfo(ADState* state, ADSearchStateSpace_t* pSearchStateSpace)
{
	state->g = INFINITECOST;
	state->v = INFINITECOST;
	state->iterationclosed = 0;
	state->callnumberaccessed = pSearchStateSpace->callnumber;
	state->bestnextstate = NULL;
	state->costtobestnextstate = INFINITECOST;
	state->heapindex = 0;
	state->listelem[AD_INCONS_LIST_ID] = NULL;
	state->numofexpands = 0;

#if AD_SEARCH_FORWARD == 1
	state->bestpredstate = NULL;
#endif

	//compute heuristics
#if USE_HEUR

	if(pSearchStateSpace->searchgoalstate != NULL)
	{

#if AD_SEARCH_FORWARD == 0
		state->h = ComputeHeuristic(state->MDPstate, pSearchStateSpace); 
#endif

	}
	else 
		state->h = 0;

#else

	state->h = 0;

#endif


}



void ADPlanner::DeleteSearchStateData(ADState* state)
{
	//no memory was allocated
	MaxMemoryCounter = 0;
	return;
}


void ADPlanner::UpdateSetMembership(ADState* state)
{
	CKey key;

    if(state->v != state->g)
    {
        if(state->iterationclosed != pSearchStateSpace_->iteration)
        {
			key = ComputeKey(state);
            if(state->heapindex == 0)
                pSearchStateSpace_->heap->insertheap(state, key);
            else
				pSearchStateSpace_->heap->updateheap(state, key);
        }
		else if(state->listelem[AD_INCONS_LIST_ID] == NULL)
		{
			pSearchStateSpace_->inconslist->insert(state, AD_INCONS_LIST_ID);
		}
    }
    else
    {
        if(state->heapindex != 0)
            pSearchStateSpace_->heap->deleteheap(state);
        else if(state->listelem[AD_INCONS_LIST_ID] != NULL)
			pSearchStateSpace_->inconslist->remove(state, AD_INCONS_LIST_ID);
    }
}


void ADPlanner::Recomputegval(ADState* state)
{
    vector<int> searchpredsIDV; //these are predecessors if search is done forward and successors otherwise
    vector<int> costV;
	CKey key;
	ADState *searchpredstate;

#if AD_SEARCH_FORWARD
    environment_->GetPreds(state->MDPstate->StateID, &searchpredsIDV, &costV);
#else
    environment_->GetSuccs(state->MDPstate->StateID, &searchpredsIDV, &costV);
#endif


	//iterate through predecessors of s and pick the best
	state->g = INFINITECOST;
	for(int pind = 0; pind < (int)searchpredsIDV.size(); pind++)
	{
		CMDPSTATE* predMDPState = GetState(searchpredsIDV[pind], pSearchStateSpace_);
		int cost = costV[pind];
		searchpredstate = (ADState*)(predMDPState->PlannerSpecificData);
	
		//see if it can be used to improve
		if(searchpredstate->callnumberaccessed == pSearchStateSpace_->callnumber && state->g > searchpredstate->v + cost)
		{

#if AD_SEARCH_FORWARD
			state->g = searchpredstate->v + cost;
			state->bestpredstate = searchpredstate;
#else
			state->g = searchpredstate->v + cost;
			state->bestnextstate = predMDPState;
			state->costtobestnextstate = cost;
#endif
		}		
	} //over preds
}



#if !AD_SEARCH_FORWARD
//used for backward search
void ADPlanner::UpdatePredsofOverconsState(ADState* state, ADSearchStateSpace_t* pSearchStateSpace)
{
    vector<int> PredIDV;
    vector<int> CostV;
	CKey key;
	ADState *predstate;

    environment_->GetPreds(state->MDPstate->StateID, &PredIDV, &CostV);

	//iterate through predecessors of s
	for(int pind = 0; pind < (int)PredIDV.size(); pind++)
	{
		CMDPSTATE* PredMDPState = GetState(PredIDV[pind], pSearchStateSpace);
		predstate = (ADState*)(PredMDPState->PlannerSpecificData);
		if(predstate->callnumberaccessed != pSearchStateSpace->callnumber)
			ReInitializeSearchStateInfo(predstate, pSearchStateSpace);

		//see if we can improve the value of predstate
		if(predstate->g > state->v + CostV[pind])
		{
			predstate->g = state->v + CostV[pind];
			predstate->bestnextstate = state->MDPstate;
			predstate->costtobestnextstate = CostV[pind];

			//update set membership
			UpdateSetMembership(predstate);

#if DEBUG
			fprintf(fDeb, "updated pred %d of overcons exp\n", predstate->MDPstate->StateID);
			PrintSearchState(predstate, fDeb);
			fprintf(fDeb, "\n");
#endif

		}
	} //for predecessors

}
#endif

#if AD_SEARCH_FORWARD
//used for forward search
void ADPlanner::UpdateSuccsofOverconsState(ADState* state, ADSearchStateSpace_t* pSearchStateSpace)
{
    vector<int> SuccIDV;
    vector<int> CostV;
	CKey key;
	ADState *succstate;

    environment_->GetSuccs(state->MDPstate->StateID, &SuccIDV, &CostV);

	//iterate through predecessors of s
	for(int sind = 0; sind < (int)SuccIDV.size(); sind++)
	{
		CMDPSTATE* SuccMDPState = GetState(SuccIDV[sind], pSearchStateSpace);
		int cost = CostV[sind];

		succstate = (ADState*)(SuccMDPState->PlannerSpecificData);
		if(succstate->callnumberaccessed != pSearchStateSpace->callnumber)
			ReInitializeSearchStateInfo(succstate, pSearchStateSpace);

		//see if we can improve the value of succstate
		//taking into account the cost of action
		if(succstate->g > state->v + cost)
		{
			succstate->g = state->v + cost;
			succstate->bestpredstate = state->MDPstate; 

			//update set membership
			UpdateSetMembership(succstate);

		} //check for cost improvement 

	} //for actions
}
#endif



#if !AD_SEARCH_FORWARD
//used for backward search
void ADPlanner::UpdatePredsofUnderconsState(ADState* state, ADSearchStateSpace_t* pSearchStateSpace)
{
    vector<int> PredIDV;
    vector<int> CostV;
	CKey key;
	ADState *predstate;

    environment_->GetPreds(state->MDPstate->StateID, &PredIDV, &CostV);

	//iterate through predecessors of s
	for(int pind = 0; pind < (int)PredIDV.size(); pind++)
	{

		CMDPSTATE* PredMDPState = GetState(PredIDV[pind], pSearchStateSpace);
		predstate = (ADState*)(PredMDPState->PlannerSpecificData);
		if(predstate->callnumberaccessed != pSearchStateSpace->callnumber)
			ReInitializeSearchStateInfo(predstate, pSearchStateSpace);

		if(predstate->bestnextstate == state->MDPstate)
        {				  
			Recomputegval(predstate);
			UpdateSetMembership(predstate);

#if DEBUG
		fprintf(fDeb, "updated pred %d of undercons exp\n", predstate->MDPstate->StateID);
		PrintSearchState(predstate, fDeb);
		fprintf(fDeb, "\n");
#endif

		}		
	} //for predecessors

}
#endif

#if AD_SEARCH_FORWARD
//used for forward search
void ADPlanner::UpdateSuccsofUnderconsState(ADState* state, ADSearchStateSpace_t* pSearchStateSpace)
{
    vector<int> SuccIDV;
    vector<int> CostV;
	CKey key;
	ADState *succstate;

    environment_->GetSuccs(state->MDPstate->StateID, &SuccIDV, &CostV);

	//iterate through predecessors of s
	for(int sind = 0; sind < (int)SuccIDV.size(); sind++)
	{
		CMDPSTATE* SuccMDPState = GetState(SuccIDV[sind], pSearchStateSpace);
		int cost = CostV[sind];

		succstate = (ADState*)(SuccMDPState->PlannerSpecificData);
		if(succstate->callnumberaccessed != pSearchStateSpace->callnumber)
			ReInitializeSearchStateInfo(succstate, pSearchStateSpace);

		if(succstate->bestpredstate == state->MDPstate)
        {				  
			Recomputegval(predstate);
			UpdateSetMembership(predstate);
		}		

	} //for actions
}
#endif



int ADPlanner::GetGVal(int StateID, ADSearchStateSpace_t* pSearchStateSpace)
{
	 CMDPSTATE* cmdp_state = GetState(StateID, pSearchStateSpace);
	 ADState* state = (ADState*)cmdp_state->PlannerSpecificData;
	 return state->g;
}

//returns 1 if the solution is found, 0 if the solution does not exist and 2 if it ran out of time
int ADPlanner::ComputePath(ADSearchStateSpace_t* pSearchStateSpace, double MaxNumofSecs)
{
	int expands;
	ADState *state, *searchgoalstate;
	CKey key, minkey;
	CKey goalkey;

	expands = 0;

	if(pSearchStateSpace->searchgoalstate == NULL)
	{
		printf("ERROR searching: no goal state is set\n");
		exit(1);
	}

	//goal state
	searchgoalstate = (ADState*)(pSearchStateSpace->searchgoalstate->PlannerSpecificData);
	if(searchgoalstate->callnumberaccessed != pSearchStateSpace->callnumber)
		ReInitializeSearchStateInfo(searchgoalstate, pSearchStateSpace);

	//set goal key
	goalkey = ComputeKey(searchgoalstate);

	//expand states until done
	minkey = pSearchStateSpace->heap->getminkeyheap();
	CKey oldkey = minkey;
	while(!pSearchStateSpace->heap->emptyheap() && (goalkey > minkey || searchgoalstate->g > searchgoalstate->v) &&
		(clock()-TimeStarted) < MaxNumofSecs*(double)CLOCKS_PER_SEC) 
    {

		//get the state		
		state = (ADState*)pSearchStateSpace->heap->deleteminheap();


#if DEBUG
		CKey debkey = ComputeKey(state);
		fprintf(fDeb, "expanding state(%d): g=%u v=%u h=%d key=[%d %d] iterc=%d callnuma=%d expands=%d (g(goal)=%u)\n",
			state->MDPstate->StateID, state->g, state->v, state->h, (int)debkey[0], (int)debkey[1],  
			state->iterationclosed, state->callnumberaccessed, state->numofexpands, searchgoalstate->g);
		environment_->PrintState(state->MDPstate->StateID, true, fDeb);
		fflush(fDeb);
#endif

#if DEBUG
		if(minkey.key[0] < oldkey.key[0] && fabs(this->finitial_eps - 1.0) < ERR_EPS)
		{
			printf("WARN in search: the sequence of keys decreases in an optimal search\n");
			//exit(1);
		}
		oldkey = minkey;
#endif

		if(state->v == state->g)
		{
			printf("ERROR: consistent state is being expanded\n");
			exit(1);
		}

		//new expand      
		expands++;
		state->numofexpands++;

		if(state->v > state->g)
		{
			//overconsistent expansion

			//recompute state value      
			state->v = state->g;
			state->iterationclosed = pSearchStateSpace->iteration;


#if AD_SEARCH_FORWARD == 0
			UpdatePredsofOverconsState(state, pSearchStateSpace);
#else
			UpdateSuccsofOverconsState(state, pSearchStateSpace);
#endif
		}
		else
		{
			//underconsistent expansion
			printf("underconstistent expansion\n");

			//force the state to be overconsistent
			state->v = INFINITECOST;
	
			//update state membership
			UpdateSetMembership(state);


#if AD_SEARCH_FORWARD == 0
			UpdatePredsofUnderconsState(state, pSearchStateSpace);
#else
			UpdateSuccsofUnderconsState(state, pSearchStateSpace);
#endif

		}
      

		//recompute minkey
		minkey = pSearchStateSpace->heap->getminkeyheap();

		//recompute goalkey if necessary
		goalkey = ComputeKey(searchgoalstate);

		if(expands%100000 == 0 && expands > 0)
		{
			printf("expands so far=%u\n", expands);
		}

	}

	int retv = 1;
	if(searchgoalstate->g == INFINITECOST && pSearchStateSpace->heap->emptyheap())
	{
		printf("solution does not exist: search exited because heap is empty\n");
#if DEBUG
		fprintf(fDeb, "solution does not exist: search exited because heap is empty\n");
#endif
		retv = 0;
	}
	else if(!pSearchStateSpace->heap->emptyheap() && (goalkey > minkey || searchgoalstate->g > searchgoalstate->v))
	{
		printf("search exited because it ran out of time\n");
#if DEBUG
		fprintf(fDeb, "search exited because it ran out of time\n");
#endif
		retv = 2;
	}
	else if(searchgoalstate->g == INFINITECOST && !pSearchStateSpace->heap->emptyheap())
	{
		printf("solution does not exist: search exited because all candidates for expansion have infinite heuristics\n");
#if DEBUG
		fprintf(fDeb, "solution does not exist: search exited because all candidates for expansion have infinite heuristics\n");
#endif
		retv = 0;
	}
	else
	{
		printf("search exited with a solution for eps=%.3f\n", pSearchStateSpace->eps);
#if DEBUG
		fprintf(fDeb, "search exited with a solution for eps=%.3f\n", pSearchStateSpace->eps);
#endif
		retv = 1;
	}

	//fprintf(fDeb, "expanded=%d\n", expands);

	searchexpands += expands;

	return retv;		
}


void ADPlanner::BuildNewOPENList(ADSearchStateSpace_t* pSearchStateSpace)
{
	ADState *state;
	CKey key;
	CHeap* pheap = pSearchStateSpace->heap;
	CList* pinconslist = pSearchStateSpace->inconslist; 
		
	//move incons into open
	while(pinconslist->firstelement != NULL)
	  {
	    state = (ADState*)pinconslist->firstelement->liststate;
	    
	    //compute f-value
		key = ComputeKey(state);
	    
	    //insert into OPEN
	    pheap->insertheap(state, key);
	    //remove from INCONS
	    pinconslist->remove(state, AD_INCONS_LIST_ID);
	  }
}


void ADPlanner::Reevaluatefvals(ADSearchStateSpace_t* pSearchStateSpace)
{
	CKey key;
	int i;
	CHeap* pheap = pSearchStateSpace->heap;
	
#if DEBUG
	fprintf(fDeb, "re-computing heap priorities\n");
#endif

	//recompute priorities for states in OPEN and reorder it
	for (i = 1; i <= pheap->currentsize; ++i)
	  {
		ADState* state = (ADState*)pheap->heap[i].heapstate;
		pheap->heap[i].key = ComputeKey(state);
	  }
	pheap->makeheap();

	pSearchStateSpace->bReevaluatefvals = false;
}




//creates (allocates memory) search state space
//does not initialize search statespace
int ADPlanner::CreateSearchStateSpace(ADSearchStateSpace_t* pSearchStateSpace)
{

	//create a heap
	pSearchStateSpace->heap = new CHeap;
	pSearchStateSpace->inconslist = new CList;
	MaxMemoryCounter += sizeof(CHeap);
	MaxMemoryCounter += sizeof(CList);

	pSearchStateSpace->searchgoalstate = NULL;
	pSearchStateSpace->searchstartstate = NULL;

	searchexpands = 0;


    pSearchStateSpace->bReinitializeSearchStateSpace = false;
	
	return 1;
}

//deallocates memory used by SearchStateSpace
void ADPlanner::DeleteSearchStateSpace(ADSearchStateSpace_t* pSearchStateSpace)
{
	if(pSearchStateSpace->heap != NULL)
	{
		pSearchStateSpace->heap->makeemptyheap();
		delete pSearchStateSpace->heap;
		pSearchStateSpace->heap = NULL;
	}

	if(pSearchStateSpace->inconslist != NULL)
	{
		pSearchStateSpace->inconslist->makeemptylist(AD_INCONS_LIST_ID);
		delete pSearchStateSpace->inconslist;
		pSearchStateSpace->inconslist = NULL;
	}

	//delete the states themselves
	int iend = (int)pSearchStateSpace->searchMDP.StateArray.size();
	for(int i=0; i < iend; i++)
	{
		CMDPSTATE* state = pSearchStateSpace->searchMDP.StateArray[i];
		DeleteSearchStateData((ADState*)state->PlannerSpecificData);
		delete (ADState*)state->PlannerSpecificData;
		state->PlannerSpecificData = NULL;
	}
	pSearchStateSpace->searchMDP.Delete();
	environment_->StateID2IndexMapping.clear();
}



//reset properly search state space
//needs to be done before deleting states
int ADPlanner::ResetSearchStateSpace(ADSearchStateSpace_t* pSearchStateSpace)
{
	pSearchStateSpace->heap->makeemptyheap();
	pSearchStateSpace->inconslist->makeemptylist(AD_INCONS_LIST_ID);

	return 1;
}

//initialization before each search
void ADPlanner::ReInitializeSearchStateSpace(ADSearchStateSpace_t* pSearchStateSpace)
{
	CKey key;

	//increase callnumber
	pSearchStateSpace->callnumber++;

	//reset iteration
	pSearchStateSpace->iteration = 0;


#if DEBUG
    fprintf(fDeb, "reinitializing search state-space (new call number=%d search iter=%d)\n", 
            pSearchStateSpace->callnumber,pSearchStateSpace->iteration );
#endif



	pSearchStateSpace->heap->makeemptyheap();
	pSearchStateSpace->inconslist->makeemptylist(AD_INCONS_LIST_ID);

    //reset 
	pSearchStateSpace->eps = this->finitial_eps;
    pSearchStateSpace->eps_satisfied = INFINITECOST;

	//initialize start state
	ADState* startstateinfo = (ADState*)(pSearchStateSpace->searchstartstate->PlannerSpecificData);
	if(startstateinfo->callnumberaccessed != pSearchStateSpace->callnumber)
		ReInitializeSearchStateInfo(startstateinfo, pSearchStateSpace);

	startstateinfo->g = 0;

	//insert start state into the heap
	key = ComputeKey(startstateinfo);
	pSearchStateSpace->heap->insertheap(startstateinfo, key);

    pSearchStateSpace->bReinitializeSearchStateSpace = false;
	pSearchStateSpace->bReevaluatefvals = false;
}

//very first initialization
int ADPlanner::InitializeSearchStateSpace(ADSearchStateSpace_t* pSearchStateSpace)
{

	if(pSearchStateSpace->heap->currentsize != 0 || 
		pSearchStateSpace->inconslist->currentsize != 0)
	{
		printf("ERROR in InitializeSearchStateSpace: heap or list is not empty\n");
		exit(1);
	}

	pSearchStateSpace->eps = this->finitial_eps;
    pSearchStateSpace->eps_satisfied = INFINITECOST;
	pSearchStateSpace->iteration = 0;
	pSearchStateSpace->callnumber = 0;
	pSearchStateSpace->bReevaluatefvals = false;


	//create and set the search start state
	pSearchStateSpace->searchgoalstate = NULL;
	//pSearchStateSpace->searchstartstate = GetState(SearchStartStateID, pSearchStateSpace);
    pSearchStateSpace->searchstartstate = NULL;
	

    pSearchStateSpace->bReinitializeSearchStateSpace = true;

	return 1;

}


int ADPlanner::SetSearchGoalState(int SearchGoalStateID, ADSearchStateSpace_t* pSearchStateSpace)
{

	if(pSearchStateSpace->searchgoalstate == NULL || 
		pSearchStateSpace->searchgoalstate->StateID != SearchGoalStateID)
	{
		pSearchStateSpace->searchgoalstate = GetState(SearchGoalStateID, pSearchStateSpace);

		//recompute heuristic for the heap if heuristics is used
#if USE_HEUR
		int i;
		//TODO - should get rid of and instead use iteration to re-compute h-values online as needed
		for(i = 0; i < (int)pSearchStateSpace->searchMDP.StateArray.size(); i++)
		{
			CMDPSTATE* MDPstate = pSearchStateSpace->searchMDP.StateArray[i];
			ADState* state = (ADState*)MDPstate->PlannerSpecificData;
			state->h = ComputeHeuristic(MDPstate, pSearchStateSpace);
		}
#if DEBUG
		printf("re-evaluated heuristic values for %d states\n", i);
#endif
		
		pSearchStateSpace->bReevaluatefvals = true;
#endif
	}


	return 1;

}


int ADPlanner::SetSearchStartState(int SearchStartStateID, ADSearchStateSpace_t* pSearchStateSpace)
{

	pSearchStateSpace->searchstartstate = GetState(SearchStartStateID, pSearchStateSpace);

    pSearchStateSpace->bReinitializeSearchStateSpace = true;

	return 1;

}



int ADPlanner::ReconstructPath(ADSearchStateSpace_t* pSearchStateSpace)
{	

#if AD_SEARCH_FORWARD == 1 //nothing to do, if search is backward

	CMDPSTATE* MDPstate = pSearchStateSpace->searchgoalstate;
	CMDPSTATE* PredMDPstate;
	ADState *predstateinfo, *stateinfo;
		
	int steps = 0;
	const int max_steps = 10000;
	while(MDPstate != pSearchStateSpace->searchstartstate && steps < max_steps)
	{
		steps++;

		stateinfo = (ADState*)MDPstate->PlannerSpecificData;

		if(stateinfo->g == INFINITECOST)
		{	
			//printf("ERROR in ReconstructPath: g of the state on the path is INFINITE\n");
			//exit(1);
			return -1;
		}

		if(stateinfo->bestpredstate == NULL)
		{
			printf("ERROR in ReconstructPath: bestpred is NULL\n");
			exit(1);
		}

		//get the parent state
		PredMDPstate = stateinfo->bestpredstate;
		predstateinfo = (ADState*)PredMDPstate->PlannerSpecificData;

		//set its best next info
		predstateinfo->bestnextstate = MDPstate;

		//check the decrease of g-values along the path
		if(predstateinfo->v >= stateinfo->g)
		{
			printf("ERROR in ReconstructPath: g-values are non-decreasing\n");
			exit(1);
		}

		//transition back
		MDPstate = PredMDPstate;
	}

	if(MDPstate != pSearchStateSpace->searchstartstate){
		printf("ERROR: Failed to reconstruct path (compute bestnextstate pointers): steps processed=%d\n", steps);
		return 0;
	}

#endif

	return 1;
}


void ADPlanner::PrintSearchState(ADState* searchstateinfo, FILE* fOut)
{
	CKey key = ComputeKey(searchstateinfo);
	fprintf(fOut, "g=%d v=%d h = %d heapindex=%d inconslist=%d key=[%d %d]", 
			searchstateinfo->g, searchstateinfo->v, searchstateinfo->h, searchstateinfo->heapindex, (searchstateinfo->listelem[AD_INCONS_LIST_ID] != NULL),
			(int)key[0], (int)key[1]);

}

void ADPlanner::PrintSearchPath(ADSearchStateSpace_t* pSearchStateSpace, FILE* fOut)
{
	ADState* searchstateinfo;
	CMDPSTATE* state = pSearchStateSpace->searchgoalstate;
	CMDPSTATE* nextstate = NULL;

	if(fOut == NULL)
		fOut = stdout;

	int PathCost = ((ADState*)pSearchStateSpace->searchgoalstate->PlannerSpecificData)->g;

	fprintf(fOut, "Printing a path from state %d to the goal state %d\n", 
			state->StateID, pSearchStateSpace->searchstartstate->StateID);
	fprintf(fOut, "Path cost = %d:\n", PathCost);
			
	
	environment_->PrintState(state->StateID, false, fOut);

	int costFromStart = 0;
	int steps = 0;
	const int max_steps = 10000;
	while(state->StateID != pSearchStateSpace->searchstartstate->StateID && steps < max_steps)
	{
		steps++;

		fprintf(fOut, "state %d ", state->StateID);

		if(state->PlannerSpecificData == NULL)
		{
			fprintf(fOut, "path does not exist since search data does not exist\n");
			break;
		}

		searchstateinfo = (ADState*)state->PlannerSpecificData;

#if AD_SEARCH_FORWARD == 1
		nextstate = searchstateinfo->bestpredstate;
#else
		nextstate = searchstateinfo->bestnextstate;
#endif

		if(nextstate == NULL)
		{
			fprintf(fOut, "path does not exist since nextstate == NULL\n");
			break;
		}
		if(searchstateinfo->g == INFINITECOST)
		{
			fprintf(fOut, "path does not exist since state->g == NULL\n");
			break;
		}

		int costToGoal = PathCost - costFromStart;
#if AD_SEARCH_FORWARD == 0 //otherwise this cost is not even set
		costFromStart += searchstateinfo->costtobestnextstate;
#endif


#if DEBUG
		if(searchstateinfo->g > searchstateinfo->v){
			fprintf(fOut, "ERROR: underconsistent state %d is encountered\n", state->StateID);
		}

#if AD_SEARCH_FORWARD == 0 //otherwise this cost is not even set
		if(nextstate->PlannerSpecificData != NULL && searchstateinfo->g < searchstateinfo->costtobestnextstate + ((ADState*)(nextstate->PlannerSpecificData))->g)
		{
			fprintf(fOut, "ERROR: g(source) < c(source,target) + g(target)\n");
		}
#endif
#endif

		PrintSearchState(searchstateinfo, fDeb);	
		fprintf(fOut, "-->state %d ctg = %d  ", 
			nextstate->StateID, costToGoal);

		state = nextstate;

		environment_->PrintState(state->StateID, false, fOut);

	}

	if(state->StateID != pSearchStateSpace->searchstartstate->StateID){
		printf("ERROR: Failed to printsearchpath, max_steps reached\n");
		return;
	}

}

int ADPlanner::getHeurValue(ADSearchStateSpace_t* pSearchStateSpace, int StateID)
{
	CMDPSTATE* MDPstate = GetState(StateID, pSearchStateSpace);
	ADState* searchstateinfo = (ADState*)MDPstate->PlannerSpecificData;
	return searchstateinfo->h;
}


vector<int> ADPlanner::GetSearchPath(ADSearchStateSpace_t* pSearchStateSpace, int& solcost)
{
    vector<int> SuccIDV;
    vector<int> CostV;
	vector<int> wholePathIds;
	ADState* searchstateinfo;
	CMDPSTATE* state = NULL; 
	CMDPSTATE* goalstate = NULL;
	CMDPSTATE* startstate=NULL;

#if AD_SEARCH_FORWARD == 1
	startstate = pSearchStateSpace->searchstartstate;
	goalstate = pSearchStateSpace->searchgoalstate;

	//reconstruct the path by setting bestnextstate pointers appropriately
	if(ReconstructPath(pSearchStateSpace) != 1){
		solcost = INFINITECOST;
		return wholePathIds;
	}
#else
	startstate = pSearchStateSpace->searchgoalstate;
	goalstate = pSearchStateSpace->searchstartstate;

#endif


#if DEBUG
		PrintSearchPath(pSearchStateSpace, fDeb);
#endif


	state = startstate;

	wholePathIds.push_back(state->StateID);
    solcost = 0;

	FILE* fOut = stdout;
	int steps = 0;
	const int max_steps = 10000;
	while(state->StateID != goalstate->StateID && steps < max_steps)
	{
		steps++;

		if(state->PlannerSpecificData == NULL)
		{
			fprintf(fOut, "path does not exist since search data does not exist\n");
			break;
		}

		searchstateinfo = (ADState*)state->PlannerSpecificData;

		if(searchstateinfo->bestnextstate == NULL)
		{
			fprintf(fOut, "path does not exist since bestnextstate == NULL\n");
			break;
		}
		if(searchstateinfo->g == INFINITECOST)
		{
			fprintf(fOut, "path does not exist since bestnextstate == NULL\n");
			break;
		}

        environment_->GetSuccs(state->StateID, &SuccIDV, &CostV);
        int actioncost = INFINITECOST;
        for(int i = 0; i < (int)SuccIDV.size(); i++)
        {   
            if(SuccIDV.at(i) == searchstateinfo->bestnextstate->StateID)
                actioncost = CostV.at(i);

        }
        solcost += actioncost;

        //fprintf(fDeb, "actioncost=%d between states %d and %d\n", 
        //        actioncost, state->StateID, searchstateinfo->bestnextstate->StateID);
        //environment_->PrintState(state->StateID, false, fDeb);
        //environment_->PrintState(searchstateinfo->bestnextstate->StateID, false, fDeb);


		state = searchstateinfo->bestnextstate;

		wholePathIds.push_back(state->StateID);
	}

	if(state->StateID != goalstate->StateID){
		printf("ERROR: Failed to getsearchpath, steps processed=%d\n", steps);
		wholePathIds.clear();
		solcost = INFINITECOST;
		return wholePathIds;
	}

	return wholePathIds;
}



bool ADPlanner::Search(ADSearchStateSpace_t* pSearchStateSpace, vector<int>& pathIds, int & PathCost, bool bFirstSolution, bool bOptimalSolution, double MaxNumofSecs)
{
	CKey key;
	TimeStarted = clock();
    searchexpands = 0;

#if DEBUG
	fprintf(fDeb, "new search call (call number=%d)\n", pSearchStateSpace->callnumber);
#endif

    if(pSearchStateSpace->bReinitializeSearchStateSpace == true){
        //re-initialize state space 
        ReInitializeSearchStateSpace(pSearchStateSpace);
    }


	if(bOptimalSolution)
	{
		pSearchStateSpace->eps = 1;
		MaxNumofSecs = INFINITECOST;
	}
	else if(bFirstSolution)
	{
		MaxNumofSecs = INFINITECOST;
	}

	//the main loop of AD*
	int prevexpands = 0;
	while(pSearchStateSpace->eps_satisfied > AD_FINAL_EPS && 
		(clock()- TimeStarted) < MaxNumofSecs*(double)CLOCKS_PER_SEC)
	{
		//it will be a new search iteration
		pSearchStateSpace->iteration++;

		//decrease eps for all subsequent iterations
		if(fabs(pSearchStateSpace->eps_satisfied - pSearchStateSpace->eps) < ERR_EPS)
		{
			pSearchStateSpace->eps = pSearchStateSpace->eps - AD_DECREASE_EPS;
			if(pSearchStateSpace->eps < AD_FINAL_EPS)
				pSearchStateSpace->eps = AD_FINAL_EPS;


			pSearchStateSpace->bReevaluatefvals = true;

		}

		//build a new open list by merging it with incons one
		BuildNewOPENList(pSearchStateSpace);
		
		//re-compute f-values if necessary and reorder the heap
		if(pSearchStateSpace->bReevaluatefvals)
			Reevaluatefvals(pSearchStateSpace);


		//improve or compute path
		if(ComputePath(pSearchStateSpace, MaxNumofSecs) == 1){
            pSearchStateSpace->eps_satisfied = pSearchStateSpace->eps;
        }

		//print the solution cost and eps bound
		printf("eps=%f expands=%d g(sstart)=%d\n", pSearchStateSpace->eps_satisfied, searchexpands - prevexpands,
							((ADState*)pSearchStateSpace->searchgoalstate->PlannerSpecificData)->g);

#if DEBUG
        fprintf(fDeb, "eps=%f eps_sat=%f expands=%d g(sstart)=%d\n", pSearchStateSpace->eps, pSearchStateSpace->eps_satisfied, searchexpands - prevexpands,
							((ADState*)pSearchStateSpace->searchgoalstate->PlannerSpecificData)->g);
#endif
		prevexpands = searchexpands;


		//if just the first solution then we are done
		if(bFirstSolution)
			break;

		//no solution exists
		if(((ADState*)pSearchStateSpace->searchgoalstate->PlannerSpecificData)->g == INFINITECOST)
			break;

	}


#if DEBUG
	fflush(fDeb);
#endif

	PathCost = ((ADState*)pSearchStateSpace->searchgoalstate->PlannerSpecificData)->g;
	MaxMemoryCounter += environment_->StateID2IndexMapping.size()*sizeof(int);
	
	printf("MaxMemoryCounter = %d\n", MaxMemoryCounter);

	int solcost = INFINITECOST;
    bool ret = false;
	if(PathCost == INFINITECOST)
	{
		printf("could not find a solution\n");
		ret = false;
	}
	else
	{
		printf("solution is found\n");

    	pathIds = GetSearchPath(pSearchStateSpace, solcost);
        ret = true;
	}

	printf("total expands this call = %d, planning time = %.3f secs, solution cost=%d\n", 
           searchexpands, (clock()-TimeStarted)/((double)CLOCKS_PER_SEC), solcost);
    

    //fprintf(fStat, "%d %d\n", searchexpands, solcost);

	return true;

}

void ADPlanner::Update_SearchSuccs_of_ChangedEdges(vector<int>* statesIDV)
{
	printf("updating %d affected states\n", statesIDV->size());

	//it will be a new search iteration
	pSearchStateSpace_->iteration++;

	int numofstatesaffected = 0;
	for(int pind = 0; pind < (int)statesIDV->size(); pind++)
	{
		int stateID = statesIDV->at(pind);

		//first check that the state exists (to avoid creation of additional states)
		if(environment_->StateID2IndexMapping[stateID][ADMDP_STATEID2IND] == -1)
			continue;

		//now get the state
		CMDPSTATE* state = GetState(stateID, pSearchStateSpace_);
		ADState* searchstateinfo = (ADState*)state->PlannerSpecificData;

		//now check that the state is not start state and was created after last search reset
		if(stateID != pSearchStateSpace_->searchstartstate->StateID && searchstateinfo->callnumberaccessed == pSearchStateSpace_->callnumber)
		{
			//now we really do need to update it
			Recomputegval(searchstateinfo);
			UpdateSetMembership(searchstateinfo);
			numofstatesaffected++;
		}
	}

	printf("%d states really affected\n", numofstatesaffected);

	//reset eps for which we know a path was computed
	if(numofstatesaffected > 0)
	    pSearchStateSpace_->eps_satisfied = INFINITECOST;


}


//-----------------------------Interface function-----------------------------------------------------
//returns 1 if found a solution, and 0 otherwise
int ADPlanner::replan(double allocated_time_secs, vector<int>* solution_stateIDs_V)
{
    vector<int> pathIds; 
    int PathCost = 0;
    bool bFound = false;


    //plan for the first solution only
    if((bFound = Search(pSearchStateSpace_, pathIds, PathCost, false, false, allocated_time_secs)) == false) 
    {
        printf("failed to find a solution\n");
    }

    //copy the solution
    *solution_stateIDs_V = pathIds;


	return (int)bFound;

}

int ADPlanner::set_goal(int goal_stateID)
{

	//it will be a new search iteration
	pSearchStateSpace_->iteration++;

#if AD_SEARCH_FORWARD == 1

    if(SetSearchGoalState(goal_stateID, pSearchStateSpace_) != 1)
        {
            printf("ERROR: failed to set search goal state\n");
            return 0;
        }
#else

    if(SetSearchStartState(goal_stateID, pSearchStateSpace_) != 1)
        {
            printf("ERROR: failed to set search start state\n");
            return 0;
        }
#endif

    return 1;
}


int ADPlanner::set_start(int start_stateID)
{

	//it will be a new search iteration
	pSearchStateSpace_->iteration++;


#if AD_SEARCH_FORWARD == 1

    if(SetSearchStartState(start_stateID, pSearchStateSpace_) != 1)
        {
            printf("ERROR: failed to set search start state\n");
            return 0;
        }
#else

    if(SetSearchGoalState(start_stateID, pSearchStateSpace_) != 1)
        {
            printf("ERROR: failed to set search goal state\n");
            return 0;
        }
#endif

    return 1;

}


#if AD_SEARCH_FORWARD == 1
void ADPlanner::update_succs_of_changededges(vector<int>* succstatesIDV)
{
	Update_SearchSuccs_of_ChangedEdges(succstatesIDV);
}
#else
void ADPlanner::update_preds_of_changededges(vector<int>* predstatesIDV)
{
	Update_SearchSuccs_of_ChangedEdges(predstatesIDV);
}
#endif


int ADPlanner::force_planning_from_scratch()
{

    pSearchStateSpace_->bReinitializeSearchStateSpace = true;

    return 1;
}



//---------------------------------------------------------------------------------------------------------

