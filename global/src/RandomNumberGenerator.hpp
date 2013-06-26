/*

Copyright (c) 2005-2013, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef RANDOMNUMBERGENERATORS_HPP_
#define RANDOMNUMBERGENERATORS_HPP_

#include <sstream>
#include <boost/shared_ptr.hpp>
#include <boost/random.hpp>

#include "ChasteSerialization.hpp"
#include "SerializableSingleton.hpp"
#include <boost/serialization/split_member.hpp>

/*Windows_Port_Begins*/
#ifdef _MSC_VER
#define srandom srand
#define random rand 
#endif
/*Windows_Port_Ends*/


/**
 * A special singleton class allowing one to generate different types of
 * random number in a globally consistent way.
 */
class RandomNumberGenerator : public SerializableSingleton<RandomNumberGenerator>
{
private:
    /** The main random number generator. **/
    boost::mt19937 mMersenneTwisterGenerator;

    /** An adaptor to a unit interval distribution. */
    boost::variate_generator<boost::mt19937& , boost::uniform_real<> > mGenerateUnitReal;

    /** An adaptor to a standard normal distribution. */
    boost::variate_generator<boost::mt19937& , boost::normal_distribution<> > mGenerateStandardNormal;

    /** Pointer to the single instance. */
    static RandomNumberGenerator* mpInstance;

    friend class boost::serialization::access;
    /**
     * Save the RandomNumberGenerator and its member variables.
     *
     * @note do not serialize this singleton directly.  Instead, serialize
     * the object returned by GetSerializationWrapper().
     *
     * @param archive the archive
     * @param version the current version of this class
     */
    template<class Archive>
    void save(Archive & archive, const unsigned int version) const
    {
        std::stringstream rng_internals;
        rng_internals << mMersenneTwisterGenerator;
        std::string rng_internals_string = rng_internals.str();
        archive & rng_internals_string;

        std::stringstream normal_internals;
        const boost::normal_distribution<>& r_normal_dist = mGenerateStandardNormal.distribution();
        normal_internals << r_normal_dist;
        std::string normal_internals_string = normal_internals.str();
        archive & normal_internals_string;
    }

    /**
     * Load the RandomNumberGenerator and its member variables.
     *
     * @note do not serialize this singleton directly.  Instead, serialize
     * the object returned by GetSerializationWrapper().
     *
     * @param archive the archive
     * @param version the current version of this class
     */
    template<class Archive>
    void load(Archive & archive, const unsigned int version)
    {
        std::string rng_internals_string;
        archive & rng_internals_string;
        std::stringstream rng_internals(rng_internals_string);
        rng_internals >> mMersenneTwisterGenerator;

        std::string normal_internals_string;
        archive & normal_internals_string;
        std::stringstream normal_internals(normal_internals_string);
        normal_internals >> mGenerateStandardNormal.distribution();
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

protected:

    /**
     * Protected constructor.
     * Use Instance() to access the random number generator.
     */
    RandomNumberGenerator();

public:

    /**
     *
     * @return a random number from the normal distribution with mean 0
     * and standard distribution 1.
     */
    double StandardNormalRandomDeviate();

    /**
     * @return Generate a random number from a normal distribution with given
     * mean and standard deviation.
     *
     * @param mean the mean of the normal distribution from which the random number is drawn
     * @param stdDev the standard deviation of the normal distribution from which the random number is drawn
     */
    double NormalRandomDeviate(double mean, double stdDev);

    /**
     * @return Generate a uniform random number in (0,1].
     */
    double ranf();

    /**
     * @return Generate a random number from a gamma distribution with specified shape and scale parameters.
     *
     * @param shape the shape parameter of the gamma distribution from which the random number is drawn
     * @param scale the scale parameter of the gamma distribution from which the random number is drawn
     */
    double GammaRandomDeviate(double shape, double scale);

    /**
     * @return Generate a random number modulo base (i.e. an integer
     * within the range [0, base) == [0,1,..,base-1] ).
     *
     * @param base the order of the field of positive integers from which the random number is drawn.  This should be no greater than INT_MAX
     */
    unsigned randMod(unsigned base);

    /**
     * Produce a permutation of the integers and non-empty std::vector, using the Knuth-algorithm
     * (also called the Fisher-Yates algorithm), a linear time unbiased method.
     * The values are shuffled in place.
     *
     * @param rValues  the initial values and the output permutation of shuffled values.  Must be non-empty
     */
    template <class T>
    void Shuffle(std::vector<boost::shared_ptr<T> >& rValues)
    {
        unsigned num = rValues.size();
        if (num == 0)
        {
            return;
        }
        for (unsigned end=num-1; end>0; end--)
        {
            // Pick a random integer from {0,..,end}
            unsigned k = RandomNumberGenerator::Instance()->randMod(end+1);
            boost::shared_ptr<T> temp = rValues[end];
            rValues[end] = rValues[k];
            rValues[k] = temp;
        }
    }

    /**
     * Produce a permutation of the integers 0,1,..,num-1, using the Knuth-algorithm
     * (also called the Fisher-Yates algorithm), a linear time unbiased method.
     * The shuffled values are returned in rValues, which doesn't need to
     * be correctly-sized when passed in.
     *
     * @param num  the number of integers to shuffle
     * @param rValues  the output permutation of shuffled values (any initial values ignored)
     */
    void Shuffle(unsigned num, std::vector<unsigned>& rValues);

    /**
     * @return a pointer to the random number generator object.
     * The object is created the first time this method is called.
     */
    static RandomNumberGenerator* Instance();

    /**
     * Destroy the current instance of the random number generator.
     * The next call to Instance will create a new instance and re-seed.
     * This method *must* be called before program exit, to avoid a memory
     * leak.
     */
    static void Destroy();

    /**
     * Reseed the random number generator.
     *
     * @param seed the new seed
     */
    void Reseed(unsigned seed);
};

#endif /*RANDOMNUMBERGENERATORS_HPP_*/
