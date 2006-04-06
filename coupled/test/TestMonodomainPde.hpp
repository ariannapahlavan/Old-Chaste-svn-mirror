#ifndef _TESTMONODOMAINPDE_HPP_
#define _TESTMONODOMAINPDE_HPP_

//#include <iostream>
#include <vector>
#include "InitialStimulus.hpp"
#include "EulerIvpOdeSolver.hpp"

#include "LuoRudyIModel1991OdeSystem.hpp"
#include "MonodomainPdeIteration7.hpp"

#include "OdeSolution.hpp"
#include "PetscSetupAndFinalize.hpp"
#include <petsc.h>

 
#include <cxxtest/TestSuite.h>

class TestMonodomainPde : public CxxTest::TestSuite
{
    public:    
    void testMonodomainPde( void )
    {
        // Test for 2 nodes, check MonodomainPdeIteration7 correctly solves gating variable and ca_i concentration 
        // dynamics, comparing answers to LuoRudy data (chaste/data/Lr91Good.dat and Lr91NoStimGood.dat)
        // with no stimulus applied to node 1 and init stimulus applied to node 0. 
        // BigTimeStep = 1, SmallTimeStep = 0.01       
        // We really are solving extra ode (the one for voltage as its results are never used)
        int num_nodes=2; 
          
        Node<1> node0(0,true,0);
        Node<1> node1(1,true,0);
        
        double start_time = 0;  
        double big_time_step = 0.5;
        double small_time_step = 0.01; 
        
        AbstractIvpOdeSolver*   solver = new EulerIvpOdeSolver;
        InitialStimulus*     zero_stim = new InitialStimulus(0,0,0);
        
        std::vector<AbstractCardiacCell*> cells(num_nodes);
        for(int i=0; i<num_nodes;i++)
        {
            cells[i] = new LuoRudyIModel1991OdeSystem(solver, zero_stim, small_time_step);
        }        
        
         // Stimulus function to use at node 0. Node 1 is not stimulated.
        double magnitudeOfStimulus = -80.0;  
        double durationOfStimulus  = 0.5 ;  // ms   
        AbstractStimulusFunction* stimulus = new InitialStimulus(magnitudeOfStimulus, durationOfStimulus);
        
        cells[0]->SetStimulusFunction(stimulus);
               
        MonodomainPdeIteration7<1> monodomain_pde( cells, start_time, big_time_step );
//        InitialStimulus* stimulus = new InitialStimulus(magnitudeOfStimulus, durationOfStimulus);

        
        // voltage that gets passed in solving ode
        double voltage = -84.5;
 
   		// initial condition;   
		Vec currentVoltage;
		VecCreate(PETSC_COMM_WORLD, &currentVoltage);
		VecSetSizes(currentVoltage, PETSC_DECIDE, num_nodes);
		//VecSetType(initialCondition, VECSEQ);
		VecSetFromOptions(currentVoltage);
  
		double* currentVoltageArray;
		VecGetArray(currentVoltage, &currentVoltageArray); 
        
        int lo, hi;
        VecGetOwnershipRange(currentVoltage,&lo,&hi);
        
		// initial voltage condition of a constant everywhere on the mesh
		if (lo<=0 && 0<hi)
        {
            currentVoltageArray[0-lo] = voltage;
        }
        if (lo<=1 && 1<hi)
        {
		  currentVoltageArray[1-lo] = voltage;
        }
		
		VecRestoreArray(currentVoltage, &currentVoltageArray);      
		VecAssemblyBegin(currentVoltage);
		VecAssemblyEnd(currentVoltage);
		 
	    monodomain_pde.PrepareForAssembleSystem(currentVoltage);


        double value1 = monodomain_pde.ComputeNonlinearSourceTermAtNode(node0, voltage);
   
        LuoRudyIModel1991OdeSystem ode_system_stimulated(solver, stimulus, small_time_step);
                              
        OdeSolution SolutionNewStimulated = ode_system_stimulated.Compute(
                                                           start_time,
                                                           start_time + big_time_step);
        std::vector<double> solutionSetStimT_05 = SolutionNewStimulated.mSolutions[ SolutionNewStimulated.mSolutions.size()-1 ];
        double value2 = -(-80 + ode_system_stimulated.GetIIonic());

//        TS_ASSERT_DELTA(value1, value2, 0.000001);

        // shouldn't be different when called again as reset not yet been called
        value1 = monodomain_pde.ComputeNonlinearSourceTermAtNode(node0, voltage);
//        TS_ASSERT_DELTA(value1, value2, 0.000001);
  
        LuoRudyIModel1991OdeSystem ode_system_not_stim(solver, zero_stim, small_time_step);

        value1 = monodomain_pde.ComputeNonlinearSourceTermAtNode(node1, voltage);

        OdeSolution SolutionNewNotStim = ode_system_not_stim.Compute(
                                                        start_time,
                                                        start_time + big_time_step);
        std::vector<double> solutionSetNoStimT_05 = SolutionNewNotStim.mSolutions[ SolutionNewNotStim.mSolutions.size()-1 ];
        value2 = -(0 + ode_system_not_stim.GetIIonic());

 //       TS_ASSERT_DELTA(value1, value2, 0.000001);
 

 
 
        // Reset       
       	VecGetArray(currentVoltage, &currentVoltageArray); 
        
        if (lo<=0 && 0<hi)
        {
    		currentVoltageArray[0-lo] = solutionSetStimT_05[4];
        }
        if (lo<=1 && 1<hi)
        {
    		currentVoltageArray[1-lo] = solutionSetNoStimT_05[4];
        }
		
		VecRestoreArray(currentVoltage, &currentVoltageArray);      
		VecAssemblyBegin(currentVoltage);
		VecAssemblyEnd(currentVoltage);

		monodomain_pde.ResetAsUnsolvedOdeSystem();
        monodomain_pde.PrepareForAssembleSystem(currentVoltage);
              
        value1 = monodomain_pde.ComputeNonlinearSourceTermAtNode(node0, solutionSetStimT_05[4]);

        std::vector<double> state_variables = solutionSetStimT_05;
        ode_system_stimulated.SetStateVariables(state_variables);
        OdeSolution SolutionNewStimulatedT_1 = ode_system_stimulated.Compute( start_time + big_time_step, start_time + 2*big_time_step );
        std::vector<double> solutionSetStimT_1 = SolutionNewStimulatedT_1.mSolutions[ SolutionNewStimulatedT_1.mSolutions.size()-1 ];
        value2 = -(0 + ode_system_stimulated.GetIIonic());
                
//        TS_ASSERT_DELTA(value1, value2, 1e-10);
        
        state_variables = solutionSetNoStimT_05;
        ode_system_not_stim.SetStateVariables(state_variables);
        OdeSolution SolutionNewNotStimT_1 = ode_system_not_stim.Compute( start_time + big_time_step, start_time + 2*big_time_step );
        std::vector<double> solutionSetNoStimT_1 = SolutionNewNotStimT_1.mSolutions[ SolutionNewNotStimT_1.mSolutions.size()-1 ];
       
        value1 = monodomain_pde.ComputeNonlinearSourceTermAtNode(node1, solutionSetNoStimT_05[4]);
        value2 = -(0 + ode_system_not_stim.GetIIonic());
        
        
//        TS_ASSERT_DELTA(value1, value2, 1e-10);

     
        VecDestroy(currentVoltage);
        for(int i=0;i<num_nodes;i++)
        {
            delete cells[i];
        }
        delete zero_stim;
        delete stimulus;
        delete solver;
    }
    
     
};

#endif //_TESTMONODOMAINPDE_HPP_
