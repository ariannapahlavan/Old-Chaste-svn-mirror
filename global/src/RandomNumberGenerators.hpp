#ifndef RANDOMNUMBERGENERATORS_HPP_
#define RANDOMNUMBERGENERATORS_HPP_
#include <cmath>
#include <time.h>
#include <stdlib.h>

class RandomNumberGenerators
{ 
public:
	double StandardNormalRandomDeviate(void);
	double NormalRandomDeviate(double mean, double sd);
	double ranf(void);
	int randMod(int base);
	RandomNumberGenerators()
	{
		srandom(0);
	}

};
#endif /*RANDOMNUMBERGENERATORS_HPP_*/
