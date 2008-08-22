#include <ros/node.h>
#include "rosthread/mutex.h"

#include <std_msgs/PointCloudFloat32.h>

#include <octree.h>
#include <scan_utils/OctreeMsg.h>

/*! \file This is a thin node that listens for any point clouds coming
  over the network and places them in a binary Octree, then broadcasts
  the Octree over the network.

  Usage: cloudToOctree [cellSize]

  \param - cellSize is the size of the smallest possible Octree leaf
  that you want. Default is 0.02. The Octree is set to grow on its own
  to accomodate all the data that is inserted, while keeping the
  smallest leaf of the specified size.
 */

namespace scan_utils{

	class CloudToOctree : public ros::node
	{
	private:
		Octree<char> *mOctree;
		std_msgs::PointCloudFloat32 mNewCloud;
		void fullCloudCallback();
	public:
		CloudToOctree(float cellSize);
		~CloudToOctree();
		void resetOctree();
	};


	CloudToOctree::CloudToOctree(float cellSize) : ros::node("cloud_to_octree_node")
	{
		/* initialize Octree to depth 0. This means that the
		   smallest possible leaf will always have the
		   specified size */
		mOctree = new Octree<char>(0.0, 0.0, 0.0, cellSize, cellSize, cellSize, 0, (char)0);
		/* tell the Octree to expands on its own (grow in
		   height) if necessary whenever new data is
		   inserted.*/
		mOctree->setAutoExpand(true);

		//subsribe and advertise to relevant ROS topics
		subscribe("full_cloud", mNewCloud, &CloudToOctree::fullCloudCallback,1);
		advertise<OctreeMsg>("full_octree",1);
		fprintf(stderr,"ROS cloud-to-octree node with cell size %f created and subscribed!\n",cellSize);
	}

	CloudToOctree::~CloudToOctree() {
		delete mOctree;
		fprintf(stderr,"ROS cloud-to-octree node deleted.\n");
	}

	void CloudToOctree::fullCloudCallback() {
		fprintf(stderr,"Cloud received with %d points!\n",mNewCloud.get_pts_size());
		if ( mNewCloud.get_pts_size() == 0 ) {
			return;
		}
		//insert points into Octree
		for ( unsigned int i=0; i<mNewCloud.get_pts_size(); i++ ){
			mOctree->insert( mNewCloud.pts[i].x, mNewCloud.pts[i].y, mNewCloud.pts[i].z, (char)1);
		}

		//create and populate message
		OctreeMsg outMsg;
		mOctree->getAsMsg(outMsg);
		//and publish
		publish("full_octree",outMsg);
	}
	/*! Clears the contents of the Octree. Note that the size and
            depth are not changed, they remain whatever they were
            after the Octree expanded to accomodate all the inserts
            done so far.

	    If you want to make the Octree small again, you need to
	    set the depth back to 1 and then set the extents to
	    2*cell_size along each dimension.
	 */
	void CloudToOctree::resetOctree() {
		mOctree->clear();
	}

} //namespace scan_utils

int main(int argc, char **argv)
{
	ros::init(argc, argv);
	float cellSize = -1.0;
	if (argc > 1) {
		cellSize = atof(argv[1]);
	}
	if (cellSize <= 0) cellSize = 0.02;
	scan_utils::CloudToOctree cto(cellSize);
	cto.spin();
	ros::fini();
	return 0;
}
