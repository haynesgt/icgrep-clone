#include <string>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <toolchain/toolchain.h>
#include "cuda.h"
#include "cuda_runtime.h"

#define GROUPTHREADS 64

void checkCudaErrors(CUresult err) {
  assert(err == CUDA_SUCCESS);
}

struct PtxBundle {
  CUfunction function;
  cudaStream_t stream;
  CUdeviceptr devBufferOutput;
};

/// main - Program entry point
ulong * RunPTX(const std::vector<std::string> & ptx_files, char * fileBuffer, ulong filesize, bool CountOnly, std::vector<size_t> LFPositions, ulong * startPoints, ulong * accumBytes, int REn) {
  
  CUdevice    device;
  CUcontext   context;
  std::vector<PtxBundle> bundles;
  int         devCount;

  CUmodule cudaModule;

  // CUDA initialization
  checkCudaErrors(cuInit(0));
  checkCudaErrors(cuDeviceGetCount(&devCount));
  checkCudaErrors(cuDeviceGet(&device, 0));

  char name[128];
  checkCudaErrors(cuDeviceGetName(name, 128, device));
  std::cout << "Using CUDA Device [0]: " << name << "\n";

  int devMajor, devMinor;
  checkCudaErrors(cuDeviceComputeCapability(&devMajor, &devMinor, device));
  //checkCudaErrors(cuDeviceGetAttribute(&devMajor, CUdevice_attribute::CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, device));
  // std::cout << "Device Compute Capability: " << devMajor << "." << devMinor << "\n";
  if (devMajor < 2) {
    std::cerr << "ERROR: Device 0 is not SM 2.0 or greater\n" << devMajor << "\n";
  //  exit(-1);
  }

  // Create driver context
  checkCudaErrors(cuCtxCreate(&context, 0, device));

  CUfunction  function;

  for (int REi = 0; REi < REn; REi++) {
    std::ifstream t(ptx_files.at(REi));
    if (!t.is_open()) {
      std::cerr << "Error: cannot open " << ptx_files.at(REi) << " for processing. Skipped.\n";
      exit(-1);
    }

    std::string ptx_str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    // Create module for object
    checkCudaErrors(cuModuleLoadDataEx(&cudaModule, ptx_str.c_str(), 0, 0, 0));

    cudaStream_t stream;

    // Get kernel function
    checkCudaErrors(cuModuleGetFunction(&function, cudaModule, (std::string("Main") + std::to_string(REi)).c_str() ));
    std::cout << "Found Function: " << REi << "\n";

    cudaStreamCreate(&stream);

    PtxBundle bundle;
    bundle.function = function;
    bundle.stream = stream;
    bundles.push_back(bundle);
  }

  std::cout << "Found kernel functions\n";

  // Device data
  CUdeviceptr devBufferInput;
  CUdeviceptr devStartPoints;
  CUdeviceptr devBufferSizes;

  int groupSize = GROUPTHREADS * sizeof(ulong) * 8;
  const unsigned numOfGroups = codegen::GroupNum;

  if(LFPositions.size() < numOfGroups){
    std::cerr << "Number of line Breaks:" << LFPositions.size() << std::endl;
    std::cerr << "Number of GPU groups:" << numOfGroups << std::endl;
    std::cerr << "Line breaks must be more than GPU groups. Use -group-num option to change the group size." << std::endl;
    exit(-1);
  }

  unsigned avg = LFPositions.size()/numOfGroups;
  unsigned left = LFPositions.size()%numOfGroups;

  size_t divPoints[numOfGroups + 1];
  size_t bufferSizes[numOfGroups];
  divPoints[0] = 0;
  startPoints[0] = 0;
  unsigned i = 1;
  unsigned pos = 0;
  while (i < numOfGroups){
    if (i < left)
      pos += avg + 1;
    else
      pos += avg;

    divPoints[i] = LFPositions[pos]+1;
    bufferSizes[i-1] = divPoints[i]-divPoints[i-1];
    startPoints[i] = startPoints[i-1] + ((bufferSizes[i-1]-1)/groupSize+1)*groupSize;

    i++;
  }

  divPoints[numOfGroups] = filesize;
  bufferSizes[i-1] = divPoints[i]-divPoints[i-1];
  startPoints[i] = startPoints[i-1] + ((bufferSizes[i-1]-1)/groupSize+1)*groupSize;
   
  checkCudaErrors(cuMemAlloc(&devBufferInput, startPoints[numOfGroups]));
  checkCudaErrors(cuMemsetD8(devBufferInput,0,startPoints[numOfGroups]));
  checkCudaErrors(cuMemAlloc(&devStartPoints, sizeof(ulong) * (numOfGroups + 1)));
  checkCudaErrors(cuMemAlloc(&devBufferSizes, sizeof(ulong) * numOfGroups));

  int outputSize = 0;
  if (CountOnly){
    outputSize = sizeof(ulong) * GROUPTHREADS * numOfGroups;
  }
  else{
    outputSize = startPoints[numOfGroups]/4;
  }


  //Copy from host to device
  for(unsigned i=0; i<numOfGroups; i++){
    checkCudaErrors(cuMemcpyHtoD(devBufferInput+startPoints[i], fileBuffer+divPoints[i], bufferSizes[i]));
  }
  checkCudaErrors(cuMemcpyHtoD(devStartPoints, startPoints, sizeof(ulong) * (numOfGroups + 1)));
  checkCudaErrors(cuMemcpyHtoD(devBufferSizes, bufferSizes, sizeof(ulong) * numOfGroups));

  unsigned blockSizeX = GROUPTHREADS;
  unsigned blockSizeY = 1;
  unsigned blockSizeZ = 1;
  unsigned gridSizeX  = numOfGroups;
  unsigned gridSizeY  = 1;
  unsigned gridSizeZ  = 1;


  std::cerr << "Launching kernel\n";

  CUevent start;
  CUevent stop;
  float elapsedTime;

  cuEventCreate(&start, CU_EVENT_BLOCKING_SYNC);
  cuEventRecord(start,0);

  // Kernel launch
  for (PtxBundle & bundle : bundles) {

    CUdeviceptr devBufferOutput;
    checkCudaErrors(cuMemAlloc(&devBufferOutput, outputSize));

    // Kernel parameters
    void *KernelParams[] = { &devBufferInput, &devStartPoints, &devBufferSizes, &devBufferOutput};

    checkCudaErrors(cuLaunchKernel(bundle.function, gridSizeX, gridSizeY, gridSizeZ,
                                   blockSizeX, blockSizeY, blockSizeZ,
                                   0, bundle.stream, KernelParams, NULL));

    bundle.devBufferOutput = devBufferOutput;
  }
  std::cerr << "kernel success.\n";

  cuEventCreate(&stop, CU_EVENT_BLOCKING_SYNC);
  cuEventRecord(stop,0);
  cuEventSynchronize(stop);

  cuEventElapsedTime(&elapsedTime, start, stop);
  printf("GPU Kernel time : %f ms\n" ,elapsedTime);

  // Retrieve device data
  ulong * matchRslt;
  if (posix_memalign((void**)&matchRslt, 32, outputSize)) {
    std::cerr << "Cannot allocate memory for output.\n";
    exit(-1);
  }

  int count = 0;

  for (PtxBundle const & bundle : bundles) {
    checkCudaErrors(cuMemcpyDtoH(matchRslt, bundle.devBufferOutput, outputSize));

    if (CountOnly){
      for (unsigned i = 0; i < GROUPTHREADS * numOfGroups; ++i) {
        // std::cout << i << ":" << matchRslt[i] << "\n";
        count += matchRslt[i];
      }
    }
    else{
      for(unsigned i=0; i<=numOfGroups; i++){
        accumBytes[i] = startPoints[i] - divPoints[i];
      }
    }
  }
  if (CountOnly) {
    std::cout << count << "\n";
  }


  // Clean-up
  checkCudaErrors(cuMemFree(devBufferInput));
  checkCudaErrors(cuMemFree(devStartPoints));
  checkCudaErrors(cuMemFree(devBufferSizes));
  for (PtxBundle const & bundle : bundles) {
    cudaStreamDestroy(bundle.stream);
    checkCudaErrors(cuMemFree(bundle.devBufferOutput));
  }
  checkCudaErrors(cuCtxDestroy(context));

  return matchRslt;
}
