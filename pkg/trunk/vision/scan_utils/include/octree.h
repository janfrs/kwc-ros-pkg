#ifndef _octree_h_
#define _octree_h_

#include "octreeNodes.h"
#include <scan_utils/OctreeMsg.h>
#include <cmath>


namespace scan_utils {


/*! An Octree class. It is designed for access based on spatial
    coordinates, rather than cell indices.

    The datatype that is held in the leaves is templated. For now, the
    only requirement on the type is that is must allow the assignment
    operator = and the equality operators == and !=. 

    The Octree needs to know what part of the 3D world it is
    responsible for. It has a defined center somewhere in space and
    defined dimensions. After this is defined, just use spatial
    coordinates for insertions / queries and you never have to worry
    about cell indices.

    You can specifiy different extents along x,y and z. However, for
    now, this just means that all the Octree branches and leaves will
    have that given aspect ratio. It is not smart enough to always
    have cubic leaves, but more of them along one direction than
    another.

    You can specify the maximum depth of the tree as well. A \a
    maxDepth of \a d guarantees that the smallest leaves will have
    size 2^(-d) * total_octree_size. The Octree will have at most
    (2^d)^3 leaves.

    Example: an octree that covers a space of 20m * 20m * 20m with max
    depth 10 will have the smallest leaves of approx. 2cm * 2cm * 2cm
    in size.

    Set an empty value that will be returned for all unvisited regions
    of space. If you query the value of a region in space that you
    never set, the empty value will be returned.

    The Octree can be set to auto expand whenever an insertion is made
    that is currently out of bounds, Expansion works by adding new
    leaves above the current root, this increasing the depth of the
    Octree. The size of the smallest cell stays unchanged.

    IMPORTANT: do not store the \a emptyValue in a leaf and expect the
    Octree to behave as if that leaf was never set. A NULL leaf and a
    leaf with the \a emptyValue stored inside will return the same
    thing, but not always act the same way (see below for intersection
    tests). If you want a region of space to be EMPTY, use \erase to
    remove all leaves there.

    The Octree also provides cell access, meaning you can access an
    individual cell by its indices in the Octree rather than its
    coordinates in space. This was implemented for using the Octree as
    a dense voxel grid. However, this kind of access is discouraged -
    use spatial coordinates and don't worry about cells unless you
    really need to.

    An important note on the difference between cells and leaves: a
    leaf can be of any size, up to one eighth of the entire octree (in
    the case in which is sits directly under the root). A cell on the
    oter hand is always the same size, equal to the smallest possible
    leaf.

    The convention for cell accessors is the following: all indices
    range from 0 to 2^(mMaxDepth)-1. The cell with \a 0,0,0 indices is
    in the negative corner of the Octree, i.e. at the point with
    spatial coordinates mCx-mDx/2, mCy-mDy/2, mCz-mDz/2.

    The Octree also provides a number of boolean intersection tests
    against primitives (triangle, box, sphere). These tests will return
    true if ANY LEAF of the triangle intersects the primitive,
    REGARDLESS of the leaf's actual value. If you do not want a
    certain part of space to return true for these intersection tests,
    use \a erase to remove the leaves in that part of space.
*/
template <typename T>
class Octree {
 private:
	//! The root of the Octree - always a branch and never NULL.
	OctreeBranch<T> *mRoot;
	//! The max depth of the octree, minimum 1 (root and 8 leaves).
	/*! The smallest leaf is guaranteed to have size of 2^(-maxDepth) * total_octree_size
	 */
	int mMaxDepth;
	//! The total size of the Octree. Can be thought of as the dimensions of the root branch.
	float mDx, mDy, mDz;
	//! The location of the center of the octree.
	float mCx, mCy, mCz;
	//! The value that is returned for a never visited leaf
	T mEmptyValue;
	//! Intended for forward compatibility with future versions that might use timestamps
	bool mUsesTimestamps;
	//! If true, the Octree will expand itself whenever asked to insert someplace that is out of bounds
	bool mAutoExpand;

	//! Main accessor loop. Performs either insertion or deletion at the given coordinates.
	void insertOrErase(float x, float y, float z, T newValue, bool deletion);

	//! Converts cell indices to spatial coordinates. See main class description for details.
	inline bool cellToCoordinates(int i, int j, int k, float *x, float *y, float *z) const;
	//! Converts spatial coordinates to cell indices. See main class description for details.
	inline bool coordinatesToCell(float x, float y, float z, int *i, int *j, int *k) const;

 public:
	//! Constructor needs all initialization values.
	Octree(float cx, float cy, float cz,
	       float dx, float dy, float dz, 
	       int maxDepth, T emptyValue);
	//! Recursively deletes the tree by deleting the root.
	~Octree(){delete mRoot;}
	//! Sets the center of this Octree. Does NOT change the inner data.
	void setCenter(float cx, float cy, float cz){mCx = cx; mCy = cy; mCz = cz;}
	//! Sets the size of this Octree. Does NOT change the inner data.
	void setSize(float dx, float dy, float dz){mDx = dx; mDy = dy; mDz = dz;}
	//! Sets the max depth of this Octree. Does NOT change the inner data.
	/*! TODO: provide a version (or a flag) that also prunes any
            leaves that are further down than the new depth that is
            set.
	 */
	void setDepth(int d){if (d<=0) d=1; mMaxDepth = d;}
	//! Returns the max. depth of the Octree
	int getMaxDepth() const {return mMaxDepth;}
	//! Returns the maximum number of cells along one dimension of this Octree
	/*! Equals 2^(mMaxDepth). Overall, the Octree might contain up to getNumCells()^3 cells */
	int getNumCells() const {return (int)pow((float)2,mMaxDepth);} 
	//! Returns \a true if the point at x,y,z is inside the volume of this Octree
	inline bool testBounds(float x, float y, float z) const;
	//! Sets the mAutoExpand property
	/*! If the autoExpand flag is set to true, whenever an
            insertion is made to a point that is out of the bounds of
            the Octree, the tree will expand itself by increasing the
            depth until it contains the new coordinates.*/
	void setAutoExpand(bool ae){mAutoExpand = true;}
	void isAutoExpand() const {return mAutoExpand;}

	//! Inserts a value at given spatial coordinates.
	/*! If you want the region at x,y,z to be treated as if it was
            never visited, do NOT use this to insert an
            \a emptyValue. Use \a erase(x,y,z) instead.*/
	void insert(float x, float y, float z, T newValue){
		this->insertOrErase(x,y,z,newValue,false);
	}
	//! Deletes the data at given spatial coordinates.
	/*! The region at \a x,y,z will from now be treated as never
            touched, any query at that point will return \a
            mEmptyValue */
	void erase(float x, float y, float z) {
		this->insertOrErase(x,y,z,mEmptyValue, true);
	}
	//! Returns the value at given spatial coordinates
	T get(float x, float y, float z) const;
	//! Expands the Octree to contain the point at \a x,y,z. Works by adding new leaves ABOVE the current root.
	void expandTo(float x, float y, float z);

	//! Inserts a value at given cell coordinates.
	void cellInsert(int i, int j, int k, T newValue) {
		float x,y,z;
		if ( !cellToCoordinates(i,j,k,&x,&y,&z) ) return;
		insert(x,y,z,newValue);
	}
	//! Returns the value at given cell coordinates
	T cellGet(int i, int j, int k) const {
		float x,y,z;
		if ( !cellToCoordinates(i,j,k,&x,&y,&z) ) return mEmptyValue;
		return get(x,y,z);
	}
	//! Deletes data at given cell coordinates
	void cellErase(int i, int j, int k) {
		float x,y,z;
		if ( !cellToCoordinates(i,j,k,&x,&y,&z) ) return;
		erase(x,y,z);
	}
	//!Recursively counts how many cells (not leaves!) hold a particular value
	int cellCount(T value) const {return mRoot->cellCount(value, mEmptyValue, mMaxDepth);}

	//! Checks intersection of the Octree against a triangle
	bool intersectsTriangle(float*, float*, float*) const;
	//! Checks intersection of the Octree against a box (not necessarily axis-oriented)
	bool intersectsBox(const float *center, const float *extents, const float axes[][3]) const;
	//! Checks intersection of the Octree against a sphere
	bool intersectsSphere(const float *center, float radius) const;

	//! Traces a scanner ray through this octree
	void traceRay(float sx, float sy, float sz, 
		      float dx, float dy, float dz, float distance,  
		      T emptyVal, T occupiedVal);

	//! Clears and deallocates the entire Octree. 
	void clear();
	//! Recursively parses the Octree performing aggregation everyehere where appropriate
	void aggregate(){mRoot->recursiveAggregation();}

	//! Returns the triangles that form the surface mesh of all non-empty leaves
	void getAllTriangles(std::list<Triangle> &triangles) const;
	//! Returns the triangles that form the surface mesh of the leaves with the given value
	void getTrianglesByValue(std::list<Triangle> &triangles, T value) const;
	//! Recursively computes and returns the number of branches of the tree
	int getNumBranches() const {return mRoot->getNumBranches();}
	//! Recursively computes and returns the number of leaves of the tree
	int getNumLeaves() const {return mRoot->getNumLeaves();}
	//! Returns the space in memory occupied by the tree
	long long unsigned int getMemorySize() const;

	//! Serializes this octree to a string
	void serialize(char **destinationString, unsigned int *size) const;
	//! Reads in the content of this Octree. OLD CONTENT IS DELETED!
	bool deserialize(char *sourceString, unsigned int size);


	//! Recursively computes the max depth in the Octree. Useful when Octree is read from file.
	int computeMaxDepth() const {return mRoot->computeMaxDepth();}
	//! Set this Octree from a ROS message
	void setFromMsg(const OctreeMsg &msg);
	//! Write this Octree in a ROS message
	void getAsMsg(OctreeMsg &msg) const;
	//! Write the Octree to a file in its own internal format
	void writeToFile(std::ostream &os) const;
	//! Read Octree from a file saved by the \a writeToFile(...) function 
	void readFromFile(std::istream &is);

	//! Checks if the given ray intersects the volume that this Octree occupies
	bool intersectsRay(float rx, float ry, float rz, 
			   float dx, float dy, float dz,
			   float &t0, float &t1) const;

	//! Inserts the contents of a scan into this octree
	//void insertScan(const SmartScan *s, T insertVal);
};


//------------------------------------- Constructor, destructor -------------------------------


/*! Initializes the root to a branch with all NULL children.

  \param cx,cy,cz - the center of the Octree in space

  \param dx,dy,dz - the dimensions of the Octree along all axes

  \param maxDepth - the maximum depth of the Octree

  \param emptyValue - the value that is returned for unvisited regions
  of space.
 */

template <typename T>
Octree<T>::Octree(float cx, float cy, float cz,
	       float dx, float dy, float dz, 
	       int maxDepth, T emptyValue)

{
	mRoot = new OctreeBranch<T>();
	setCenter(cx,cy,cz);
	setSize(dx,dy,dz);
	setDepth(maxDepth);
	mEmptyValue = emptyValue;
	mUsesTimestamps = false;
	mAutoExpand = false;
}

//--------------------------------------- Navigation ------------------------------------------


template <typename T>
bool Octree<T>::testBounds(float x, float y, float z) const
{
	float dx = mDx / 2.0;
	float dy = mDy / 2.0;
	float dz = mDz / 2.0;
	if ( x > mCx + dx ) return false; if (x < mCx - dx ) return false;
	if ( y > mCy + dy ) return false; if (y < mCy - dy ) return false;
	if ( z > mCz + dz ) return false; if (z < mCz - dz ) return false;
	return true;
}

/*! 
    If \a deletion is true, this will delete the value stored at the
    lowermost leaf at \a x,y,z. Any subsequent query will then return
    \a mEmptyValue. In this case, \a newValue is not used.

    If \a deletion is false, this function inserts the new value at the
    spatial location specified by \a x,y,z. It is not recursive, in an
    attempt to be more efficient.  If the given region of space is
    previously unvisited, it will travel down creating Branches as it
    goes, until reaching a depth of \a mMaxDepth. At that point it
    will create a Leaf with the value \a newValue.

    After performing deletion or insertion, this function will
    navigate back towards the top of the tree, aggregating leaves
    where appropriate.

    If the point at \a x,y,z is out of bounds: if the \a mAutoExpand
    is true, the tree will expand (by adding new leaves on top of the
    current root) until it contains the point at \a x,y,z. It will
    then proceed with insertion normally. If the flag is false, this
    will just return without inserting anything.
 */
template <typename T>
void Octree<T>::insertOrErase(float x, float y, float z, T newValue, bool deletion)
{
	if (!testBounds(x,y,z)) {
		if (mAutoExpand && !deletion) {
			expandTo(x,y,z);
		} else {
			return;
		}
	}

	float cx = mCx, cy = mCy, cz = mCz;
	float dx = mDx / 2.0, dy = mDy / 2.0, dz = mDz / 2.0;
	
	int depth = 0;
	unsigned char address;
	OctreeBranch<T> *currentNode = mRoot;
	OctreeNode<T> *nextNode;

	std::list<OctreeBranch<T>*> visitedBranches;

	while (1) {
		dx /= 2.0;
		dy /= 2.0;
		dz /= 2.0;
		depth++;

		address = 0;
		if ( x > cx) {address += 4; cx += dx;}
		else { cx -= dx;}
		if ( y > cy) {address += 2; cy += dy;}
		else {cy -= dy;}
		if ( z > cz) {address += 1; cz += dz;}
		else {cz -= dz;}
		
		//current node is a branch by definition
		visitedBranches.push_back( currentNode );
		nextNode = currentNode->getChild(address);

		if (!nextNode) {
			// unexplored region of space

			//if we wanted to perform deletion it is not necessary; we are done
			if (deletion) break;

			//if not, extend the tree
			if (depth >= mMaxDepth) {
				// we have reached max depth; create a new leaf
				nextNode = new OctreeLeaf<T>(newValue);
				currentNode->setChild(address, nextNode);
				// and we are done
				break;
			} else {
				// create a new unexplored branch
				nextNode = new OctreeBranch<T>();
				currentNode->setChild(address, nextNode);
			}
		} else if (nextNode->isLeaf()) {

			//leaf already is set to new value; we are done. Check for aggregates anyway.
			if ( !deletion && ((OctreeLeaf<T>*)nextNode)->getVal()==newValue ) break;

			//we have reached the max depth; set the leaf to new value then done
			if (depth >= mMaxDepth) { 
				//delete if we want to
				if (deletion) currentNode->setChild(address, NULL);
				//or just set new value
				else ((OctreeLeaf<T>*)nextNode)->setVal(newValue); 
				break;
			}

			//create a new branch with the all children leaves with the old value
			nextNode = new OctreeBranch<T>( ((OctreeLeaf<T>*)nextNode)->getVal() );
			currentNode->setChild(address, nextNode);
		} 

		// advance the recursion 
		currentNode = (OctreeBranch<T>*)nextNode;
	}

	//go back through the list of visited nodes and check if we need to aggregate
	bool agg = true;
	OctreeLeaf<T> *newLeaf;
	while (agg && (int)visitedBranches.size() > 1) {
		currentNode = visitedBranches.back();
		visitedBranches.pop_back();
		agg = currentNode->aggregate(&newLeaf);
		if (agg) {
			//this will also delete the old branch
			visitedBranches.back()->replaceChild(currentNode, newLeaf);
		}
	}
	visitedBranches.clear();
}

/*! Returns the value at the specified spatial coordinates. If that
    region of space is unvisited, returns \a mEmptyValue.
 */
template <typename T>
T Octree<T>::get(float x, float y, float z) const
{
	if (!testBounds(x,y,z)) return mEmptyValue;

	float cx = mCx, cy = mCy, cz = mCz;
	float dx = mDx / 2.0, dy = mDy / 2.0, dz = mDz / 2.0;
	
	unsigned char address;
	OctreeBranch<T> *currentNode = mRoot;
	OctreeNode<T> *nextNode;

	while (1) {
		dx /= 2.0; dy /= 2.0; dz /= 2.0;

		address = 0;
		if ( x > cx) {address += 4; cx += dx;}
		else { cx -= dx;}
		if ( y > cy) {address += 2; cy += dy;}
		else {cy -= dy;}
		if ( z > cz) {address += 1; cz += dz;}
		else {cz -= dz;}
		
		nextNode = currentNode->getChild(address);
		if (!nextNode) {
			return mEmptyValue;
		} else if (nextNode->isLeaf()) {
			return ((OctreeLeaf<T>*)nextNode)->getVal();
		} 

		currentNode = (OctreeBranch<T>*)nextNode;
	}
}

template <typename T>
void Octree<T>::clear()
{
	delete mRoot;
	mRoot = new OctreeBranch<T>();
}

/*! This works just by increasing the depth of the Octree - adding new
    leaves ABOVE the current root until the point at \a x,y,z is
    contained within the Octree. It does not actually insert anyting
    at \a x,y,z. All the current structure is left unchanged, and the
    smallest leaves will still have the same size. They will just be
    deeper down inside the tree.
 */
template <typename T>
void Octree<T>::expandTo(float x, float y, float z)
{
	while (!testBounds(x,y,z)) {
		unsigned char address = 0;
		//we are now computing the address of the current root in the list of
		//the future root which will be added on top. It is reversed from the address
		//computation used in insertOrErase(...)
		if ( x < mCx - mDx/2.0 ) { address +=4; mCx -= mDx/2.0;}
		else { mCx += mDx/2.0;}
		if ( y < mCy - mDy/2.0 ) { address +=2; mCy -= mDy/2.0;}
		else { mCy += mDy/2.0;}
		if ( z < mCz - mDz/2.0 ) { address +=1; mCz -= mDz/2.0;}
		else { mCz += mDz/2.0;}

		//we have doubled the size
		mDx *= 2.0; mDy *= 2.0; mDz *= 2.0;
		
		//create the new root and set current root as child
		OctreeBranch<T>* newRoot = new OctreeBranch<T>();
		newRoot->setChild(address,mRoot);
		
		//see if we can aggregate the old root
		OctreeLeaf<T>* newLeaf;
		if (mRoot->aggregate(&newLeaf)) {
			//this also deletes the old root
			newRoot->setChild(address, newLeaf);
		}
		
		//set the new root
		mRoot = newRoot;
		
		//we have increased the depth
		mMaxDepth++;
	}
}

//----------------------------------------- Cell accessors -----------------------------------------

/*!
 */
template <typename T>
bool Octree<T>::cellToCoordinates(int i, int j, int k, float *x, float *y, float *z) const
{
	int numCells = getNumCells();
	*x = mCx - (mDx / 2.0) + (mDx / numCells) * (i + 0.5);
	*y = mCy - (mDy / 2.0) + (mDy / numCells) * (j + 0.5);
	*z = mCz - (mDz / 2.0) + (mDz / numCells) * (k + 0.5);
	if (i<0 || j<0 || k<0) return false;
	if (i >= numCells || j >= numCells || k >= numCells) return false;
	return true;
}
 
 /*!
  */
template <typename T>
bool Octree<T>::coordinatesToCell(float x, float y, float z, int *i, int *j, int *k) const
{
	int numCells = getNumCells();
	*i = (int)( floor( (x - mCx) / numCells ) + (numCells / 2) );
	*j = (int)( floor( (y - mCy) / numCells ) + (numCells / 2) );
	*k = (int)( floor( (z - mCz) / numCells ) + (numCells / 2) );
	if (!testBounds(x,y,z)) return false;
	return true;
}

//------------------------------------------ Intersection tests ------------------------------------
/*!
  \param trivert0,trivert1,trivert2 - the 3 vertices of the triangle
 */
template <typename T>
bool Octree<T>::intersectsTriangle(float trivert0[3], float trivert1[3], float trivert2[3]) const
{
	std::list< SpatialNode<T>* > stack;

	SpatialNode<T> *sn = new SpatialNode<T>;
	sn->node = mRoot;

	sn->cx = mCx; sn->cy = mCy; sn->cz = mCz;
	sn->dx = mDx; sn->dy = mDy; sn->dz = mDz;
	stack.push_front(sn);

	bool retVal = false;
	while (!stack.empty()) {
		sn = stack.front();
		stack.pop_front();

		if (sn->node) {
			
			/* this is the actual intersection test. If you need a novel intersection test
			   (with a new primitive) this is the only part of this fctn that you need to
			   modify
			*/
			if ( nodeTriangleIntersection<T>(*sn, trivert0, trivert1, trivert2) ) {

				if (sn->node->isLeaf()) {
					delete sn;
					retVal = true;
					break;
				}

				for (int i=0; i<8; i++) {
					stack.push_front( ((OctreeBranch<T>*)sn->node)->getSpatialChild(i,sn->cx, sn->cy, 
													sn->cz, sn->dx, 
													sn->dy, sn->dz) );
				}
			}
		}
		delete sn;
	}
	
	while (!stack.empty()) {
		delete stack.front();
		stack.pop_front();
	}
	return retVal;
}

/*!
  \param center - the center of the box

  \param extents - the half dimensions of the box along each of its
  axes. Think of them as how much the box extends from the center in
  each direction.

  \param axes - a 3x3 orthonormal matrix holding the directions of the
  box axes. If this is not orthonormal, the behavior of the function
  is undefined!
 */
template <typename T>
bool Octree<T>::intersectsBox(const float *center, const float *extents, const float axes[][3]) const
{
	/*
	fprintf(stderr,"Axes:\n");
	fprintf(stderr,"%f %f %f \n",axes[0][0], axes[0][1], axes[0][2]);
	fprintf(stderr,"%f %f %f \n",axes[1][0], axes[1][1], axes[1][2]);
	fprintf(stderr,"%f %f %f \n",axes[2][0], axes[2][1], axes[2][2]);
	*/
	std::list< SpatialNode<T>* > stack;

	SpatialNode<T> *sn = new SpatialNode<T>;
	sn->node = mRoot;

	sn->cx = mCx; sn->cy = mCy; sn->cz = mCz;
	sn->dx = mDx; sn->dy = mDy; sn->dz = mDz;
	stack.push_front(sn);

	bool retVal = false;
	while (!stack.empty()) {
		sn = stack.front();
		stack.pop_front();

		if (sn->node) {
			
			/* this is the actual intersection test. If you need a novel intersection test
			   (with a new primitive) this is the only part of this fctn that you need to
			   modify
			*/
			if ( nodeBoxIntersection<T>(*sn, center, extents, axes) ) {
				if (sn->node->isLeaf()) {
					delete sn;
					retVal = true;
					break;
				}

				for (int i=0; i<8; i++) {
					stack.push_front( ((OctreeBranch<T>*)sn->node)->getSpatialChild(i,sn->cx, sn->cy, 
													sn->cz, sn->dx, 
													sn->dy, sn->dz) );
				}
			}
		}
		delete sn;
	}
	
	while (!stack.empty()) {
		delete stack.front();
		stack.pop_front();
	}
	return retVal;

}

/*!
  \param center - the center of the sphere

  \param radius - the radius of the sphere
 */
template <typename T>
bool Octree<T>::intersectsSphere(const float *center, float radius) const
{
	std::list< SpatialNode<T>* > stack;

	SpatialNode<T> *sn = new SpatialNode<T>;
	sn->node = mRoot;

	sn->cx = mCx; sn->cy = mCy; sn->cz = mCz;
	sn->dx = mDx; sn->dy = mDy; sn->dz = mDz;
	stack.push_front(sn);

	bool retVal = false;
	while (!stack.empty()) {
		sn = stack.front();
		stack.pop_front();

		if (sn->node) {
			
			/* this is the actual intersection test. If you need a novel intersection test
			   (with a new primitive) this is the only part of this fctn that you need to
			   modify
			*/
			if ( nodeSphereIntersection<T>(*sn, center, radius) ) {
				if (sn->node->isLeaf()) {
					delete sn;
					retVal = true;
					break;
				}

				for (int i=0; i<8; i++) {
					stack.push_front( ((OctreeBranch<T>*)sn->node)->getSpatialChild(i,sn->cx, sn->cy, 
													sn->cz, sn->dx, 
													sn->dy, sn->dz) );
				}
			}
		}
		delete sn;
	}
	
	while (!stack.empty()) {
		delete stack.front();
		stack.pop_front();
	}
	return retVal;

}
//------------------------------------------ Statistics --------------------------------------------

template <typename T>
long long unsigned int Octree<T>::getMemorySize() const
{
	unsigned int leaves = mRoot->getNumLeaves();
	unsigned int branches = mRoot->getNumBranches();
	return leaves * sizeof(OctreeLeaf<T>) + branches * sizeof(OctreeBranch<T>);
}

//------------------------------------------ Serialization ------------------------------------------

/*!  This only saves the inner structure of the tree, not center,
  extents, maxdepth etc.
 */
template <typename T>
void Octree<T>::serialize(char **destinationString, unsigned int *size) const
{
	int nLeaves = mRoot->getNumLeaves();
	int nBranches = mRoot->getNumBranches();

	//each branch stores a byte for each child
	//each leaf stores its value
	*size = 8 * nBranches + sizeof(mEmptyValue) * nLeaves;

	*destinationString = new char[*size];

	unsigned int address = 0;
	mRoot->serialize(*destinationString, address);
	//sanity check
	if (address != *size) {
		fprintf(stderr,"Serialization error; unexpected size\n");
	}
}

/*! It is equivalent to calling \a clear(...) first and then \a
  deserialize(...)  

  \param size - the total size of the string passed in. It is only
  used for checking correctness and avoiding memory corruption

  Return \a true if the Octree is deserialized succesfully. If it
  returns \a false, the deserialized Octree is not to be trusted.
*/

template <typename T>
bool Octree<T>::deserialize(char *sourceString, unsigned int size)
{
	unsigned int address = 0;
	mRoot->deserialize(sourceString,address, size);
	if (address != size) {
		fprintf(stderr,"Octree serialization error!\n");
		return false;
	}
	return true;
}

template <typename T>
void Octree<T>::setFromMsg(const OctreeMsg &msg)
{
	setCenter( msg.center.x, msg.center.y, msg.center.z);
	setSize(msg.size.x, msg.size.y, msg.size.z);
	setDepth(msg.max_depth);
	if (msg.uses_timestamps) mUsesTimestamps = true;
	else mUsesTimestamps = false;

	if (sizeof(mEmptyValue) != msg.get_empty_value_size()) {
		fprintf(stderr,"Incompatible data type in ROS Octree message!\n");
		return;
	}
	memcpy((char*)&mEmptyValue, msg.empty_value, sizeof(mEmptyValue) );

	unsigned int size = msg.get_structure_data_size();
	char *data  = new char[size];
	memcpy(data, msg.structure_data, size);
	deserialize(data,size);
	delete[] data;

	int depth = computeMaxDepth();
	if (depth > mMaxDepth) {
		fprintf(stderr,
			"Octree read from message: depth found in data is greater than maxDepth specified in header!\n");
		mMaxDepth = depth;
	}
}

template <typename T>
void Octree<T>::getAsMsg(OctreeMsg &msg) const
{
	msg.center.x = mCx; msg.center.y = mCy; msg.center.z = mCz;
	msg.size.x = mDx; msg.size.y = mDy; msg.size.z = mDz;
	msg.max_depth = mMaxDepth;
	if (mUsesTimestamps) msg.uses_timestamps = 1;
	else msg.uses_timestamps = 0;

	msg.set_empty_value_size(sizeof(mEmptyValue));
	memcpy(msg.empty_value, (char*)&mEmptyValue, sizeof(mEmptyValue));

	unsigned int size;
	char *data;
	serialize(&data, &size);
	msg.set_structure_data_size(size);
	memcpy(msg.structure_data, data, size);
	delete [] data;
}

template <typename T>
void Octree<T>::writeToFile(std::ostream &os) const
{
	//write the admin data
	float fl[6];
	fl[0] = mCx; fl[1] = mCy; fl[2] = mCz;
	fl[3] = mDx; fl[4] = mDy; fl[5] = mDz;
	os.write((char*)fl, 6 * sizeof(float));
	os.write((char*)&mEmptyValue, sizeof(mEmptyValue));
	os.write((char*)&mMaxDepth, sizeof(int));
	os.write((char*)&mUsesTimestamps,sizeof(bool));

	//write the volume data
	unsigned int size;
	char *octreeString;
	serialize(&octreeString,&size);
	os.write(octreeString,size);
	delete [] octreeString;
}

/*
  WARNING: the structure data is assumed to span from the current
  point in the stream UNTIL EOF IS REACHED. There should be NOTHING in
  the stream following this Octree.
 */
template <typename T>
void Octree<T>::readFromFile(std::istream &is)
{
	//read the admin data
	float fl[6];
	is.read( (char*)fl, 6*sizeof(float));
	mCx = fl[0]; mCy = fl[1]; mCz = fl[2];
	mDx = fl[3]; mDy = fl[4]; mDz = fl[5];
	is.read( (char*)&mEmptyValue, sizeof(mEmptyValue) );
	is.read( (char*)&mMaxDepth, sizeof(int) );
	is.read( (char*)&mUsesTimestamps, sizeof(bool) );

	//read the volume data
	unsigned int current = (unsigned int)is.tellg();
	is.seekg (0, std::ios::end);
	unsigned int size = (unsigned int)is.tellg() - current;
	is.seekg (current );

	char *octreeString = new char[size];
	is.read(octreeString, size);
	if (is.fail()) {
		fprintf(stderr,"Only able to read %d instead of %d characters\n",is.gcount(), size);
	} else {
		deserialize(octreeString,size);
	}

	delete [] octreeString;
}

/*
template <typename T>
void Octree<T>::insertScan(const SmartScan *s, T insertVal)
{
}
*/

//--------------------------------------------- Triangulation -----------------------------------------

/*!  Returns the triangles that form the surface mesh of the NON-EMPTY
  (previously visited) leaves in the Octree (regardless of the value
  they hold).

  Tries to be somewhat smart about not returning unnecessary triangles,
  but is not very good at it. A good algorithm for doing that seems to
  be an interesting problem...
 */
template <typename T>
void Octree<T>::getAllTriangles(std::list<Triangle> &triangles) const
{
	if (!mRoot) return;
	mRoot->getTriangles( true, true, true, true, true, true, 
			     mCx, mCy, mCz, mDx/2.0, mDy/2.0, mDz/2.0, 
			     triangles, NULL, mEmptyValue);
}

/*!  Returns the triangles that form the surface mesh of the leaves
  with the value equal to \a value.

  Tries to be somewhat smart about not returning unnecessary triangles,
  but is not very good at it. A good algorithm for doing that seems to
  be an interesting problem...
 */
template <typename T>
void Octree<T>::getTrianglesByValue(std::list<Triangle> &triangles, T value) const
{
	if (!mRoot) return;
	T val = value;
	mRoot->getTriangles( true, true, true, true, true, true, 
			     mCx, mCy, mCz, mDx/2.0, mDy/2.0, mDz/2.0, 
			     triangles, &val, mEmptyValue);

}

//---------------------------------------------- Miscellaneous ------------------------------------------

/*! Traces a ray from a scanned through the volume of this octree,
    marking both empty and occupied leaves.

    \param sx,sy,sz - the starting point of the ray (presumably the
    location of the scanner)

    \param dx,dy,dz - the direction of the ray. Needs not to be
    normalized.

    \param distance - distance along the ray to object. If a negative
    value is passed, this will just mark with the empty value along
    the ray across the entire octree.

    \param emptyVal - the value that is used to mark "empty" leaves

    \param occupiedVal - the value that is used to mark the "occupied"
    cell at the end of the ray.

    All cells on the ray between the starting point and the point that
    is at \a distance are marked as empty. The one cell that is
    exactly at \distance away is then marked as occupied. All other
    cells in the grid (including those beyong \a distance) are left
    untouched.

    WARNING: this is currently a rather innefficient and incomplete
    approximation, which was very easy to implement.

 */
template <typename T>
void Octree<T>::traceRay(float sx, float sy, float sz, 
			 float dx, float dy, float dz, float distance, 
			 T emptyVal, T occupiedVal)
{
	//normalize the direction
	float norm = dx*dx + dy*dy + dz*dz;
	dx /= norm; dy /= norm; dz /= norm;

	//figure out the smallest dimension of the smallest cell
	int numCells = (int)pow((float)2,mMaxDepth);
	float minSize = mDx / numCells;
	if ( mDy / numCells < minSize) minSize = mDy / numCells;
	if ( mDz / numCells < minSize) minSize = mDz / numCells;

	float currentDistance = 0;
	bool done = false;
	
	//check if ray hits octree at all, and if it does what are the bounds
	float minDist=0, maxDist=-1;
	if (!intersectsRay(sx, sy, sz,  dx, dy, dz,  minDist, maxDist)) {
		//fprintf(stderr,"Miss\n");
		return;
	}

	//fprintf(stderr,"Hit\n");
	currentDistance = minDist;
	done = false;
	//advance until we get to where we should, or exit the octree
	while (!done) {
		insert(sx + currentDistance * dx, 
		       sy + currentDistance * dy, 
		       sz + currentDistance * dz, emptyVal);
		currentDistance += minSize;
		//check if we have hit the occupied cell
		if (distance > 0 && currentDistance > distance) done = true;
		//check if we have exited the tree
		if (currentDistance > maxDist) done = true;
	}

	//mark the occuppied cell
	if (distance > 0) {
		insert( sx + distance * dx, sy + distance * dy, sz + distance * dz, occupiedVal );
	}
}

/*! Returns \a true if the ray starting at \a rx,ry,rz in the
    direction \a dx,dy,dz intersects this octree between \a t0 and \a
    t1.
 */
template <typename T>
bool Octree<T>::intersectsRay(float rx, float ry, float rz, 
			      float dx, float dy, float dz,
			      float &t0, float &t1) const
{
	float ex = mDx / 2.0;
	float ey = mDy / 2.0;
	float ez = mDz / 2.0;

	float minx = mCx - ex, maxx = mCx + ex;
	float miny = mCy - ey, maxy = mCy + ey;
	float minz = mCz - ez, maxz = mCz + ez;

	float tmin, tmax, tymin, tymax, tzmin, tzmax;

	if (dx >= 0) {
		tmin = (minx - rx) / dx;
		tmax = (maxx - rx) / dx;
	}
	else {
		tmin = (maxx - rx) / dx;
		tmax = (minx - rx) / dx;
	}
	if (dy >= 0) {
		tymin = (miny - ry) / dy;
		tymax = (maxy - ry) / dy;
	}
	else {
		tymin = (maxy - ry) / dy;
		tymax = (miny - ry) / dy;
	}

	if ( (tmin > tymax) || (tymin > tmax) )
		return false;
	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;

	if (dz >= 0) {
		tzmin = (minz - rz) / dz;
		tzmax = (maxz - rz) / dz;
	}
	else {
		tzmin = (maxz - rz) / dz;
		tzmax = (minz - rz) / dz;
	}

	if ( (tmin > tzmax) || (tzmin > tmax) )
		return false;

	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;

	//return ( (tmin < t1) && (tmax > t0) );
	if (tmax < t0) return false;
	if (t1 > 0 && tmin < t1) return false;

	t0 = tmin; t1 = tmax;
	return true;
}

} //namespace scan_utils

#endif
