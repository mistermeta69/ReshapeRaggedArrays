#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>
#include "pch.h"
//#define LDD_LIB_EXPORT 1
#include "ReshapeRaggedArrays.h"
//
// returns total length after elimination of "NaN's"
// ready for reshaping into a 1D array
// (compress)
//

namespace RAGGED_LIB
{
	namespace ReshapeRaggedArrays
	{
		template <typename T>
		inline long  RectToRaggedLinear(const T * A, const int d1, const int d2, const int16_t  * lens)
		{
			long dstIdx = (long) lens[0];
			for (long i = 1; i < d1; i++) {
				memmove((void*) &(A[dstIdx]), (void*) &(A[i * d2]), lens[i] * sizeof(T));		//shift over a-line by a-line
				dstIdx += (long) lens[i];
			}
			return dstIdx;
		}


		// reshapes linear array into rectangular - ready for LV reshaping
		// (expand)
		template <typename T>
		inline long RaggedLinearToRect(T *  A, const int d1, const int d2, const int16_t  *  lens, const T filler)
		{
			long fcnt = 0;
			long srcIdx = 0;
			for (long i = 0; i < d1; i++) {
				srcIdx += lens[i];
			}

			for (long i = d1 - 1; i >= 0; i--) {		//traverse backwards, expanding into buffer
				srcIdx -= lens[i];
				T *  dst = (T*)memmove((void*) &(A[i*d2]), (void*)&(A[srcIdx]), lens[i] * sizeof(T));
				for (long j = lens[i]; j < d2; j++) {
					dst[j] = filler;			//A[i*d2 + j] = filler;
					fcnt++;
				}
			}
			return fcnt;
		}




		long fRectToRaggedLinear(const void *A, const int d1, const int d2, const int16_t  *lens)
		{
			for (int i = 0; i < d1; i++)
				if (lens[i] > d2 || lens[i]<0) return -1;
			return RectToRaggedLinear((const float*)A, d1, d2, lens);
		}

		long iRectToRaggedLinear(const void *A, const int d1, const int d2, const int16_t  *lens)
		{
			for (int i = 0; i < d1; i++)
				if (lens[i] > d2 || lens[i]<0) return -1;
			return RectToRaggedLinear((const int16_t *)A, d1, d2, lens);
		}

		long fRaggedLinearToRect(const void*A, const int d1, const int d2, const int16_t * lens, const float filler)
		{
			for (int i = 0; i < d1; i++)
				if (lens[i] > d2 || lens[i]<0) return -1;
			return RaggedLinearToRect((float*)A, d1, d2, lens, filler);
		}

		long iRaggedLinearToRect(const void*A, const int d1, const int d2, const int16_t * lens, const int16_t  filler)
		{
			for (int i = 0; i < d1; i++)
				if (lens[i] > d2 || lens[i]<0) return -1;
			return RaggedLinearToRect((int16_t *)A, d1, d2, lens, (int16_t )filler);
		}

#ifdef TEST
		const int D1 = 1000;
		const int D2 = 448;
		float aTf[D1][D2];
		int aTi[D1][D2];
		int aL[D1];

		template <typename T>
		int TestRagged(T *at, int *al, const int d1, const int d2, const T filler);

		int main()
		{
			//test
			int err1 = TestRagged((float*)aTf, (int*)aL, D1, D2, NAN);
			int err2 = TestRagged((int*)aTi, (int*)aL, D1, D2, -1);

			return err1 + err2;
		}

		//returns mismatches...
		// or 0 if success
		//
		template <typename T>
		int TestRagged(T *at, int *al, const int d1, const int d2, const T filler)
		{
			long len = d2;
			for (long i = 0; i < d1; i++) {
				for (long j = 0; j < d2; j++) {
					at[i*d2 + j] = (T)(i + j);
				}
				al[i] = len--; if (len == 0) len = d2;
			}
			long r = RectToRaggedLinear(at, d1, d2, al);
			RaggedLinearToRect(at, d1, d2, al, filler);
			int errcnt = 0;
			for (long i = 0; i < d1; i++) {
				for (long j = 0; j < al[i]; j++) {
					if (at[i*d2 + j] != (T)(i + j)) {
						errcnt++;
					}
				}
			}
			return errcnt;
		}
#endif
#ifdef OLD
		//
		// Reads and writes any size (up to 64 bit counter) and dimension arrays to disk, 
		//		selected data types, compressing special character repetitions 
		// optimally, e.g.  
		//		array of chars, get rid of blank reps, 8 bit counter means 3 or more reps before compression
		//		array of int32's, get rid of -1 reps, 32 bit counter means 5 or more reps before compression
		//		array of doubles, get rid of NaN reps, 64 bit counter means 9 or more reps before compression
		// parameters:  
		//		int		element size in bytes, 
		//		var		rep element value to look for,  
		//		long	size of array in elements
		//
		//


		//  shrinks the array in place before writing to disk, without using meta chars, only repetitions flag runs for expansion
		//  converse operation expands in place when it sees a rep beyond the min
		//  meta data is saved to allow pre-allocation etc
		//
		// designed to be callable from Labview - so can be called in two phases to get size meta data when reading, to pre-allocate dest.
		// todo:
		// - look into non-blocking
		// - test LV calls and timing, unit tests...
		// - make into multifile calls - like TDMS, with meta data blocks/headers with general purpose data/channel names
		//
		// File format:
		struct MetaData {
			int dataTypeLen;					//in bytes, 1,2,4,8
			void* specialValue;					//to be cast based on type (UINT8,16,32,64)
			int numDimensions;
			long *dimensions;					//array allocated in LV
		};
		//
		struct InternalMetaData {							//variable length? see how to return filename...
			long metaDataLen;							//in bytes
			int dataTypeLen;							//in bytes, 1,2,4,8
			int numDimensions;
			long *dimensions;
			int *filenameLen;
			char *fileName;
			char *dataBlob;
		};
		//
		// API:
		int SaveArray(const char *filename, const MetaData md, const void *blob);			// returns error or 0, blob is the buffer ptr from LV
		int ReadMetaData(const char* filename, MetaData *md);
		int ReadArray(const char *filename, const void *blob);		// returns error or 0, blob is the pre-allocated buffer ptr from LV

// fwd decl's
		int writeHeader(int fd, const char *fileName, const MetaData md);
		int writeBlob(int fd, const long compBlobLen, const char* blob, const MetaData md);
		const long compressInPlace(const char* blob, const int dataTypeLen, const long blobLen);
		const long expandInPlace(const char *blob, const int dataTypeLen, const long blobLen);

		// top level
		//
		int SaveArray(const char *fileName, const MetaData md, const void* blob)
		{
			long blobLen = md.dataTypeLen;
			for (int i = 0; i < md.numDimensions; i++) {
				blobLen *= md.dimensions[i];
			}
			const long compBlobLen = compressInPlace((char*)blob, md.dataTypeLen, blobLen);
			int fd = _open(fileName, _O_CREAT | _O_BINARY | _O_SEQUENTIAL | _O_APPEND, _S_IWRITE);
			int retval = 0;
			if (-1 != fd) {
				if (-1 != writeHeader(fd, fileName, md)) {
					retval = writeBlob(fd, compBlobLen, (char*)blob, md);
				}
			}
			_close(fd);
			return retval;
		}


		// write the fixed size header
		// with padding for the future
		//
		int writeHeader(int fd, const char *fileName, const MetaData md)
		{
			int retval = 0;
			int hlen = sizeof(int)		//includes hlen
				+ strlen(fileName)
				+ sizeof(md.dataTypeLen)
				+ sizeof(md.numDimensions)
				+ sizeof(md.numDimensions * sizeof(long));		//dimensions can be bigger than int
			int paddingLen = 4096 - hlen;

			if (paddingLen < 0) {
				retval |= -1;
				goto done;
			}
			char *padding = (char*)malloc(paddingLen);
			retval |= _write(fd, &hlen, sizeof(hlen));
			retval |= _write(fd, &(md.dataTypeLen), sizeof(md.dataTypeLen));
			retval |= _write(fd, &(md.numDimensions), sizeof(md.numDimensions));
			retval |= _write(fd, md.dimensions, sizeof(md.numDimensions * sizeof(long)));
			retval |= _write(fd, padding, paddingLen);

		done:
			return retval;
		}

		// todo - use templates...
		// write the run length meta data and shift the array
		//
		UINT64 inline shiftRunLong(const char* p, UINT64 shiftlen, UINT64 curlen)
		{
			// all lens are in bytes
			int runLen = 3 * sizeof(UINT64);			//rep of 3 indicates a run
			int metaLen = sizeof(UINT64) + runLen;		//4 longs includes the counter
			memcpy((void*)(p + runLen),
				&shiftlen,
				sizeof(shiftlen));		//place the run len after the run of 3
			memmove((void*)(p + metaLen),
				(void*)(p + metaLen + shiftlen),
				curlen - shiftlen);		//shift the array
			return curlen - shiftlen;
		}

		UINT64 inline shiftRunInt(const char* p, UINT32 shiftlen, UINT64 curlen)
		{
			// all lens are in bytes
			int runLen = 3 * sizeof(UINT32);			//rep of 3 indicates a run
			int metaLen = sizeof(UINT32) + runLen;		//includes the counter 
			memcpy((void*)(p + runLen),
				&shiftlen,
				sizeof(shiftlen));		//place the run len after the run of 3
			memmove((void*)(p + metaLen),
				(p + metaLen + shiftlen),
				curlen - shiftlen);		//shift the array
			return curlen - shiftlen;
		}

		// compresses the blob reducing any repetitions that exceed the compressed size sizeof(counter)+sizeof(dataTypeLen)
		//
		const long compressInPlace(const char* blob, const int dataTypeLen, long blobLen)
		{
			long curLen = blobLen;
			int eqCnt;
			bool goodRun;
			switch (dataTypeLen) {

				return curLen;
			error:
				return -1;
			}
		}


		// writes the (compressed) array data as a blob of chars
		//
		int writeBlob(int fd, const long blobLen, const char* blob, const MetaData md)
		{
			int blobLenLeft = blobLen;
			int chunkLen = 2 ^ (sizeof(int) * 8 - 1);		//31 bits

			for (long offset = 0; offset < blobLen; offset += chunkLen) { // write in chunks 

				int lenToWrite = (blobLenLeft > chunkLen) ? (chunkLen) : (blobLenLeft);
				blobLenLeft -= lenToWrite;

				int retVal = _write(fd, blob + offset, lenToWrite);
				if (retVal == -1 || retVal != lenToWrite)
					goto errexit;

			}
			return 0;
		errexit:
			return -1;
		}

		// reads the meta data from file so that LV can allocate arrays to make the call to receive blob data
		// returns 0 or -1 if error
		//
		int ReadMetaData(const char* filename, MetaData *md)
		{
			int retval = 0;
			return retval;
		}

		int ReadArray(const char *filename, const void *blob)
		{
			int retval = 0;
			return retval;
		}


		// expands in place (assumes *blob is big enough) using reverse algo to compressInPlace
		//
		const long expandInPlace(const char* blob, const int dataTypeLen, const long blobLen)
		{
			long compBlobLen = blobLen;

			return compBlobLen;
		}

#endif
	}
}
