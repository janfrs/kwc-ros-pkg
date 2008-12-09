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
#ifndef __UTILS_H_
#define __UTILS_H_

#ifndef WIN32
#define __max(x,y) (x>y?x:y)
#define __min(x,y) (x>y?y:x)
#endif

#define NORMALIZEDISCTHETA(THETA, THETADIRS) (((THETA>=0)?((THETA)%(THETADIRS)):(((THETA)%(THETADIRS)+THETADIRS)%THETADIRS)))

#define CONTXY2DISC(X, CELLSIZE) (((X)>=0)?((int)((X)/(CELLSIZE))):((int)((X)/(CELLSIZE))-1))
#define DISCXY2CONT(X, CELLSIZE) ((X)*(CELLSIZE) + (CELLSIZE)/2.0)


#define PI_CONST 3.141592653

#define UNKNOWN_COST 1000000

typedef struct {
//   int X1, Y1, Z1;
//   int X2, Y2, Z2;
//   int Increment;
//   int UsingYIndex;
//   int DeltaX, DeltaY, DeltaZ;
//   int DTerm;
//   int IncrE, IncrNE;
//   int XIndex, YIndex, ZIndex;
//   int Flipped;
  
  int X1, Y1, Z1;
  int X2, Y2, Z2;
  int XIndex, YIndex, ZIndex;
  int UsingXYZIndex;
  int IncX, IncY, IncZ;
  int dx, dy, dz;
  int dx2, dy2, dz2;
  int err1, err2;
} bresenham_param_t;




//function prototypes
#if MEM_CHECK == 1
void DisableMemCheck();
void EnableMemCheck();
#endif
void CheckMDP(CMDP* mdp);
void PrintMatrix(int** matrix, int rows, int cols, FILE* fOut);
void EvaluatePolicy(CMDP* PolicyMDP, int StartStateID, int GoalStateID,
					double* PolValue, bool *bFullPolicy, double *Pcgoal, 
					int* nMerges, bool *bCycles);
int ComputeNumofStochasticActions(CMDP* pMDP);

void get_bresenham_parameters(int p1x, int p1y, int p1z, int p2x, int p2y, int p2z, bresenham_param_t *params);
void get_current_point(bresenham_param_t *params, int *x, int *y, int *z);
int get_next_point(bresenham_param_t *params);

//converts discretized version of angle into continuous (radians)
//maps 0->0, 1->delta, 2->2*delta, ...
double DiscTheta2Cont(int nTheta, int NUMOFANGLEVALS);
//converts continuous (radians) version of angle into discrete
//maps 0->0, [delta/2, 3/2*delta)->1, [3/2*delta, 5/2*delta)->2,...
int ContTheta2Disc(double fTheta, int NUMOFANGLEVALS);

//input angle should be in radians
//counterclockwise is positive
//output is an angle in the range of from 0 to 2*PI
double normalizeAngle(double angle);

#if 0
void CheckSearchMDP(CMDP* mdp, int ExcludeSuccStateID = -1);
void CheckSearchPredSucc(CMDPSTATE* state, int ExcludeSuccStateID = -1);
#endif

#endif
