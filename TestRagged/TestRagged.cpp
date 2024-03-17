//#undef LDD_LIB_EXPORT
#include "TestRagged.h"
namespace RAGGED_LIB
{

	const int D1 = 1000;
	const int D2 = 448;
	float aTf[D1][D2];
	int aTi[D1][D2];
	int aL[D1];

	// returns mismatches...
	//  or 0 if success
	//

	int fTestRagged(float *at, int *al, const int d1, const int d2, const float filler)
	{
		long len = d2;
		for (long i = 0; i < d1; i++)
		{
			for (long j = 0; j < d2; j++)
			{
				at[i * d2 + j] = (float)(i + j);
			}
			al[i] = len--;
			if (len == 0)
				len = d2;
		}
		long r = fRectToRaggedLinear(at, d1, d2, al);
		fRaggedLinearToRect(at, d1, d2, al, filler);
		int errcnt = 0;
		for (long i = 0; i < d1; i++)
		{
			for (long j = 0; j < al[i]; j++)
			{
				if (at[i * d2 + j] != (float)(i + j))
				{
					errcnt++;
				}
			}
		}
		return errcnt;
	}

	int iTestRagged(int *at, int *al, const int d1, const int d2, const int filler)
	{
		long len = d2;
		for (long i = 0; i < d1; i++)
		{
			for (long j = 0; j < d2; j++)
			{
				at[i * d2 + j] = (int)(i + j);
			}
			al[i] = len--;
			if (len == 0)
				len = d2;
		}
		long r = iRectToRaggedLinear(at, d1, d2, al);
		iRaggedLinearToRect(at, d1, d2, al, filler);
		int errcnt = 0;
		for (long i = 0; i < d1; i++)
		{
			for (long j = 0; j < al[i]; j++)
			{
				if (at[i * d2 + j] != (int)(i + j))
				{
					errcnt++;
				}
			}
		}
		return errcnt;
	}

	int main()
	{
		// test
		int err1 = fTestRagged((float *)aTf, (int *)aL, D1, D2, NAN);
		int err2 = iTestRagged((int *)aTi, (int *)aL, D1, D2, -1);

		return err1 + err2;
	}
}