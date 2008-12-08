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


#include <iostream>
#include <getopt.h>
#include <sysexits.h>
#include "topological_map/bottleneck_graph.h"

using std::cout;
using std::endl;


int main (int argc, char* argv[])
{
  int bottleneckSize=-1;
  int bottleneckSkip=-1;
  int inflationRadius=0;
  int domain=0;
  char* outputFilename=0;
  char* inputFilename=0;
  
  while (1) {
    static struct option options[] =
      {{"bottleneck-size", required_argument, 0, 'b'},
       {"bottleneck-skip", required_argument, 0, 'k'},
       {"inflation-radius", required_argument, 0, 'r'},
       {"domain", required_argument, 0, 'd'},
       {"outfile", required_argument, 0, 'o'},
       {"infile", required_argument, 0, 'i'},
       {0, 0, 0, 0}};

    int option_index=0;
    int c = getopt_long (argc, argv, "b:k:r:d:o:i:", options, &option_index);
    if (c==-1) {
      break;
    }
    else {
      switch (c) {
      case 'b':
        bottleneckSize = atoi(optarg);
        if (bottleneckSkip<0) {
          bottleneckSkip = 1+bottleneckSize/3;
        }
        break;
      case 'k':
        bottleneckSkip=atoi(optarg);
        break;
      case 'r':
        inflationRadius=atoi(optarg);
        break;
      case 'd':
        domain=atoi(optarg);
        break;
      case 'o':
        outputFilename=optarg;
        break;
      case 'i':
        inputFilename=optarg;
        break;
      default:
        exit(EX_USAGE);
      }
    }
  }
  topological_map::GridArray *grid;
  topological_map::IndexedBottleneckGraph g;
  if (inputFilename) {
    g = topological_map::readBottleneckGraphFromFile(inputFilename);
  }
  else {

    if ((bottleneckSize<0) || (domain>1)) {
      exit(EX_USAGE);
    }
  
    if (domain==0) {
      // Initialize grid
      grid = new topological_map::GridArray(boost::extents[4][5]);
      (*grid)[0][2] = true;
      (*grid)[2][2] = true;
      (*grid)[3][2] = true;
    }
    else {
      grid = new topological_map::GridArray(boost::extents[41][41]);
      for (int i=0; i<20; i++) {
        (*grid)[10][i] = true;
        (*grid)[10][40-i] = true;
      }
    }
      
    g = topological_map::makeBottleneckGraph (*grid, bottleneckSize, bottleneckSkip, inflationRadius);
  }


  //g.printBottleneckGraph ();

  cout << "Bottlenecks:" << endl;
  g.printBottlenecks();
  if (outputFilename) {
    g.printBottlenecks(outputFilename);
  }
}

  
  
  
  
  
