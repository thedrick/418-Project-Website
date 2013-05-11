#include <math.h>
#include <stdio.h>
#include <vector>

#include <cuda.h>
#include <cuda_runtime.h>
#include <driver_functions.h>

#include "photomosaic.h"

struct GlobalConstants {
  int numImages;
  int finalIndex;
  int numSlices;
  int cutSize;

  float* imageAverages;
  float* allAverages;
  int* imageIndex;
};

__constant__ GlobalConstants cuConstMosaicParams;


__global__ void kernelMatchImages() {

}

CudaMosaic::CudaMosaic() {
  numImages = 0;
  finalIndex = 0;
  numSlices = 0;
  cutSize = 0;

  cudaDeviceImageAverages = NULL;
  cudaDeviceAllAverages = NULL;
  cudaDeviceImageIndex = NULL;

  imageAverages = NULL;
  allAverages = NULL;
  imageIndex = NULL;
}

CudaMosaic::~CudaMosaic() {
  if (imageAverages) {
    delete [] imageAverages;
    delete [] allAverages;
    delete [] imageIndex;
  }

  if (cudaDeviceImageAverages) {
    delete [] cudaDeviceImageAverages;
    delete [] cudaDeviceAllAverages;
    delete [] cudaDeviceImageIndex;
  }
}

void CudaMosaic::setup() {
  int deviceCount = 0;
  cudaError_t err = cudaGetDeviceCount(&deviceCount);

  printf("Initializing CUDA for CudaRenderer\n");
  printf("Found %d CUDA devices\n", deviceCount);

  for (int i=0; i<deviceCount; i++) {
    cudaDeviceProp deviceProps;
    cudaGetDeviceProperties(&deviceProps, i);
    printf("Device %d: %s\n", i, deviceProps.name);
    printf("   SMs:        %d\n", deviceProps.multiProcessorCount);
    printf("   Global mem: %.0f MB\n", static_cast<float>(deviceProps.totalGlobalMem) / (1024 * 1024));
    printf("   CUDA Cap:   %d.%d\n", deviceProps.major, deviceProps.minor);
  }

  cudaMalloc(&cudaDeviceImageAverages, sizeof(float) * cutSize * 3 * (numSlices * numSlices));
  cudaMalloc(&cudaDeviceAllAverages, sizeof(float) * cutSize * 3 * numImages);
  cudaMalloc(&cudaDeviceImageIndex, sizeof(int) * numImages);

  cudaMemcpy(cudaDeviceImageAverages, imageAverages, sizeof(float) * cutSize * 3 * (numSlices * numSlices), cudaMemcpyHostToDevice);
  cudaMemcpy(cudaDeviceAllAverages, allAverages, sizeof(float) * cutSize * 3 * numImages, cudaMemcpyHostToDevice);
  cudaMemcpy(cudaDeviceImageIndex, imageIndex, sizeof(int) * numImages, cudaMemcpyHostToDevice);

  GlobalConstants params;
  params.numImages = numImages;
  params.finalIndex = finalIndex;
  params.numSlices = numSlices;
  params.cutSize = cutSize;
  params.imageAverages = cudaDeviceImageAverages;
  params.allAverages = cudaDeviceAllAverages;
  params.imageIndex = cudaDeviceImageIndex;

  cudaMemcpyToSymbol(cuConstMosaicParams, &params, sizeof(GlobalConstants));
}

void CudaMosaic::imageMatch() {
  dim3 threadsPerBock(cutSize, cutSize, 1);
  dim3 numBlocks(numSlices, numSlices, 1);

  kernelMatchImages<<<numBlocks, threadsPerBock>>>();
}















