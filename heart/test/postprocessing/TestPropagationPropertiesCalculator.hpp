/*

Copyright (c) 2005-2014, University of Oxford.
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


#ifndef _TESTPROPAGATIONPROPERTIESCALCULATOR_HPP_
#define _TESTPROPAGATIONPROPERTIESCALCULATOR_HPP_

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <cassert>
#include "PropagationPropertiesCalculator.hpp"


class TestPropagationPropertiesCalculator : public CxxTest::TestSuite
{
public:
    void TestConductionVelocity1D(void) throw (Exception)
    {
        Hdf5DataReader simulation_data("heart/test/data/Monodomain1d",
                                       "MonodomainLR91_1d", false);

        PropagationPropertiesCalculator ppc(&simulation_data);

        // Should throw because node 95 never crosses the threshold
        TS_ASSERT_THROWS_THIS(ppc.CalculateConductionVelocity(5,95,0.9),
                "AP did not occur, never descended past threshold voltage.");
        TS_ASSERT_THROWS_THIS(ppc.CalculateAllConductionVelocities(5,95,0.9)[0],
                "AP did not occur, never descended past threshold voltage.");

        // Should throw because AP is not complete here
        TS_ASSERT_THROWS_THIS(ppc.CalculateActionPotentialDuration(50,5), "No full action potential was recorded");

        // Should not throw because the upstroke propagated far enough in simulation time
        //for both methods of last conduction velocity and all of them
        TS_ASSERT_THROWS_NOTHING(ppc.CalculateConductionVelocity(20,40,0.1));
        TS_ASSERT_THROWS_NOTHING(ppc.CalculateAllConductionVelocities(20,40,0.1)[0]);

        //check some value
        TS_ASSERT_DELTA(ppc.CalculateConductionVelocity(20,40,0.2),0.0498,0.01);

        //check both methods return the same value
        TS_ASSERT_EQUALS(ppc.CalculateConductionVelocity(20,40,0.2),ppc.CalculateAllConductionVelocities(20,40,0.2)[0]);

        //other parameters
        TS_ASSERT_DELTA(ppc.CalculateMaximumUpstrokeVelocity(1),343.9429,0.001);

        TS_ASSERT_DELTA(ppc.CalculatePeakMembranePotential(5),23.6271,0.001);

    }

    void TestConductionVelocityWithRepeatedStimuli(void) throw (Exception)
    {
        Hdf5DataReader simulation_data("heart/test/data/Monodomain1d",
                                       "RepeatedStimuli", false);

        // These data were generated by a monodomain simulation of a 1D cable
        // of 100 nodes with a space step of 0.1mm.
        // Luo Rudy model with conductivity at 0.2 (unlike above test).
        // 405 ms were simulated. Two stimuli were applied at node 0, one at 2 ms
        // and the other at 402.
        // The file contains exported data at 3 nodes (5, 50 and 95).

         PropagationPropertiesCalculator ppc(&simulation_data);

         // Make sure that node 5 has 2 upstrokes, Node 50 and 95 only one
        TS_ASSERT_EQUALS (ppc.CalculateAllMaximumUpstrokeVelocities(5, -30.0).size(),2U);
        TS_ASSERT_EQUALS (ppc.CalculateAllMaximumUpstrokeVelocities(50, -30.0).size(),1U);
        TS_ASSERT_EQUALS (ppc.CalculateAllMaximumUpstrokeVelocities(95, -30.0).size(),1U);
        TS_ASSERT_EQUALS (ppc.CalculateUpstrokeTimes(5,  -30.0).size(),2U);
        TS_ASSERT_EQUALS (ppc.CalculateUpstrokeTimes(50, -30.0).size(),1U);
        TS_ASSERT_EQUALS (ppc.CalculateUpstrokeTimes(95, -30.0).size(),1U);

        TS_ASSERT_DELTA(ppc.CalculateUpstrokeTimes(5,-30.0)[0],2.5100,0.01);
        TS_ASSERT_DELTA(ppc.CalculateUpstrokeTimes(5,-30.0)[1],402.5100,0.01);
        TS_ASSERT_DELTA(ppc.CalculateUpstrokeTimes(50, -30.0)[0],7.1300,0.01);
        TS_ASSERT_DELTA(ppc.CalculateUpstrokeTimes(95, -30.0)[0],11.7600,0.01);

        // Checking the velocity from a node with 2 upstrokes to a node with one only.
        // The method checks for the last AP that reached both, i.e. second from lasta t node 5.
        TS_ASSERT_DELTA(ppc.CalculateConductionVelocity(5,95,0.9), 0.097, 0.003);

        // Then we test the calculator with calculations in both directions for both methods
        //i.e. from 5 to 95 is equals to the negative of from 95 to 5
        TS_ASSERT_EQUALS(ppc.CalculateConductionVelocity(5,95,0.45),-ppc.CalculateConductionVelocity(95,5,0.45));
        TS_ASSERT_EQUALS(ppc.CalculateAllConductionVelocities(5,95,0.45)[0],-ppc.CalculateAllConductionVelocities(95,5,0.45)[0]);

       // Then we check some values (should be rather uniform over the cable)
        TS_ASSERT_DELTA(ppc.CalculateConductionVelocity(5,50,0.45), 0.097, 0.003);
        TS_ASSERT_DELTA(ppc.CalculateConductionVelocity(50,95,0.45), 0.097, 0.003);

        //cover the case of conduction calculated from and to the same node, it should return 0
        TS_ASSERT_DELTA(ppc.CalculateConductionVelocity(5,5,0.0), 0.0, 0.000001);

        //faking half euclidian distance, check that it gets twice slower
        TS_ASSERT_DELTA(ppc.CalculateConductionVelocity(5,50,0.225), 0.048, 0.003);
        TS_ASSERT_DELTA(ppc.CalculateConductionVelocity(50,95,0.225), 0.048, 0.003);

        //Finally make sure that the method that gets all the conduction velocities returns the same answers
        TS_ASSERT_EQUALS(ppc.CalculateAllConductionVelocities(5,95,0.45)[0],ppc.CalculateConductionVelocity(5,95,0.45));
        TS_ASSERT_EQUALS(ppc.CalculateAllConductionVelocities(5,50,0.45)[0],ppc.CalculateConductionVelocity(5,50,0.45));

    }

    void TestConductionBidomain3D() throw (Exception)
    {
        //Note: these data files (from notforrelease/test/TestCardiacFastSlowProblem3D.hpp) are incomplete.
        unsigned middle_index = 14895U;
        unsigned rhs_index = 14910U;


        Hdf5DataReader simulation_data_fs("heart/test/data/BidomainFastSlow3D",
                                       "res", false);
        PropagationPropertiesCalculator properties_fs(&simulation_data_fs);

        /*
         * From Raf's Matlab/Octave script
        r4256_Bi_FastSlow_3d
        Mid APD90: 229.7
        Mid dv/dt_max: 180.28
        Mid dv/dt time: 3.2
        RHS dv/dt time: 5.8
        CV: 0.057692
        *
        */
        TS_ASSERT_DELTA(properties_fs.CalculateActionPotentialDuration(90, middle_index), 230.3917, 0.25);
        TS_ASSERT_DELTA(properties_fs.CalculateConductionVelocity(middle_index, rhs_index, 0.15), 0.057692, 0.001);
        TS_ASSERT_DELTA(properties_fs.CalculateMaximumUpstrokeVelocity(middle_index), 180.28, 0.01);

        //Testing the method that returns all APs
        TS_ASSERT_EQUALS(properties_fs.CalculateActionPotentialDuration(90, middle_index), properties_fs.CalculateAllActionPotentialDurations(90, middle_index, -30.0)[0]);

        //Throws because the percentage "0.9%" looks too small and is likely to be a mistake
        TS_ASSERT_THROWS_THIS(properties_fs.CalculateActionPotentialDuration(0.9, middle_index),
                "First argument of CalculateActionPotentialDuration() is expected to be a percentage");

        //Throws because the percentage "100%" looks too big.
        TS_ASSERT_THROWS_THIS(properties_fs.CalculateActionPotentialDuration(100.0, middle_index),
                "First argument of CalculateActionPotentialDuration() is expected to be a percentage");

        Hdf5DataReader simulation_data_bw("heart/test/data/BidomainBackwardToCompareWithFastSlow3D",
                                       "res", false);
        PropagationPropertiesCalculator properties_bw(&simulation_data_bw);
        /*
         * From Raf's Matlab/Octave script
        r4256_Bi_Back_3d
        Mid APD90: 229.2
        Mid dv/dt_max: 173.02
        Mid dv/dt time: 3.3
        RHS dv/dt time: 6
        CV: 0.055556
        *
        */
        TS_ASSERT_DELTA(properties_bw.CalculateActionPotentialDuration(90, middle_index), 229.9328, 0.25);
        TS_ASSERT_DELTA(properties_bw.CalculateConductionVelocity(middle_index, rhs_index, 0.15), 0.055556, 0.001);
        TS_ASSERT_DELTA(properties_bw.CalculateMaximumUpstrokeVelocity(middle_index), 173.02, 0.01);

        //Testing the method that returns all APs
        TS_ASSERT_EQUALS(properties_bw.CalculateActionPotentialDuration(90, middle_index), properties_bw.CalculateAllActionPotentialDurations(90, middle_index, -30.0)[0]);
    }

    void TestUpstrokeBidomain3D()
    {
        Hdf5DataReader mono_fs_reader("heart/test/data/MonodomainFastSlow3D", "res", false);
        //std::vector<double> voltage_fast_slow=mono_fs_reader.GetVariableOverTime("V", mMiddleIndex);
        //DumpVector(voltage_fast_slow, "middle_node_mfs.dat");

        PropagationPropertiesCalculator ppc_fs(&mono_fs_reader);

        TS_ASSERT_DELTA(ppc_fs.CalculateMaximumUpstrokeVelocity(14895U), 174.9, 4.0);
        TS_ASSERT_DELTA(ppc_fs.CalculateActionPotentialDuration(90, 14895U), 231.739, 1.0);

        //Testing the mtehod that returns all APs
        TS_ASSERT_EQUALS(ppc_fs.CalculateActionPotentialDuration(90, 14895U), ppc_fs.CalculateAllActionPotentialDurations(90, 14895U, -30.0)[0]);

        Hdf5DataReader mono_bw_reader("heart/test/data/MonodomainBackwardToCompareWithFastSlow3D", "res", false);
        //std::vector<double> voltage_fast_slow=mono_fs_reader.GetVariableOverTime("V", mMiddleIndex);
        //DumpVector(voltage_fast_slow, "middle_node_mfs.dat");

        PropagationPropertiesCalculator ppc_bw(&mono_bw_reader);

        TS_ASSERT_DELTA(ppc_bw.CalculateMaximumUpstrokeVelocity(14895U), 174.9, 4.0);
        TS_ASSERT_DELTA(ppc_bw.CalculateActionPotentialDuration(90, 14895U), 230.25, 1.0);
    }

    void TestAPDCalculatorOverMultipleNodes()
    {
        Hdf5DataReader mono_fs_reader("heart/test/data/Monodomain1dFullActionPotential", "Monodomain1dFullActionPotential", false);

        PropagationPropertiesCalculator ppc_fs(&mono_fs_reader);

        // hardcoded
        TS_ASSERT_DELTA(ppc_fs.CalculateActionPotentialDuration(90, 5u), 331.1516, 1e-3);

        // Testing the method that returns all APs (for a given node)
        TS_ASSERT_EQUALS(ppc_fs.CalculateActionPotentialDuration(90, 5),
                         ppc_fs.CalculateAllActionPotentialDurations(90, 5u, -30.0)[0]);

        // Testing the method that returns all APs over a range over nodes
        std::vector<std::vector<double> > all_aps_for_node_range
            = ppc_fs.CalculateAllActionPotentialDurationsForNodeRange(90, 1u, 7u, -30.0);

        TS_ASSERT_EQUALS(all_aps_for_node_range.size(), 6u);
        for(unsigned i=0; i<all_aps_for_node_range.size(); i++)
        {
            TS_ASSERT_EQUALS(all_aps_for_node_range[i].size(), 1u);
            //std::cout << "Node " << i+1 << ", APD = " << all_aps_for_node_range[i][0] << "\n";
        }

        // verify that the APDs for two of the nodes are definitely different
        assert(all_aps_for_node_range[4][0] != all_aps_for_node_range[5][0]);

        // Check that the CalculateAllActionPotentialDurationsForNodeRange() results
        // agrees with the single-node version, for both these nodes
        TS_ASSERT_EQUALS(ppc_fs.CalculateAllActionPotentialDurations(90, 5u, -30.0)[0],
                         all_aps_for_node_range[4][0]);

        TS_ASSERT_EQUALS(ppc_fs.CalculateAllActionPotentialDurations(90, 6u, -30.0)[0],
                         all_aps_for_node_range[5][0]);

    }

    void TestEadCalculation() throw(Exception)
    {
       Hdf5DataReader ead_file("heart/test/data/PostProcessingWriter", "Ead", false);
       PropagationPropertiesCalculator ead_calc(&ead_file);

       TS_ASSERT_EQUALS(ead_calc.CalculateAllAboveThresholdDepolarisations(97u,-40.0)[0],1u);
       TS_ASSERT_EQUALS(ead_calc.CalculateAllAboveThresholdDepolarisations(97u,-40.0)[1],0u);
       TS_ASSERT_EQUALS(ead_calc.CalculateAllAboveThresholdDepolarisations(97u,-40.0)[2],3u);
       TS_ASSERT_EQUALS(ead_calc.CalculateAllAboveThresholdDepolarisations(97u,-40.0)[3],0u);

       TS_ASSERT_EQUALS(ead_calc.CalculateAboveThresholdDepolarisationsForLastAp(97u,-40.0), 0u);
     }
};

#endif //_TESTPROPAGATIONPROPERTIESCALCULATOR_HPP_
