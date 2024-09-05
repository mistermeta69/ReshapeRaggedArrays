#pragma once
#include <math.h>
//#ifdef LIB_EXPORT
//#define NATIVE_DLL_EXPORT __declspec(dllexport)
//#else
//#define NATIVE_DLL_EXPORT __declspec(dllimport)
//#endif
#define NATIVE_DLL_EXPORT

namespace RAGGED_LIB
{

#ifdef __cplusplus
	extern "C"
	{
#endif
		// Just compress/uncompress ragged arrays float or int A[d1][d2] in place
		// arrays must be pre-allocated by LV
		// if reshaping is expensive in LV (to a 1D), then write disk i/o here
		//

		NATIVE_DLL_EXPORT long fRectToRaggedLinear( // returns compressed total len (d1*d2)-num fillers
			const void *A,
			const int d1,
			const int d2,
			const int *lens);

		NATIVE_DLL_EXPORT long iRectToRaggedLinear( // returns compressed total len (d1*d2)-num fillers
			const void *A,
			const int d1,
			const int d2,
			const int *lens);

		NATIVE_DLL_EXPORT long fRaggedLinearToRect( // returns filler count as a check
			const void *A,
			const int d1,
			const int d2,
			const int *lens,
			const float filler = NAN);

		NATIVE_DLL_EXPORT long iRaggedLinearToRect( // returns filler count as a check
			const void *A,
			const int d1,
			const int d2,
			const int *lens,
			const int filler = -1);
	}
}
