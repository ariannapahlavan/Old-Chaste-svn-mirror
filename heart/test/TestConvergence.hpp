#ifndef TESTCONVERGENCE_HPP_
#define TESTCONVERGENCE_HPP_

#include <cxxtest/TestSuite.h>
#include "BidomainProblem.hpp"
#include "MonodomainProblem.hpp"
#include <petscvec.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <math.h>

#include "BackwardEulerLuoRudyIModel1991.hpp"
#include "LuoRudyIModel1991OdeSystem.hpp"
#include "PdeConvergenceTester.hpp"
#include "SpaceConvergenceTester.hpp"
#include "StimulusConvergenceTester.hpp"
#include "KspConvergenceTester.hpp"
#include "OdeConvergenceTester.hpp"


class TestConvergence : public CxxTest::TestSuite
{   
public:

    void RunConvergenceTester(AbstractUntemplatedConvergenceTester *pTester, bool stimulateRegion)
    {
            pTester->StimulateRegion=stimulateRegion;
            if ( stimulateRegion )
            {
                pTester->MeshNum = 6u;    
            }
            
            pTester->Converge();
            TS_ASSERT(pTester->Converged);
    }

    void ConvergeInVarious(bool stimulateRegion)
    {
       {
            PdeConvergenceTester<BackwardEulerLuoRudyIModel1991, BidomainProblem<1>, 1> tester;
            RunConvergenceTester(&tester, stimulateRegion);           
            TS_ASSERT_DELTA(tester.PdeTimeStep, 5.0e-3, 1e-10);
        }
    
        {
            SpaceConvergenceTester<BackwardEulerLuoRudyIModel1991, BidomainProblem<1>, 1> tester;
            RunConvergenceTester(&tester, stimulateRegion);   
            if (!stimulateRegion)
            {
                TS_ASSERT_EQUALS(tester.MeshNum, 5u); 
            }
            else
            {
                TS_ASSERT_EQUALS(tester.MeshNum, 6u);
            }
        }
            
        {
            KspConvergenceTester<BackwardEulerLuoRudyIModel1991, BidomainProblem<1>, 1> tester;
            RunConvergenceTester(&tester, stimulateRegion);    
            if (!stimulateRegion)
            {
                TS_ASSERT_DELTA(tester.KspRtol, 1e-5, 1e-10);
            }
            else
            {
                TS_ASSERT_DELTA(tester.KspRtol, 1e-6, 1e-10);
            }
        }
    
        {
            OdeConvergenceTester<LuoRudyIModel1991OdeSystem, BidomainProblem<1>, 1> tester;
            RunConvergenceTester(&tester, stimulateRegion);    
            TS_ASSERT_DELTA(tester.OdeTimeStep, 0.0025, 1e-10);
        }
        
        {
            OdeConvergenceTester<BackwardEulerLuoRudyIModel1991, BidomainProblem<1>, 1> tester;
            tester.PdeTimeStep=0.01;
            RunConvergenceTester(&tester, stimulateRegion);    
            TS_ASSERT_DELTA(tester.OdeTimeStep, 0.0025, 1e-10);
        }
        
    }
    
    void TestStimulatePlanein1D() throw(Exception)
    {
        ConvergeInVarious(false);
    }

    void TestStimulateRegionin1D() throw(Exception)
    {
        ConvergeInVarious(true);
    }
 
};

#endif /*TESTCONVERGENCE_HPP_*/
