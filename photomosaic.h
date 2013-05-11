#ifndef __CUDA_MOSAIC_H__
#define __CUDA_MOSAIC_H__

#ifndef uint
#define uint unsigned int
#endif

#include "circleRenderer.h"


class CudaMosaic {

private:
  int numImages;
  int finalIndex;
  int numSlices;
  int cutSize;

  float* cudaDeviceImageAverages;
  float* cudaDeviceAllAverages;
  int* cudaDeviceImageIndex;

  float* imageAverages;
  float* allAverages;
  int* imageIndex;

public:
  CudaMosaic();
  virtual ~CudaMosaic();

  void setup();

  void imageMatch();
};


#endif
