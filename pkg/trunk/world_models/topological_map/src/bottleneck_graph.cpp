/*
 * Copyright (c) 2008, Willow Garage, Inc.
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
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
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



// Generalities about the code:
// 1) Often uses the fact that in boost graph adjacency list graphs with nodes stored in a vector,
// the vertex descriptors are nonnegative integers (so it would all break if a different storage 
// method was used)
// 2) Runs really slow if compiled without optimization


#include <topological_map/bottleneck_graph.h>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <sysexits.h>
#include <boost/graph/breadth_first_search.hpp>     
#include <boost/graph/connected_components.hpp>     
#include <rosconsole/rosconsole.h>

using boost::get;
using boost::tie;
using namespace std;

namespace topological_map 
{

// Typedefs for graphs
struct coords_t { typedef boost::vertex_property_tag kind; };
typedef boost::property<coords_t,Coords> coords_property; 

struct bottleneck_t { typedef boost::vertex_property_tag kind; };
typedef boost::property<bottleneck_t,bool,coords_property> bottleneck_property;

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, bottleneck_property> Graph; 
typedef boost::property_map<Graph, coords_t>::type CoordsMap; // The property map for coordinates
typedef boost::property_map<Graph, bottleneck_t>::type BottleneckMap; // And for whether or not a cell is a bottleneck
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;
typedef boost::graph_traits<Graph>::vertex_iterator vertex_iter;
typedef boost::graph_traits<Graph>::out_edge_iterator edge_iter;
typedef map<Vertex,int> VertexCompMap;

// Represents a square block from (r,c) to (r+s-1,c+s-1)
struct Block 
{
  int r;
  int c;
  int s;
  Block (int rInit, int cInit, int sInit) : r(rInit), c(cInit), s(sInit) {}
};
typedef list<Block> BlockList;

// Struct containing a graph over the vertices of a 2d grid together with a 2d array allowing vertices to be looked up quickly
typedef boost::multi_array<Vertex, 2> VertexMap;
typedef boost::multi_array<bool, 2> OccMap;
struct GridGraph 
{
  Graph g;
  VertexMap m;
  OccMap o;

  GridGraph (const grid_size* dims) : m(boost::extents[dims[0]][dims[1]]), o(boost::extents[dims[0]][dims[1]]) {}
};

// Forward declarations
void indexRegions (IndexedBottleneckGraph* g);




/************************************************************
 * We want to have a breadth-first search that terminates
 * when a goal vertex is reached, or a distance threshold is
 * exceeded.  This is done by adding a visitor to the 
 * boost bfs algorithm that throws an exception when one
 * of these things happens
 ************************************************************/

struct TerminateBfs
{
  bool found;
  TerminateBfs (bool f) : found(f) {}
};
class BfsVisitor : public boost::default_bfs_visitor {
public:
  void discover_vertex (Vertex u, const Graph& g) const
  {
    int r, c;
    tie (r, c) = get (coords_t(), g, u);
    if ((r == rg_) && (c == cg_))
      throw TerminateBfs (true);
    else if (abs(r-r0_) + abs(c-c0_) > threshold_)
      throw TerminateBfs (false);
  }

  BfsVisitor (int r0, int c0, int threshold, int rg, int cg)
  {
    r0_ = r0;
    c0_ = c0;
    rg_ = rg;
    cg_ = cg;
    threshold_ = threshold;
  }

private:
  int threshold_, r0_, c0_, rg_, cg_;
};

// Return true iff l1 distance from r0, c0 to r1, c1 is < threshold
bool distLessThan (GridGraph* gr, int r0, int c0, int r1, int c1, int threshold)
{
  BfsVisitor vis (r0, c0, threshold, r1, c1);

  try {
    boost::breadth_first_search (gr->g, gr->m[r0][c0], boost::visitor(vis));
  }
  
  // Breadth first search can either throw an exception with found = true,
  // with found=false, or terminate naturally.  In the latter two cases, the
  // distance is above the threshold.
  catch (TerminateBfs e)
  {
    if (e.found) 
      return true;
  }
  return false;
}





/****************************************
 * Printing, reading
 ****************************************/


void printGraph (const Graph& g) 
{
  vertex_iter vi, vg;
  edge_iter ei, eg;
  coords_t coords_key;
  
  cout << endl;
  for (tie(vi,vg) = boost::vertices(g); vi != vg; vi++)
  {
    int r, c;
    tie (r, c) = get (coords_key, g, *vi);
    cout << r << "," << c << " ";
    for (tie(ei, eg) = boost::out_edges (*vi, g); ei != eg; ei++) {
      int r2, c2;
      tie (r2, c2) = get (coords_key, g, boost::target (*ei, g));
      cout << r2 << "," << c2 << " ";
    }
    cout << endl;
  }
}

void printVertexMap (const GridGraph& gr)
{
  const grid_size* dims = gr.m.shape();  
  for (grid_size i=0; i<dims[0]; i++) {
    for (grid_size j=0; j<dims[1]; j++) {
      cout << setw(2) << gr.m[i][j] << " ";
    }
    cout << endl;
  }
}


void IndexedBottleneckGraph::printBottleneckGraph (void)
{
  BottleneckVertexIterator i, end;
  for (tie(i, end) = boost::vertices(graph); i!=end; ++i) {
    BottleneckVertex v = *i;
    VertexDescription d = get (desc_t(), graph, v);
    cout << "Vertex: " << endl << " Id: " << d.id << endl << " Type: ";
    if (d.type == BOTTLENECK)
      cout << "bottleneck" << endl << " Region: ";
    else
      cout << "open" << endl << " Region: ";
    
    int rmax = 0;
    int rmin = 10000000;
    int cmax = 0;
    int cmin = 10000000;


    if (d.region.size () > 20) {
      for (Region::iterator i = d.region.begin(); i!=d.region.end(); i++) {
        int r, c;
        tie (r, c) = *i;

        rmin = min (rmin, r);
        rmax = max (rmax, r);
        cmin = min (cmin, c);
        cmax = max (cmax, c);
      }

    
      cout << d.region.size() << " cells contained in bounding box between (" << rmin << "," << cmin << ") and (" << rmax << "," << cmax << ")" << endl << endl;
    }
    else {
      for (Region::iterator i = d.region.begin(); i!=d.region.end(); i++) {
        int r, c;
        tie (r,c) = *i;
        cout << "(" << r << ", " << c << ") ";
      }
      cout << endl;
    }

    cout << " Neighbors: ";
    
    BottleneckAdjacencyIterator it, end;
    for (tie(it,end) = boost::adjacent_vertices(v, graph); it!=end; it++) {
      cout << get(desc_t(), graph, *it).id << " ";
    }
    cout << endl;
  }
}


void IndexedBottleneckGraph::printBottlenecks (void)
{
  writeToStream (cout);
}

void IndexedBottleneckGraph::printBottlenecks (const char* filename)
{
  ofstream str(filename);
  if (!str) {
    ROS_WARN ("Could not open file %s for writing", filename);
  }
  else {
    writeToStream (str);
    str.close();
  }
}




void IndexedBottleneckGraph::writeToStream (ostream& str)
{
 
  BottleneckVertexIterator i, end;
  
  str << numRows << " " << numCols << endl;
  str << boost::num_vertices(graph) << endl;
  for (tie(i, end) = boost::vertices(graph); i!=end; ++i) {
    BottleneckVertex v = *i;
    VertexDescription d = get (desc_t(), graph, v);
    str << d.id << endl;
    if (d.type == BOTTLENECK)
      str << "Bottleneck" << endl;
    else
      str << "Open" << endl;

    str << d.region.size() << endl;
    for (Region::iterator i = d.region.begin(); i!=d.region.end(); i++) {
      int r, c;
      tie (r, c) = *i;
      str << r << " " << c << endl;
    }
  }
}

    
           

void printBlock (const GridGraph& g, int r, int c, int s, int r0, int c0, int r1, int c1, int nr, int nc)
{
  int rmin = max(min(r0,r),0);
  int rmax = min(max(r1,r+s),nr-1);
  int cmin = max(min(c0,c),0);
  int cmax = min(max(c1,c+s),nc-1);

  for (int i=rmin; i<=rmax; i++) {
    for (int j=cmin; j<=cmax; j++) {
      if ((i==r0) && (j==c0))
        cout << "S";
      else if ((i==r1) && (j==c1)) 
        cout << "G";
      else if (g.o[i][j])
        if ((i<r-1) || (i > r+s) || (j<c-1) || (j>c+s))
          cout << ".";
        else
          cout << " ";
      else
        if ((i<r-1) || (i > r+s) || (j<c-1) || (j>c+s))
          cout << "Y";
        else
          cout << "X";
    }
    cout << endl;
  }
}


IndexedBottleneckGraph readBottleneckGraphFromFile (const char* filename)
{
  ifstream str(filename);

  if (!str) {
    ROS_FATAL ("Unable to open file %s", filename);
    exit(EX_NOINPUT);
  }

  int numRows, numCols, numVertices;
  str >> numRows >> numCols >> numVertices;
  ROS_DEBUG ("About to read graph with %d vertices.  Grid is %dx%d.", numVertices, numRows, numCols);

  IndexedBottleneckGraph g(numRows, numCols);
  char buf[100];

  // Skip newline
  str.getline(buf,99);

  for (int i=0; i<numVertices; i++) {
    VertexDescription v;
    int numCells;
    str.getline(buf,99);
    v.id = atoi(buf);
    str.getline(buf, 99);
    switch (buf[0]) {
    case 'B': v.type = BOTTLENECK; break;
    case 'O': v.type = OPEN; break;
    default: ROS_FATAL ("Unable to parse file %s.  Unexpected line %s", filename, buf); exit(EX_DATAERR);
    }
    str.getline(buf,99);
    numCells = atoi(buf);
    for (int j=0; j<numCells; j++) {
      Coords c;
      str >> c.first;
      str >> c.second;
      str.getline(buf,99);
      v.region.insert (c);
    }


    BottleneckVertex vertex = boost::add_vertex(g.graph);
    boost::put (desc_t(), g.graph, vertex, v);
  }
  indexRegions (&g);

  // Add edges
  set<Coords > addedEdges; // Abuse of the Coords type 
  for (int r=0; r<numRows; r++) {
    for (int c=0; c<numCols; c++) {
      int id=g.regionId(r,c);
      if (id<0) {
        continue;
      }
      int idAbove=g.regionId(r-1,c);
      int idLeft=g.regionId(r,c-1);
      
      if ((idAbove>=0) && (id!=idAbove) && (addedEdges.count (Coords(min(id,idAbove), max(id,idAbove)))==0)) {
        addedEdges.insert (Coords(min(id,idAbove), max(id,idAbove)));
        add_edge ((*g.regions)[r][c], (*g.regions)[r-1][c], g.graph);
      }
      if ((idLeft>=0) && (id!=idLeft) && (addedEdges.count (Coords(min(id,idLeft), max(id,idLeft)))==0)) {
        addedEdges.insert (Coords(min(id,idLeft), max(id,idLeft)));
        add_edge ((*g.regions)[r][c], (*g.regions)[r][c-1], g.graph);
      }
    }
  }

  ROS_DEBUG ("Finished reading bottleneck graph");

  g.printBottleneckGraph();

  return g;
}


/************************************************************
 * Miscellaneous
 ************************************************************/

// Each time called, return a different positive number
int getUniqueId ()
{
  static int id=1;
  return id++;
}




/*****************************************
 * Low-level operations on grid graph
 *****************************************/


// If the given vertices both exist in the graph, remove edge between them
void possiblyRemove (GridGraph* gr, grid_size r, grid_size c, grid_size r2, grid_size c2)
{
  if (gr->o[r][c] && gr->o[r2][c2])
    remove_edge (gr->m[r][c], gr->m[r2][c2], gr->g);
}


// If the given vertices both exist in the graph, add an edge between them
void possiblyAdd (GridGraph* gr, grid_size r, grid_size c, grid_size r2, grid_size c2)
{
  if (gr->o[r][c] && gr->o[r2][c2])
    add_edge (gr->m[r][c], gr->m[r2][c2], gr->g);
}
    


// Disconnect the square of size s with top-left corner r0, c0 from the rest of the graph
void removeBlock (GridGraph* gr, grid_size r0, grid_size c0, int s)
{
  const grid_size* dims = gr->m.shape();
  for (int i=0; i<s; i++) {
    if (r0>0)
      possiblyRemove (gr, r0, c0+i, r0-1, c0+i);
    if (c0>0)
      possiblyRemove (gr, r0+i, c0, r0+i, c0-1);
    if (r0<dims[0]-s)
      possiblyRemove (gr, r0+s-1, c0+i, r0+s, c0+i);
    if (c0<dims[1]-s)
      possiblyRemove (gr, r0+i, c0+s-1, r0+i, c0+s);
  }

}

// Reconnect the square of size s with top-left corner r0, c0 to the rest of the graph
void addBlock (GridGraph* gr, grid_size r0, grid_size c0, int s)
{
  const grid_size* dims = gr->m.shape();
  for (int i=0; i<s; i++) {
    if (r0>0)
      possiblyAdd (gr, r0, c0+i, r0-1, c0+i);
    if (c0>0)
      possiblyAdd (gr, r0+i, c0, r0+i, c0-1);
    if (r0<dims[0]-s)
      possiblyAdd (gr, r0+s-1, c0+i, r0+s, c0+i);
    if (c0<dims[1]-s)
      possiblyAdd (gr, r0+i, c0+s-1, r0+i, c0+s);
  }

}


// look for a nonobstacle cell within the grid, within s/2 of (r,c)
// If found, return true, and set r,c to the new cell.  Else return false.
bool getFreePointNear (int& r, int& c, const int s, const grid_size* dims, const GridGraph* gr) {
  
  bool foundPoint = false;

  int rMin = r-s/2;
  int rMax = r+s/2;
  int cMin = c-s/2;
  int cMax = c+s/2;
  
  for (r=rMin; (r<=rMax) && !foundPoint; r++) {
    for (c=cMin; (c<=cMax) && !foundPoint; c++) {
      if ((r>=0) && (r<(int)dims[0]) && (c>=0) && (c<(int)dims[1]) && gr->o[r][c])
        foundPoint = true;
    }
  }
  r--;
  c--;
  return foundPoint;
}



// Mark as being bottlenecks the cells in the square of size s with top-left corner r0, c0 
void markBottleneckCells (GridGraph* gr, grid_size r0, grid_size c0, int s)
{
  BottleneckMap bottleneckCells = get (bottleneck_t(), gr->g);
  for (grid_size r=r0; r<r0+s; r++) {
    for (grid_size c=c0; c<c0+s; c++) {
      if (gr->o[r][c]) {
        boost::put (bottleneckCells, gr->m[r][c], true);
      }
    }
  }
}


// Sever connections between bottleneck and non-bottleneck cells
void disconnectBottlenecks (GridGraph* gr)
{
  const grid_size* dims = gr->m.shape();
  BottleneckMap bottlenecks = get (bottleneck_t(), gr->g);
  for (grid_size r=0; r<dims[0]; r++) {
    for (grid_size c=0; c<dims[1]; c++) {
      if (gr->o[r][c]) {
        Vertex v1 = gr->m[r][c];
        if (r>0) {
          if (gr->o[r-1][c]) {
            Vertex v2 = gr->m[r-1][c];
            if (get (bottlenecks, v1) != get (bottlenecks, v2)) {
              boost::remove_edge(v1, v2, gr->g);
            }
          }
        }
        if (c>0) {
          if (gr->o[r][c-1]) {
            Vertex v2 = gr->m[r][c-1];
            if (get (bottlenecks, v1) != get (bottlenecks, v2)) {
              boost::remove_edge(v1, v2, gr->g);
            }
          }
        }
      }
    }
  }
}


// Suppose we know that a block with corner (r,c) disconnects (r0,c0) and (r1,c1).
// This function searches for a smaller block that also disconnects them.
// It should be more efficient to do it this way than to search for small blocks from the beginning.
void addSmallestDisconnectingBlocks (GridGraph* gr, BlockList* disconnectingBlocks, int r, int c, int r0, int c0, int r1, int c1, int threshold, int size)
{
  bool someChildDisconnects=false;
  ROS_DEBUG_NAMED ("bottleneck_finder","Looking for smallest disconnecting subblocks of block from (%d, %d) to (%d, %d)", r, c, r+size-1, c+size-1);
  if (size > 2) {
    int blockR=-1;
    int blockC=-1;
    int blockSize = size*2/3;
    for (int i=0; i<2; i++) {
      for (int j=0; j<2; j++) {
        bool disconnected;
        blockR=i*r+(1-i)*(r+size-blockSize);
        blockC=j*c+(1-j)*(c+size-blockSize);
        
        removeBlock (gr, blockR, blockC, blockSize);
        disconnected = !distLessThan(gr, r0, c0, r1, c1, threshold);
        addBlock (gr, blockR, blockC, blockSize);
        if (disconnected) {
          someChildDisconnects=true;
          addSmallestDisconnectingBlocks (gr, disconnectingBlocks, blockR, blockC, r0, c0, r1, c1, threshold, blockSize);
        }
      }
    }
  }
  if ((size<=2) || !someChildDisconnects) {
    ROS_DEBUG_NAMED ("bottleneck_finder","Adding disconnecting block at %d, %d of size %d", r, c, size);
    disconnectingBlocks->push_back(Block(r,c,size));
  }
}



/****************************************
 * Low-level ops on bottleneck graph
 ****************************************/


// Given two vertices in the occupancy grid, if they correspond to different connected components, add an edge between those components in the bottleneck graph 
void possiblyAddBottleneckEdge (Vertex v1, Vertex v2, const VertexCompMap& vertexComp, const vector<BottleneckVertex>& vertices, BottleneckGraph* g)
{
  int r1 = vertexComp.find(v1)->second;
  int r2 = vertexComp.find(v2)->second;
  if (r1 != r2)
    add_edge (vertices[vertexComp.find(v1)->second], vertices[vertexComp.find(v2)->second], *g);
}
  


// Find adjacent connected components.  Any such adjacent pair must consist of a
// bottleneck region and a nonbottleneck region.
void connectRegions (const GridGraph& gr, const VertexCompMap& vertexComp, const vector<BottleneckVertex>& bottleneckVertices, BottleneckGraph* g)
{
  const grid_size* dims = gr.m.shape();
  for (grid_size r=0; r<dims[0]; r++) {
    for (grid_size c=0; c<dims[1]; c++) {
      if (gr.o[r][c]) {
        Vertex v1 = gr.m[r][c];
        
        // Upward edge
        if (r>0) {
          if (gr.o[r-1][c]) {
            Vertex v2 = gr.m[r-1][c];
            possiblyAddBottleneckEdge (v1, v2, vertexComp, bottleneckVertices, g);
          }
        }

        // Leftward edge
        if (c>0) {
          if (gr.o[r][c-1]) {
            Vertex v2 = gr.m[r][c-1];
            possiblyAddBottleneckEdge (v1, v2, vertexComp, bottleneckVertices, g);
          }
        }
      }
    }
  }
}

  

void removeBottleneck (BottleneckVertex v, IndexedBottleneckGraph* g)
{
  // First, figure out the overall union region r of v and its neighbors
  
  // Add a new vertex with region r, type Open

  // For each neighbor n of v
  // Connect newVertex to each neighbor of n except v
  
  // Remove v and its neighbors from graph (be mindful of iterator/descriptor stability)
}


void pruneIsolatedBottlenecks (const int regionSizeThreshold, IndexedBottleneckGraph* g)
{
  bool prunedBottleneck = false;
  do {
    // For each node of g

    // If not bottleneck continue

    // For each neighbor node
    // Count number of cells in node's region.  If exceeds threshold, increment numLargeNeighbors.
    // End for

    // If numLargeNeighbors < 2
    // removeBottleneck (g, n) and prunedBottleneck = true

  } while (prunedBottleneck);
}


// Set up the index from cell coordinates to region id, and the index for occupiedness
void indexRegions (IndexedBottleneckGraph* g)
{
  BottleneckVertexIterator i, end;
  for (tie(i, end) = boost::vertices(g->graph); i!=end; ++i) {
    BottleneckVertex v=*i;
    VertexDescription d = get(desc_t(), g->graph, v);
    for (Region::iterator i = d.region.begin(); i!=d.region.end(); i++) {
      int r, c;
      tie (r, c) = *i;
      (*g->regions)[r][c] = v;
      (*g->isFree)[r][c] = true;
    }
  }
}




/************************************************************
 * Top level
 ************************************************************/


// GridGraph constructor
GridGraph makeGraphFromGrid (const GridArray& grid, int inflationRadius)
{
  const grid_size* dims = grid.shape();
  GridGraph gr(dims);
  Vertex v;
  CoordsMap coords = get (coords_t(), gr.g);
  int threshold = inflationRadius*inflationRadius;

  ROS_INFO_NAMED ("bottleneck_finder", "Constructing map graph\n");
  
  for (int r=0; r!=(int)dims[0]; r++) {
    ROS_DEBUG_NAMED ("bottleneck_finder"," Row %d", r);
    for (int c=0; c!=(int)dims[1]; c++) {

      // A point is added to the graph iff there are no obstacles near it

      gr.o[r][c] = true;
      for (int r2=r-inflationRadius; r2<=r+inflationRadius; r2++) {
        for (int c2=c-inflationRadius; c2<=c+inflationRadius; c2++) {
          
          //ROS_DEBUG_NAMED ("bottleneck_finder","%d %d %d %d %d\n", r, c, r2, c2, (r2-r)*(r2-r) + (c2-c)*(c2-c));
          
          if (((r2-r)*(r2-r) + (c2-c)*(c2-c) <= threshold) &&
              (r2>=0) && (r2<(int)dims[0]) && (c2>=0) && (c2<(int)dims[1]) && grid[r2][c2]) {
            //ROS_DEBUG_NAMED ("bottleneck_finder","accepted\n");
            gr.o[r][c] = false;
          }
        }
      }
      
      // If r,c is in the graph, add edges, do the necessary bookkeeping
      if (gr.o[r][c]) {
        v = add_vertex(gr.g);
        gr.m[r][c] = v;
        boost::put (coords, v, *(new Coords(r,c)));
        
        if ((r>0) && gr.o[r-1][c]) {
          boost::add_edge (v, gr.m[r-1][c], gr.g);
        }
        if ((c>0) && gr.o[r][c-1]) {
          boost::add_edge (v, gr.m[r][c-1], gr.g);
        }
      }
    }
  }
  return gr;
}





// Main loop: Iterate over the whole map and find square regions such that removing the region from the graph significantly increases the distance between cells on either side
void findDisconnectingBlocks (GridGraph* gr, BlockList* disconnectingBlocks, int bottleneckSize, int bottleneckSkip,
                              int distanceMultMin, int distanceMultMax)
{
  ROS_INFO_NAMED ("bottleneck_finder", "Searching for disconnecting blocks");
  const grid_size* dims = gr->m.shape();
  int r0, c0, r1, c1, dist=-1;
  for (grid_size r=1; r<dims[0]-bottleneckSize; r+=bottleneckSkip) {
    for (grid_size c=1; c<dims[1]-bottleneckSize; c+=bottleneckSkip) {
      ROS_DEBUG_NAMED ("bottleneck_finder","Block from (%d, %d) to (%d, %d)\n", r, c, r+bottleneckSize-1, c+bottleneckSize-1);
      
      // Will check pairs of cells on opposite sides of this block to see if they become disconnected
      bool disconnected = false;

      // Loop over which pair of opposite cells we're going to look at
      for (int vertical=0; vertical<2; vertical++) {
        if (vertical) {
          r0 = r-3*bottleneckSize/2;
          c0 = c+bottleneckSize/2;
          r1 = r+5*bottleneckSize/2;
          c1 = c+bottleneckSize/2;
        }
        else {
          r0 = r+bottleneckSize/2; 
          c0 = c-3*bottleneckSize/2;
          r1 = r+bottleneckSize/2;
          c1 = c+5*bottleneckSize/2;
        }

        if (!getFreePointNear(r0, c0, bottleneckSize, dims, gr) || !getFreePointNear(r1, c1, bottleneckSize, dims, gr))
            continue;



        // At this point, (r0,c0) and (r1,c1) are unoccupied legal cells on opposite sides of the block
        // Check if the distance is less than lower limit with the block, but greater than upper limit with block removed from graph
        dist = abs(r0-r1) + abs(c0-c1);
        ROS_DEBUG_NAMED ("bottleneck_finder","Checking if removing block %d, %d changes distance between %d,%d, and %d, %d from < %d to >= %d", 
                   r, c, r0, c0, r1, c1, dist+distanceMultMin*bottleneckSize, dist+distanceMultMax*bottleneckSize);


        if (distLessThan(gr, r0, c0, r1, c1, dist+distanceMultMin*bottleneckSize)) {
          ROS_DEBUG_NAMED ("bottleneck_finder","Within threshold before removing block");
          removeBlock (gr, r, c, bottleneckSize);
          disconnected = !distLessThan(gr, r0, c0, r1, c1, dist+distanceMultMax*bottleneckSize);
          addBlock (gr, r, c, bottleneckSize);
          if (disconnected) {
            ROS_DEBUG_NAMED ("bottleneck_finder","Disconnected because of cells (%d, %d) and (%d, %d)\n", r0, c0, r1, c1);
            break;
          }
        }
      }
        
      //printBlock (*gr, r, c, bottleneckSize, r0, c0, r1, c1, dims[0], dims[1]);
      if (disconnected) {
        assert (dist>0);
        addSmallestDisconnectingBlocks (gr, disconnectingBlocks, r, c, r0, c0, r1, c1, dist+distanceMultMax*bottleneckSize, bottleneckSize);
      }
      else {
        ROS_DEBUG_NAMED ("bottleneck_finder","Block is connected\n");
      }
    }
  }
}



// The top-level function that returns a topological graph containing bottleneck and open regions, given an occupancy grid
IndexedBottleneckGraph makeBottleneckGraph (GridArray grid, int bottleneckSize, int bottleneckSkip, int inflationRadius, int distanceMultMin, int distanceMultMax)
{
  GridGraph gr = makeGraphFromGrid(grid, inflationRadius);
  const grid_size* dims = gr.m.shape();
  IndexedBottleneckGraph g (dims[0], dims[1]);
  BlockList disconnectingBlocks;
  typedef vector<Region> RegionVector;

  // Find disconnecting blocks and disconnect them from rest of graph
  findDisconnectingBlocks (&gr, &disconnectingBlocks, bottleneckSize, bottleneckSkip, distanceMultMin, distanceMultMax);
  for (BlockList::iterator i = disconnectingBlocks.begin(); i!=disconnectingBlocks.end(); i++)
    markBottleneckCells (&gr, i->r, i->c, i->s);
  disconnectBottlenecks (&gr);


  // Compute connected components of resulting graph
  VertexCompMap vertexComp;
  boost::associative_property_map<VertexCompMap> component(vertexComp);
  int numComps = boost::connected_components (gr.g, component);
  RegionVector regions(numComps);
  for (VertexCompMap::iterator i = vertexComp.begin(); i!=vertexComp.end(); i++) 
    regions[i->second].insert(get (coords_t(), gr.g, i->first));


  // Construct the bottleneck graph
  desc_t vertexDescriptions;
  vector<BottleneckVertex> bottleneckGraphVertices;
  for (unsigned int i=0; i<regions.size(); i++) {
    Coords c = *(regions[i].begin());
    BottleneckVertex v = add_vertex (g.graph);
    bottleneckGraphVertices.push_back (v);
    VertexDescription d;
    d.type = get (bottleneck_t(), gr.g, gr.m[c.first][c.second]) ? BOTTLENECK : OPEN;
    d.region = regions[i];
    d.id = getUniqueId();
    boost::put (vertexDescriptions, g.graph, v, d);
  }
  connectRegions (gr, vertexComp, bottleneckGraphVertices, &(g.graph));
  pruneIsolatedBottlenecks (-1, &g);
  indexRegions(&g);
  

  return g;
}


} // namespace topological_map

