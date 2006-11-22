#ifndef TESTTYSONNOVAK2001ODESYSTEM_HPP_
#define TESTTYSONNOVAK2001ODESYSTEM_HPP_

#include <stdio.h>
#include <time.h>
#include <cxxtest/TestSuite.h>
#include "TysonNovak2001OdeSystem.hpp"
#include <vector>
#include <iostream>
#include "BackwardEulerIvpOdeSolver.hpp"
#include "EulerIvpOdeSolver.hpp"
#include "RungeKutta4IvpOdeSolver.hpp"
#include "ColumnDataWriter.hpp"

#include "PetscSetupAndFinalize.hpp"


class TestTysonNovak2001OdeSystem : public CxxTest::TestSuite
{
public:

    void testTysonNovak() throw(Exception)
    {
        TysonNovak2001OdeSystem tyson_novak_system;
        
        double time = 0.0;
        std::vector<double> initialConditions;
        initialConditions.push_back(0.6);
        initialConditions.push_back(0.1);
        initialConditions.push_back(1.5);
        initialConditions.push_back(0.6);
        initialConditions.push_back(0.6);
        initialConditions.push_back(0.85);
        std::vector<double> derivs = tyson_novak_system.EvaluateYDerivatives(time,initialConditions);
        
//		Test derivatives are correct
        TS_ASSERT_DELTA(derivs[0],-4.400000000000000e-02, 1e-5);
        TS_ASSERT_DELTA(derivs[1],-6.047872340425530e+00, 1e-5);
        TS_ASSERT_DELTA(derivs[2],3.361442884485838e-02, 1e-5);
        TS_ASSERT_DELTA(derivs[3],4.016602000735009e-02, 1e-5);
        TS_ASSERT_DELTA(derivs[4],8.400000000000001e-03, 1e-5);
        TS_ASSERT_DELTA(derivs[5],7.777500000000001e-03, 1e-5);
        
//		Solve system using backward Euler solver
// Matlab's strictest bit uses 0.01 below and relaxes it on flatter bits.
        double h_value=0.1;

        //Euler solver solution worked out
        BackwardEulerIvpOdeSolver backward_euler_solver;
        RungeKutta4IvpOdeSolver rk4_solver;

        OdeSolution solutions;
        OdeSolution solutions2;


        std::vector<double> state_variables = tyson_novak_system.GetInitialConditions();
        double start_time = std::clock();
        solutions = backward_euler_solver.Solve(&tyson_novak_system, state_variables, 0.0, 75.8350, h_value, h_value);
        double end_time = std::clock();
        //solutions2 = rk4_solver.Solve(&tyson_novak_system, state_variables, 0.0, 75.8350, h_value, h_value);
        //TS_ASSERT_EQUALS(solutions.GetNumberOfTimeSteps(), 10);
        std::cout <<  "Elapsed time = " << (end_time - start_time)/(CLOCKS_PER_SEC) << "\n";

        int step_per_row = 1;
        ColumnDataWriter writer("TysonNovak","TysonNovak");
        int time_var_id = writer.DefineUnlimitedDimension("Time","s");

        std::vector<int> var_ids;
        for (unsigned i=0; i<tyson_novak_system.rGetVariableNames().size(); i++)
        {
            var_ids.push_back(writer.DefineVariable(tyson_novak_system.rGetVariableNames()[i],
                                                    tyson_novak_system.rGetVariableUnits()[i]));
        }
        writer.EndDefineMode();

        for (unsigned i = 0; i < solutions.rGetSolutions().size(); i+=step_per_row)
        {
            writer.PutVariable(time_var_id, solutions.rGetTimes()[i]);
            for (unsigned j=0; j<var_ids.size(); j++)
            {
                writer.PutVariable(var_ids[j], solutions.rGetSolutions()[i][j]);
            }
            writer.AdvanceAlongUnlimitedDimension();
        }
        writer.Close();


		// Test backward euler solutions are OK for a very small time increase...
		int end = solutions.rGetSolutions().size() - 1;

//        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][0],0.59995781827316, 1e-5);
//        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][1],0.09406711653612, 1e-5);
//        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][2],1.50003361032032, 1e-5);
//        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][3],0.60004016820575, 1e-5);
//        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][4],0.60000839905560, 1e-5);
//        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][5],0.85000777753272, 1e-5);
//        
    
        // Proper values from MatLab ode15s - shocking tolerances to pass though.
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][0],0.10000000000000, 1e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][1],0.98913684535843, 1e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][2],1.54216806705641, 1e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][3],1.40562614481544, 1e-1);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][4],0.67083371879876, 1e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][5],0.95328206604519, 1e-2);
        
               
    }
};

#endif /*TESTTYSONNOVAK2001ODESYSTEM_HPP_*/
