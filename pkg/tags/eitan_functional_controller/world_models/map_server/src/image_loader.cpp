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

/*
 * This file contains helper functions for loading images as maps.
 * 
 * Author: Brian Gerkey
 */

#include <cstring>
#include <stdexcept>

#include <stdlib.h>
#include <stdio.h>

// We use SDL_image to load the image from disk
#include <SDL/SDL_image.h>

#include "map_server/image_loader.h"

// compute linear index for given map coords
#define MAP_IDX(sx, i, j) ((sx) * (j) + (i))

namespace map_server
{

void
loadMapFromFile(std_srvs::StaticMap::response* resp,
                const char* fname, double res, bool negate)
{
  SDL_Surface* img;

  unsigned char* pixels;
  unsigned char* p;
  int rowstride, n_channels;
  unsigned int i,j;
  int k;
  double occ;
  int color_sum;
  double color_avg;

  // Load the image using SDL.  If we get NULL back, the image load failed.
  if(!(img = IMG_Load(fname)))
  {
    std::string errmsg = std::string("failed to open image file \"") + 
            std::string(fname) + std::string("\"");
    throw std::runtime_error(errmsg);
  }

  // Copy the image data into the map structure
  resp->map.width = img->w;
  resp->map.height = img->h;
  resp->map.resolution = res;
  /// @todo Make the map's origin configurable, probably from within the
  /// comment section of the image file.
  resp->map.origin.x = 0.0;
  resp->map.origin.y = 0.0;
  resp->map.origin.th = 0.0;

  // Allocate space to hold the data
  resp->map.set_data_size(resp->map.width * resp->map.height);

  // Get values that we'll need to iterate through the pixels
  rowstride = img->pitch;
  n_channels = img->format->BytesPerPixel;

  // Copy pixel data into the map structure
  pixels = (unsigned char*)(img->pixels);
  for(j = 0; j < resp->map.height; j++)
  {
    for (i = 0; i < resp->map.width; i++)
    {
      // Compute mean of RGB for this pixel
      p = pixels + j*rowstride + i*n_channels;
      color_sum = 0;
      for(k=0;k<n_channels;k++)
        color_sum += *(p + (k));
      color_avg = color_sum / (double)n_channels;

      // If negate is true, we consider blacker pixels free, and whiter
      // pixels free.  Otherwise, it's vice versa.
      if(negate)
        occ = color_avg / 255.0;
      else
        occ = (255 - color_avg) / 255.0;
      
      // Apply thresholds to RGB means to determine occupancy values for
      // map.  Note that we invert the graphics-ordering of the pixels to
      // produce a map with cell (0,0) in the lower-left corner.
      //
      /// @todo Make the color thresholds configurable, probably from
      /// within the comments section of the image file.
      if(occ > 0.5)
        resp->map.data[MAP_IDX(resp->map.width,i,resp->map.height - j - 1)] = +100;
      else if(occ < 0.1)
        resp->map.data[MAP_IDX(resp->map.width,i,resp->map.height - j - 1)] = 0;
      else
        resp->map.data[MAP_IDX(resp->map.width,i,resp->map.height - j - 1)] = -1;
    }
  }

  SDL_FreeSurface(img);
}

}
