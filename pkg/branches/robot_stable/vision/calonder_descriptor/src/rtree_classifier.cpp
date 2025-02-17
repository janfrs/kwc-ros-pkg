#include "calonder_descriptor/rtree_classifier.h"
#include "calonder_descriptor/patch_generator.h"
#include <fstream>
#include <cstring>
#include <boost/foreach.hpp>

namespace features {


RTreeClassifier::RTreeClassifier()
  : classes_(0)
{
   //setReducedDim(DEFAULT_RED);
}

void RTreeClassifier::train(std::vector<BaseKeypoint> const& base_set,
                            Rng &rng, int num_trees, int depth,
                            int views, size_t reduced_num_dim,
                            int num_quant_bits)
{
  PatchGenerator make_patch(NULL, rng);
  train(base_set, rng, make_patch, num_trees, depth, views, reduced_num_dim, num_quant_bits);
}

// Single-threaded version of train(), with progress output
void RTreeClassifier::train(std::vector<BaseKeypoint> const& base_set,
                            Rng &rng, PatchGenerator &make_patch, int num_trees,
                            int depth, int views, size_t reduced_num_dim, 
                            int num_quant_bits)
{
  if (reduced_num_dim > base_set.size()) {
    printf("INVALID PARAMS in RTreeClassifier::train: reduced_num_dim > base_set.size()\n");
    return;
  }
  classes_ = reduced_num_dim; // base_set.size();
  original_num_classes_ = base_set.size();
  trees_.resize(num_trees);
  
  printf("[OK] Training trees: base size=%i, reduced size=%i\n", base_set.size(), reduced_num_dim); 
  
  int count = 1;
  printf("[OK] Trained 0 / %i trees\r", num_trees);
  fflush(stdout);
  BOOST_FOREACH( RandomizedTree &tree, trees_ ) {
    tree.train(base_set, rng, make_patch, depth, views, reduced_num_dim, num_quant_bits);    
    printf("[OK] Trained %i / %i trees\r", count++, num_trees);
    fflush(stdout);
  }
  printf("\n");
}

// TODO: vectorize
void RTreeClassifier::getSignature(IplImage* patch, float *sig)
{
  // Need pointer to 32x32 patch data
  uchar buffer[RandomizedTree::PATCH_SIZE * RandomizedTree::PATCH_SIZE];
  uchar* patch_data;
  if (patch->widthStep != RandomizedTree::PATCH_SIZE) {
    //printf("[INFO] patch is padded, data will be copied (%i/%i).\n", 
    //       patch->widthStep, RandomizedTree::PATCH_SIZE);
    uchar* data = getData(patch);
    patch_data = buffer;
    for (int i = 0; i < RandomizedTree::PATCH_SIZE; ++i) {
      memcpy((void*)patch_data, (void*)data, RandomizedTree::PATCH_SIZE);
      data += patch->widthStep;
      patch_data += RandomizedTree::PATCH_SIZE;
    }
    patch_data = buffer;
  } else {
    patch_data = getData(patch);
  }
    
  memset((void*)sig, 0, classes_ * sizeof(float));
  std::vector<RandomizedTree>::iterator tree_it;
 
  // get posteriors
  float **posteriors = new float*[trees_.size()];  // TODO: move alloc outside this func
  float **pp = posteriors;    
  for (tree_it = trees_.begin(); tree_it != trees_.end(); ++tree_it, pp++)
    *pp = tree_it->getPosterior(patch_data);       

  // sum them up
  pp = posteriors;
  for (tree_it = trees_.begin(); tree_it != trees_.end(); ++tree_it, pp++)
    add(classes_, sig, *pp, sig);

  delete [] posteriors;
  posteriors = NULL;
      
  // full quantization (experimental)
  #if 0
    int n_max = 1<<8 - 1;
    int sum_max = (1<<3 - 1)*trees_.size();
    int shift = 0;    
    while ((sum_max>>shift) > n_max) shift++;
    
    for (int i = 0; i < classes_; ++i) {
      sig[i] = int(sig[i] + .5) >> shift;
      if (sig[i]>n_max) sig[i] = n_max;
    }

    static bool warned = false;
    if (!warned) {
      printf("[WARNING] Using full quantization (RTreeClassifier::getSignature)! shift=%i\n", shift);
      warned = true;
    }    
  #else
    // TODO: get rid of this multiply (-> number of trees is known at train 
    // time, exploit it in RandomizedTree::finalize())
    float normalizer = 1.0f / trees_.size();
    for (int i = 0; i < classes_; ++i)
      sig[i] *= normalizer;
  #endif
}


void RTreeClassifier::read(const char* file_name)
{
  std::ifstream file(file_name, std::ifstream::binary);
  read(file);
}

void RTreeClassifier::read(std::istream &is)
{
  int num_trees = 0;
  is.read((char*)(&num_trees), sizeof(num_trees));
  is.read((char*)(&classes_), sizeof(classes_));
  is.read((char*)(&original_num_classes_), sizeof(original_num_classes_));

  trees_.resize(num_trees);
  std::vector<RandomizedTree>::iterator tree_it;
  for (tree_it = trees_.begin(); tree_it != trees_.end(); ++tree_it) {
    tree_it->read(is);
  }
}

void RTreeClassifier::write(const char* file_name) const
{
  std::ofstream file(file_name, std::ofstream::binary);
  write(file);
}

void RTreeClassifier::write(std::ostream &os) const
{
  int num_trees = trees_.size();
  os.write((char*)(&num_trees), sizeof(num_trees));
  os.write((char*)(&classes_), sizeof(classes_));
  os.write((char*)(&original_num_classes_), sizeof(original_num_classes_));

  std::vector<RandomizedTree>::const_iterator tree_it;
  for (tree_it = trees_.begin(); tree_it != trees_.end(); ++tree_it)
    tree_it->write(os);
}

} // namespace features
