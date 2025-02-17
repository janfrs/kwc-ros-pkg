/*********************************************************************
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
* 
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
* 
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/
//
// Navigation function computation
// Uses Dijkstra's method
// Modified for Euclidean-distance computation
//

#include"navfn.h"


//
// create nav fn buffers 
//

NavFn::NavFn(int xs, int ys)
{  
  // create cell arrays
  obsarr = costarr = NULL;
  potarr = NULL;
  pending = NULL;
  gradx = grady = NULL;
  setNavArr(xs,ys);

  // priority buffers
  pb1 = new int[PRIORITYBUFSIZE];
  pb2 = new int[PRIORITYBUFSIZE];
  pb3 = new int[PRIORITYBUFSIZE];
  
  priInc = COST_NEUTRAL;	// <= COST_NEUTRAL is not breadth-first

  // goal and start
  goal[0] = goal[1] = 0;
  start[0] = start[1] = 0;

  // display function
  displayFn = NULL;
  displayInt = 0;

  // path buffers
  npathbuf = 0;
  pathx = pathy = NULL;
}


NavFn::~NavFn()
{
  if(obsarr)
    delete[] obsarr;
  if(costarr)
    delete[] costarr;
  if(potarr)
    delete[] potarr;
  if(pending)
    delete[] pending;
  if(gradx)
    delete[] gradx;
  if(grady)
    delete[] grady;
  if(pathx)
    delete[] pathx;
  if(pathy)
    delete[] pathy;
}


//
// set goal, start positions for the nav fn
//

void
NavFn::setGoal(int *g)
{
  goal[0] = g[0];
  goal[1] = g[1];
  printf("[NavFn] Setting goal to %d,%d\n", goal[0], goal[1]);
}

void
NavFn::setStart(int *g)
{
  start[0] = g[0];
  start[1] = g[1];
  printf("[NavFn] Setting start to %d,%d\n", start[0], start[1]);
}

//
// Set/Reset map size
//

void
NavFn::setNavArr(int xs, int ys)
{
  printf("[NavFn] Array is %d x %d\n", xs, ys);

  nx = xs;
  ny = ys;
  ns = nx*ny;

  if(obsarr)
    delete[] obsarr;
  if(costarr)
    delete[] costarr;
  if(potarr)
    delete[] potarr;
  if(pending)
    delete[] pending;

  if(gradx)
    delete[] gradx;
  if(grady)
    delete[] grady;

  obsarr = new COSTTYPE[ns];	// obstacles, 255 is obstacle
  memset(obsarr, 0, ns*sizeof(COSTTYPE));
  costarr = new COSTTYPE[ns]; // cost array, 2d config space
  memset(costarr, 0, ns*sizeof(uint16_t));
  potarr = new float[ns];	// navigation potential array
  pending = new bool[ns];
  memset(pending, 0, ns*sizeof(bool));
  gradx = new float[ns];
  grady = new float[ns];
}


void
NavFn::setObs()
{
#if 0
  // set up a simple obstacle
  printf("[NavFn] Setting simple obstacle\n");
  int xx = nx/3;
  for (int i=ny/3; i<ny; i++)
    costarr[i*nx + xx] = COST_OBS;
  xx = 2*nx/3;
  for (int i=ny/3; i<ny; i++)
    costarr[i*nx + xx] = COST_OBS;

  xx = nx/4;
  for (int i=20; i<ny-ny/3; i++)
    costarr[i*nx + xx] = COST_OBS;
  xx = nx/2;
  for (int i=20; i<ny-ny/3; i++)
    costarr[i*nx + xx] = COST_OBS;
  xx = 3*nx/4;
  for (int i=20; i<ny-ny/3; i++)
    costarr[i*nx + xx] = COST_OBS;
#endif
}


// inserting onto the priority blocks
#define push_cur(n)  { if (n>=0 && n<ns && !pending[n] && \
			   costarr[n]<COST_OBS && curPe<PRIORITYBUFSIZE) \
                         { curP[curPe++]=n; pending[n]=true; }}
#define push_next(n) { if (n>=0 && n<ns && !pending[n] && \
			   costarr[n]<COST_OBS && nextPe<PRIORITYBUFSIZE) \
                         { nextP[nextPe++]=n; pending[n]=true; }}
#define push_over(n) { if (n>=0 && n<ns && !pending[n] && \
			   costarr[n]<COST_OBS && overPe<PRIORITYBUFSIZE) \
                         { overP[overPe++]=n; pending[n]=true; }}


// Set up navigation potential arrays for new propagation

void
NavFn::setupNavFn(bool keepit)
{
  // reset values in propagation arrays
  for (int i=0; i<ns; i++)
    {
      potarr[i] = POT_HIGH;
      if (!keepit) costarr[i] = COST_NEUTRAL;
      gradx[i] = grady[i] = 0.0;
    }

  // outer bounds of cost array
  COSTTYPE *pc;
  pc = costarr;
  for (int i=0; i<nx; i++)
    *pc++ = COST_OBS;
  pc = costarr + (ny-1)*nx;
  for (int i=0; i<nx; i++)
    *pc++ = COST_OBS;
  pc = costarr;
  for (int i=0; i<ny; i++, pc+=nx)
    *pc = COST_OBS;
  pc = costarr + nx - 1;
  for (int i=0; i<ny; i++, pc+=nx)
    *pc = COST_OBS;

  // priority buffers
  curT = COST_OBS;
  curP = pb1; 
  curPe = 0;
  nextP = pb2;
  nextPe = 0;
  overP = pb3;
  overPe = 0;
  memset(pending, 0, ns*sizeof(bool));

  // set goal
  int k = goal[0] + goal[1]*nx;
  initCost(k,0);

  // find # of obstacle cells
  pc = costarr;
  int ntot = 0;
  for (int i=0; i<ns; i++, pc++)
    {
      if (*pc >= COST_OBS)
	ntot++;			// number of cells that are obstacles
    }
  nobs = ntot;
}


// initialize a goal-type cost for starting propagation

void
NavFn::initCost(int k, float v)
{
  potarr[k] = v;
  push_cur(k+1);
  push_cur(k-1);
  push_cur(k-nx);
  push_cur(k+nx);
}


// 
// Critical function: calculate updated potential value of a cell,
//   given its neighbors' values
// Planar-wave update calculation from two lowest neighbors in a 4-grid
// Quadratic approximation to the interpolated value 
// No checking of bounds here, this function should be fast
//

#define INVSQRT2 0.707106781

inline void
NavFn::updateCell(int n)
{
  // get neighbors
  float u,d,l,r;
  l = potarr[n-1];
  r = potarr[n+1];		
  u = potarr[n-nx];
  d = potarr[n+nx];
  //  printf("[Update] c: %0.1f  l: %0.1f  r: %0.1f  u: %0.1f  d: %0.1f\n", 
  //	 potarr[n], l, r, u, d);
  //  printf("[Update] cost: %d\n", costarr[n]);

  // find lowest, and its lowest neighbor
  float ta, tc;
  if (l<r) tc=l; else tc=r;
  if (u<d) ta=u; else ta=d;

  // do planar wave update
  if (costarr[n] < COST_OBS)	// don't propagate into obstacles
    {
      float hf = (float)costarr[n]; // traversability factor
      float dc = tc-ta;		// relative cost between ta,tc
      if (dc < 0) 		// ta is lowest
	{
	  dc = -dc;
	  ta = tc;
	}

      // calculate new potential
      float pot;
      if (dc >= hf)		// if too large, use ta-only update
	pot = ta+hf;
      else			// two-neighbor interpolation update
	{
	  // use quadratic approximation
	  // might speed this up through table lookup, but still have to 
	  //   do the divide
	  float d = dc/hf;
	  float v = -0.2301*d*d + 0.5307*d + 0.7040;
	  pot = ta + hf*v;
	}

      //      printf("[Update] new pot: %d\n", costarr[n]);

      // now add affected neighbors to priority blocks
      if (pot < potarr[n])
	{
	  float le = INVSQRT2*(float)costarr[n-1];
	  float re = INVSQRT2*(float)costarr[n+1];
	  float ue = INVSQRT2*(float)costarr[n-nx];
	  float de = INVSQRT2*(float)costarr[n+nx];
	  potarr[n] = pot;
	  if (pot < curT)	// low-cost buffer block 
	    {
	      if (l > pot+le) push_next(n-1);
	      if (r > pot+re) push_next(n+1);
	      if (u > pot+ue) push_next(n-nx);
	      if (d > pot+de) push_next(n+nx);
	    }
	  else			// overflow block
	    {
	      if (l > pot+le) push_over(n-1);
	      if (r > pot+re) push_over(n+1);
	      if (u > pot+ue) push_over(n-nx);
	      if (d > pot+de) push_over(n+nx);
	    }
	}

    }

}


//
// Use A* method for setting priorities
// Critical function: calculate updated potential value of a cell,
//   given its neighbors' values
// Planar-wave update calculation from two lowest neighbors in a 4-grid
// Quadratic approximation to the interpolated value 
// No checking of bounds here, this function should be fast
//

#define INVSQRT2 0.707106781

inline void
NavFn::updateCellAstar(int n)
{
  // get neighbors
  float u,d,l,r;
  l = potarr[n-1];
  r = potarr[n+1];		
  u = potarr[n-nx];
  d = potarr[n+nx];
  //  printf("[Update] c: %0.1f  l: %0.1f  r: %0.1f  u: %0.1f  d: %0.1f\n", 
  //	 potarr[n], l, r, u, d);
  //  printf("[Update] cost: %d\n", costarr[n]);

  // find lowest, and its lowest neighbor
  float ta, tc;
  if (l<r) tc=l; else tc=r;
  if (u<d) ta=u; else ta=d;

  // do planar wave update
  if (costarr[n] < COST_OBS)	// don't propagate into obstacles
    {
      float hf = (float)costarr[n]; // traversability factor
      float dc = tc-ta;		// relative cost between ta,tc
      if (dc < 0) 		// ta is lowest
	{
	  dc = -dc;
	  ta = tc;
	}

      // calculate new potential
      float pot;
      if (dc >= hf)		// if too large, use ta-only update
	pot = ta+hf;
      else			// two-neighbor interpolation update
	{
	  // use quadratic approximation
	  // might speed this up through table lookup, but still have to 
	  //   do the divide
	  float d = dc/hf;
	  float v = -0.2301*d*d + 0.5307*d + 0.7040;
	  pot = ta + hf*v;
	}

      //      printf("[Update] new pot: %d\n", costarr[n]);

      // now add affected neighbors to priority blocks
      if (pot < potarr[n])
	{
	  float le = INVSQRT2*(float)costarr[n-1];
	  float re = INVSQRT2*(float)costarr[n+1];
	  float ue = INVSQRT2*(float)costarr[n-nx];
	  float de = INVSQRT2*(float)costarr[n+nx];

	  // calculate distance
	  int x = n%nx;
	  int y = n/nx;
	  float dist = (x-start[0])*(x-start[0]) + (y-start[1])*(y-start[1]);
	  dist = sqrtf(dist)*(float)COST_NEUTRAL;

	  potarr[n] = pot;
	  pot += dist;
	  if (pot < curT)	// low-cost buffer block 
	    {
	      if (l > pot+le) push_next(n-1);
	      if (r > pot+re) push_next(n+1);
	      if (u > pot+ue) push_next(n-nx);
	      if (d > pot+de) push_next(n+nx);
	    }
	  else
	    {
	      if (l > pot+le) push_over(n-1);
	      if (r > pot+re) push_over(n+1);
	      if (u > pot+ue) push_over(n-nx);
	      if (d > pot+de) push_over(n+nx);
	    }
	}

    }

}



//
// main propagation function
// Dijkstra method, breadth-first
// runs for a specified number of cycles,
//   or until it runs out of cells to update,
//   or until the Start cell is found (atStart = true)
//

bool
NavFn::propNavFnDijkstra(int cycles, bool atStart)	
{
  int nwv = 0;			// max priority block size
  int nc = 0;			// number of cells put into priority blocks
  int cycle = 0;		// which cycle we're on

  // set up start cell
  int startCell = start[1]*nx + start[0];

  for (; cycle < cycles; cycle++) // go for this many cycles, unless interrupted
    {
      // 
      if (curPe == 0 && nextPe == 0) // priority blocks empty
	break;

      // stats
      nc += curPe;
      if (curPe > nwv)
	nwv = curPe;

      // reset pending flags on current priority buffer
      int *pb = curP;
      int i = curPe;			
      while (i-- > 0)		
        pending[*(pb++)] = false;
		
      // process current priority buffer
      pb = curP; 
      i = curPe;
      while (i-- > 0)		
	updateCell(*pb++);

      if (displayInt > 0 &&  (cycle % displayInt) == 0)
	displayFn(this);

      // swap priority blocks curP <=> nextP
      curPe = nextPe;
      nextPe = 0;
      pb = curP;		// swap buffers
      curP = nextP;
      nextP = pb;

      // see if we're done with this priority level
      if (curPe == 0)
        {
          curT += priInc;	// increment priority threshold
	  curPe = overPe;	// set current to overflow block
	  overPe = 0;
          pb = curP;		// swap buffers
          curP = overP;
          overP = pb;
        }

      // check if we've hit the Start cell
      if (atStart)
	if (potarr[startCell] < POT_HIGH)
	  break;
    }

  printf("[NavFn] Used %d cycles, %d cells visited (%d%%), priority buf max %d\n", 
	       cycle,nc,(int)((nc*100.0)/(ns-nobs)),nwv);

  if (cycle < cycles) return true; // finished up here
  else return false;
}


//
// main propagation function
// A* method, best-first
// uses Euclidean distance heuristic
// runs for a specified number of cycles,
//   or until it runs out of cells to update,
//   or until the Start cell is found (atStart = true)
//

bool
NavFn::propNavFnAstar(int cycles)	
{
  int nwv = 0;			// max priority block size
  int nc = 0;			// number of cells put into priority blocks
  int cycle = 0;		// which cycle we're on

  // set initial threshold, based on distance
  float dist = (goal[0]-start[0])*(goal[0]-start[0]) + (goal[1]-start[1])*(goal[1]-start[1]);
  dist = sqrtf(dist)*(float)COST_NEUTRAL;
  curT = dist + curT;

  // set up start cell
  int startCell = start[1]*nx + start[0];

  // do main cycle
  for (; cycle < cycles; cycle++) // go for this many cycles, unless interrupted
    {
      // 
      if (curPe == 0 && nextPe == 0) // priority blocks empty
	break;

      // stats
      nc += curPe;
      if (curPe > nwv)
	nwv = curPe;

      // reset pending flags on current priority buffer
      int *pb = curP;
      int i = curPe;			
      while (i-- > 0)		
        pending[*(pb++)] = false;
		
      // process current priority buffer
      pb = curP; 
      i = curPe;
      while (i-- > 0)		
	updateCellAstar(*pb++);

      if (displayInt > 0 &&  (cycle % displayInt) == 0)
	displayFn(this);

      // swap priority blocks curP <=> nextP
      curPe = nextPe;
      nextPe = 0;
      pb = curP;		// swap buffers
      curP = nextP;
      nextP = pb;

      // see if we're done with this priority level
      if (curPe == 0)
        {
          curT += priInc;	// increment priority threshold
	  curPe = overPe;	// set current to overflow block
	  overPe = 0;
          pb = curP;		// swap buffers
          curP = overP;
          overP = pb;
        }

      // check if we've hit the Start cell
      if (potarr[startCell] < POT_HIGH)
	break;

    }

  printf("[NavFn] Used %d cycles, %d cells visited (%d%%), priority buf max %d\n", 
	       cycle,nc,(int)((nc*100.0)/(ns-nobs)),nwv);

  if (potarr[startCell] < POT_HIGH) return true; // finished up here
  else return false;
}


//
// Path construction
// Find gradient at array points, interpolate path
// Use step size of one pixel - should we use 1/2 pixel?
//

bool
NavFn::calcPath(int n, int *st)
{
  // check path arrays
  if (npathbuf < n)
    {
      if (pathx) delete [] pathx;
      if (pathy) delete [] pathy;
      pathx = new float[n];
      pathy = new float[n];
      npathbuf = n;
    }
  
  // set up start position at cell
  // st is always upper left corner for 4-point bilinear interpolation 
  if (st == NULL) st = start;
  int stc = st[1]*nx + st[0];

  // set up offset
  float dx=0;
  float dy=0;
  npath = 0;

  // go for <n> cycles at most
  for (int i=0; i<n; i++)
    {
      // check if near goal
      if (potarr[stc] < COST_OBS)
	return true;		// done!

      if (stc < nx || stc > ns-nx) // would be out of bounds
	return false;

      // add to path
      pathx[npath] = stc%nx + dx;
      pathy[npath] = stc/nx + dy;
      npath++;

      // get grad at four positions near cell
      int stcnx = stc+nx;
      gradCell(stc);
      gradCell(stc+1);
      gradCell(stcnx);
      gradCell(stcnx+1);
      
      // get interpolated gradient
      float x1 = (1.0-dx)*gradx[stc] + dx*gradx[stc+1];
      float x2 = (1.0-dx)*gradx[stcnx] + dx*gradx[stcnx+1];
      float x = (1.0-dy)*x1 + dy*x2; // interpolated x
      float y1 = (1.0-dx)*grady[stc] + dx*grady[stc+1];
      float y2 = (1.0-dx)*grady[stcnx] + dx*grady[stcnx+1];
      float y = (1.0-dy)*y1 + dy*y2; // interpolated y

      // check for zero gradient, failed
      if (x == 0.0 && y == 0.0)
	return false;

      // move in the right direction
      dx += x;
      dy += y;

      // check for overflow
      if (dx > 1.0) { stc++; dx -= 1.0; }
      if (dx < -1.0) { stc--; dx += 1.0; }
      if (dy > 1.0) { stc+=nx; dy -= 1.0; }
      if (dy < -1.0) { stc-=nx; dy += 1.0; }

      //      printf("[Path] Pot: %0.1f  grad: %0.1f,%0.1f  pos: %0.1f,%0.1f\n",
      //	     potarr[stc], x, y, pathx[npath-1], pathy[npath-1]);
    }

  return true;			// out of cycles
}


//
// gradient calculations
//

// calculate gradient at a cell
// positive value are to the right and down
void				
NavFn::gradCell(int n)
{
  if (gradx[n]+grady[n] > 0.0)	// check this cell
    return;			

  if (n < nx || n > ns-nx)	// would be out of bounds
    return;

  float cv = potarr[n];
  if (cv >= POT_HIGH) return;	// can't work in obstacles

  // dx calc, average to sides
  float dx = 0.0;
  if (potarr[n-1] < POT_HIGH)
    dx += potarr[n-1]- cv;	
  if (potarr[n+1] < POT_HIGH)
    dx += cv - potarr[n+1]; 

  // dy calc, average to sides
  float dy = 0.0;
  if (potarr[n-nx] < POT_HIGH)
    dy += potarr[n-nx]- cv;	
  if (potarr[n+nx] < POT_HIGH)
    dy += cv - potarr[n+nx]; 

  // normalize
  float norm = sqrtf(dx*dx+dy*dy);
  if (norm > 0)
    {
      norm = 1.0/norm;
      gradx[n] = norm*dx;
      grady[n] = norm*dy;
    }
}


//
// display function setup
// <n> is the number of cycles to wait before displaying,
//     use 0 to turn it off

void
NavFn::display(void fn(NavFn *nav), int n)
{
  displayFn = fn;
  displayInt = n;
}
