// Copyright 2014 Isis Innovation Limited and the authors of InfiniTAM

#pragma once

#include "Vector.h"
#include "CUDADefines.h"

#include <stdlib.h>
#include <string.h>

namespace ORUtils
{
	/** \brief
	    Represents images, templated on the pixel type
	*/
	template <typename T>
	class Image
	{
	private:
		bool allocateGPU;
		int isAllocated;

		/** Pointer to memory on CPU host. */
		T* data_host;
		/** Pointer to memory on GPU, if available. */
		T* data_device;
	public:
		/** Size of the image in pixels. */
		Vector2i noDims;
		/** Total number of pixels allocated. */
		int dataSize;

		/** Get the data pointer on CPU or GPU. */
		inline T* GetData(bool useGPU) { return useGPU ? data_device : data_host; }

		/** Get the data pointer on CPU or GPU. */
		inline const T* GetData(bool useGPU) const { return useGPU ? data_device : data_host; }

		/** Initialize an empty 0x0 image, either on CPU only
		    or on both CPU and GPU.
		*/
		explicit Image(bool allocateGPU = false)
		{
			this->isAllocated = false;
			this->noDims.x = this->noDims.y = 0;
			this->allocateGPU = allocateGPU;
		}

		/** Initialize an empty image of the given size, either
		    on CPU only or on both CPU and GPU.
		*/
		Image(Vector2i noDims, bool allocateGPU = false)
		{
			this->isAllocated = false;
			this->allocateGPU = allocateGPU;
			Allocate(noDims);
			this->Clear();
		}

		/** Allocate image data of the specified size. If the
		    image has been allocated before, nothing is done,
		    irrespective of size.
		*/
		void Allocate(Vector2i noDims)
		{
			if (!this->isAllocated) {
				this->noDims = noDims;
				dataSize = noDims.x * noDims.y;

				if (allocateGPU)
				{
#ifndef COMPILE_WITHOUT_CUDA
					ORcudaSafeCall(cudaMallocHost((void**)&data_host, dataSize * sizeof(T)));
					ORcudaSafeCall(cudaMalloc((void**)&data_device, dataSize * sizeof(T)));
#endif
				}
				else
				{ data_host = new T[dataSize]; }
			}

			isAllocated = true;
		}

		/** Set all image data to the given @p defaultValue. */
		void Clear(unsigned char defaultValue = 0) 
		{ 
			memset(data_host, defaultValue, dataSize * sizeof(T)); 
#ifndef COMPILE_WITHOUT_CUDA
			if (allocateGPU) ORcudaSafeCall(cudaMemset(data_device, defaultValue, dataSize * sizeof(T)));
#endif
		}

		/** Resize an image, loosing all old image data.
		    Essentially any previously allocated data is
		    released, new memory is allocated.
		*/
		void ChangeDims(Vector2i newDims)
		{
			if ((newDims != noDims)||(!isAllocated)) {
				Free();
				Allocate(newDims);
			}
		}

		/** Transfer data from CPU to GPU, if possible. */
		void UpdateDeviceFromHost() {
#ifndef COMPILE_WITHOUT_CUDA
			if (allocateGPU) ORcudaSafeCall(cudaMemcpy(data_device, data_host, dataSize * sizeof(T), cudaMemcpyHostToDevice));
#endif
		}
		/** Transfer data from GPU to CPU, if possible. */
		void UpdateHostFromDevice() {
#ifndef COMPILE_WITHOUT_CUDA
			if (allocateGPU) ORcudaSafeCall(cudaMemcpy(data_host, data_device, dataSize * sizeof(T), cudaMemcpyDeviceToHost));
#endif
		}

		/** Copy image content, does not resize image! */
		void SetFrom(const Image<T> *source, bool copyHost = true, bool copyDevice = false)
		{
			if (copyHost) memcpy(this->data_host, source->data_host, source->dataSize * sizeof(T));
#ifndef COMPILE_WITHOUT_CUDA
			if (copyDevice) ORcudaSafeCall(cudaMemcpy(this->data_device, source->data_device, source->dataSize * sizeof(T), cudaMemcpyDeviceToDevice));
#endif
		}

		/** Release allocated memory for this image */
		void Free()
		{
			if (this->isAllocated) {
				if (allocateGPU) {
#ifndef COMPILE_WITHOUT_CUDA
					ORcudaSafeCall(cudaFree(data_device)); 
					ORcudaSafeCall(cudaFreeHost(data_host)); 
#endif
				}
				else delete[] data_host;
			}

			this->isAllocated = false;
		}

		~Image() { this->Free(); }

		// Suppress the default copy constructor and assignment operator
		Image(const Image&);
		Image& operator=(const Image&);
	};
}