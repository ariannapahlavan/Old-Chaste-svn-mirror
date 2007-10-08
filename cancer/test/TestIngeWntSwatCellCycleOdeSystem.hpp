#ifndef TESTINGEWNTSWATCELLCYCLEODESYSTEM_HPP_
#define TESTINGEWNTSWATCELLCYCLEODESYSTEM_HPP_

#include <stdio.h>
#include <time.h>
#include <cxxtest/TestSuite.h>
#include "IngeWntSwatCellCycleOdeSystem.hpp"
#include <vector>
#include <iostream>
#include "RungeKutta4IvpOdeSolver.hpp"
#include "RungeKuttaFehlbergIvpOdeSolver.hpp"
#include "BackwardEulerIvpOdeSolver.hpp"
#include "ColumnDataWriter.hpp"
#include "CellMutationStates.hpp"


class TestIngeWntSwatCellCycleOdeSystem : public CxxTest::TestSuite
{
public:

    void TestIngeWntSwatCellCycleEquationsHighWnt()
    {
        double wnt_level = 1.0;
        IngeWntSwatCellCycleOdeSystem wnt_cell_cycle_system(wnt_level);
        
        double time = 0.0;
        std::vector<double> initial_conditions = wnt_cell_cycle_system.GetInitialConditions();
        TS_ASSERT_EQUALS(initial_conditions.size(), 22u);
        std::vector<double> derivs(initial_conditions.size());
        // test ICs are being set up properly (quite intricate equations themselves!)
        TS_ASSERT_DELTA(initial_conditions[0], 7.357000000000000e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[1], 1.713000000000000e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[2], 6.900000000000001e-02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[3], 3.333333333333334e-03, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[4], 1.000000000000000e-04, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[5], 1.428571428571428e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[6], 2.857142857142857e-02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[7], 2.120643654085212e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[8], 1.439678172957394e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[9], 0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[10], 0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[11], 0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[12], 1.000000000000000e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[13], 1.028341552112424e+02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[14], 0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[15], 2.500000000000000e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[16], 1.439678172957394e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[17], 0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[18], 0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[19], 0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[20], 2.235636835087720e+00, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[21], 1.000000000000000e+00, 1e-7);
        
        wnt_cell_cycle_system.EvaluateYDerivatives(time, initial_conditions, derivs);
        // Test derivatives are correct at t=0 for these initial conditions
        // Swat's
        TS_ASSERT_DELTA(derivs[0],-1.586627673253325e-02, 1e-5);
        TS_ASSERT_DELTA(derivs[1],-5.532201118824132e-05, 1e-5);
        TS_ASSERT_DELTA(derivs[2],5.676110199115955e+00, 1e-5); // changes due to new beta-catenin levels...
        TS_ASSERT_DELTA(derivs[3],-7.449833887043188e-03, 1e-5);
        TS_ASSERT_DELTA(derivs[4],1.549680000000000e-02, 1e-5);
        
        // Inge's
        for (unsigned i=5; i<initial_conditions.size() ; i++)
        {
            TS_ASSERT_DELTA(derivs[i],0.0, 1e-9);
        }
    }
        
    void TestIngeWntSwatCellCycleEquationsLowWnt()
    {
        /**
         * And the same for a healthy cell at a low Wnt level
         */
        double time = 0.0;
        double wnt_level = 0.0;
        IngeWntSwatCellCycleOdeSystem wnt_cell_cycle_system2(wnt_level,LABELLED);
        std::vector<double> initial_conditions = wnt_cell_cycle_system2.GetInitialConditions();
        std::vector<double> derivs(initial_conditions.size());
        // test ICs are being set up properly (quite intricate equations themselves!)
        TS_ASSERT_DELTA(initial_conditions[0], 7.357000000000000e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[1], 1.713000000000000e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[2], 6.900000000000001e-02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[3], 3.333333333333334e-03, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[4], 1.000000000000000e-04, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[5], 6.666666666666666e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[6], 6.666666666666667e-02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[7], 4.491941767913325e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[8], 2.540291160433374e+00, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[9], -4.440892098500626e-16, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[10], 0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[11], 0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[12], 1.000000000000000e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[13], 1.814493686023838e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[14], 0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[15], 2.500000000000000e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[16], 2.540291160433374e+00, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[17], 0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[18], 0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[19], 0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[20], 4.834939252004746e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[21], 0.000000000000000e+00, 1e-7);
            
        wnt_cell_cycle_system2.EvaluateYDerivatives(time, initial_conditions, derivs);
        
        // Test derivatives are correct at t=0 for these initial conditions
        // (figures from MatLab code)
        TS_ASSERT_DELTA(derivs[0],-1.586627673253325e-02, 1e-5);
        TS_ASSERT_DELTA(derivs[1],-5.532201118824132e-05, 1e-5);
        TS_ASSERT_DELTA(derivs[2],9.335139714597300e-01, 1e-5);
        TS_ASSERT_DELTA(derivs[3],-7.449833887043188e-03, 1e-5);
        TS_ASSERT_DELTA(derivs[4],1.549680000000000e-02, 1e-5);
        
        for (unsigned i=5; i<initial_conditions.size() ; i++)
        {
            TS_ASSERT_DELTA(derivs[i],0.0, 1e-9);
        }
    }
    
    void TestIngeWntSwatCellCycleEquationsAPCOneHit()  
    {  
        /**
         * A test for the case mutation = 1
         * (An APC +/- mutation)
         */
        double time = 0.0;
        CellMutationState mutation = APC_ONE_HIT;
        double wnt_level = 0.5;
        IngeWntSwatCellCycleOdeSystem wnt_cell_cycle_system3(wnt_level,mutation);
        std::vector<double> initial_conditions = wnt_cell_cycle_system3.GetInitialConditions();
        //std::cout << "mutation " << mutation << " beta-cat = " << initial_conditions[6] << "\n";
        std::vector<double> derivs(initial_conditions.size());

        TS_ASSERT_DELTA(initial_conditions[0], 7.357000000000000e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[1], 1.713000000000000e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[2], 6.900000000000001e-02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[3], 3.333333333333334e-03, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[4], 1.000000000000000e-04, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[5], 1.481481481481481e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[6], 4.444444444444445e-02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[7], 2.186081417430197e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[8], 1.406959291284901e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[9],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[10],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[11],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[12], 1.000000000000000e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[13], 1.004970922346358e+02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[14],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[15], 2.500000000000000e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[16], 1.406959291284901e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[17],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[18],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[19],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[20], 2.195986001032853e+00, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[21], 5.000000000000000e-01, 1e-7);
        
        wnt_cell_cycle_system3.EvaluateYDerivatives(time, initial_conditions, derivs);

        // Test derivatives are correct at t=0 for these initial conditions
        // (figures from MatLab code)
        TS_ASSERT_DELTA(derivs[0],-1.586627673253325e-02, 1e-5);
        TS_ASSERT_DELTA(derivs[1],-5.532201118824132e-05, 1e-5);
        TS_ASSERT_DELTA(derivs[2], 5.545234672425986e+00, 1e-4);
        TS_ASSERT_DELTA(derivs[3],-7.449833887043188e-03, 1e-5);
        TS_ASSERT_DELTA(derivs[4], 1.549680000000000e-02, 1e-5);
        for (unsigned i=5; i<initial_conditions.size() ; i++)
        {
            TS_ASSERT_DELTA(derivs[i],0.0, 1e-9);
        }
        
    }
    
    void TestIngeWntSwatCellCycleEquationsBetaCatOneHit()  
    {  /**
         * A test for the case mutation = 2
         * (A beta-cat delta45 mutation)
         */
        double time = 0.0;
        CellMutationState mutation = BETA_CATENIN_ONE_HIT;
        double wnt_level = 1.0;
        IngeWntSwatCellCycleOdeSystem wnt_cell_cycle_system4(wnt_level,mutation);
        std::vector<double> initial_conditions = wnt_cell_cycle_system4.GetInitialConditions();
        //std::cout << "mutation " << mutation << " beta-cat = " << initial_conditions[6] << "\n";
        std::vector<double> derivs(initial_conditions.size());
        
        TS_ASSERT_DELTA(initial_conditions[0], 7.357000000000000e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[1], 1.713000000000000e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[2], 6.900000000000001e-02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[3], 3.333333333333334e-03, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[4], 1.000000000000000e-04, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[5], 1.428571428571428e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[6], 2.857142857142857e-02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[7], 2.120643654085212e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[8], 1.439678172957394e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[9],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[10],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[11],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[12], 1.000000000000000e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[13], 1.028341552112424e+02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[14],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[15], 2.500000000000000e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[16], 1.439678172957394e+01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[17],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[18],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[19],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[20], 2.235636835087720e+00, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[21], 1, 1e-7);
        
        wnt_cell_cycle_system4.EvaluateYDerivatives(time, initial_conditions, derivs);
        
        // Test derivatives are correct at t=0 for these initial conditions
        // (figures from MatLab code)
        TS_ASSERT_DELTA(derivs[0],-1.586627673253325e-02, 1e-5);
        TS_ASSERT_DELTA(derivs[1],-5.532201118824132e-05, 1e-5);
        TS_ASSERT_DELTA(derivs[2],5.676110199115955e+00, 1e-4);
        TS_ASSERT_DELTA(derivs[3],-7.449833887043188e-03, 1e-5);
        TS_ASSERT_DELTA(derivs[4],1.549680000000000e-02, 1e-5);
        TS_ASSERT_DELTA(derivs[5], 0, 1e-5);
        TS_ASSERT_DELTA(derivs[6], 0, 1e-5);
        TS_ASSERT_DELTA(derivs[7], 0, 1e-5);
        TS_ASSERT_DELTA(derivs[8], -12.5, 1e-5);
        TS_ASSERT_DELTA(derivs[9], 0, 1e-5);
        TS_ASSERT_DELTA(derivs[10], 12.5, 1e-5);
        for (unsigned i=11; i<initial_conditions.size() ; i++)
        {
            TS_ASSERT_DELTA(derivs[i],0.0, 1e-9);
        }
    }
    
    void TestIngeWntSwatCellCycleEquationsAPCTwoHit()  
    {   /**
         * A test for the case mutation = 3
         * (An APC -/- mutation)
         */
        double time = 0.0;
        CellMutationState mutation = APC_TWO_HIT;
        double wnt_level = 1.0;
        IngeWntSwatCellCycleOdeSystem wnt_cell_cycle_system5(wnt_level,mutation);
        std::vector<double> initial_conditions = wnt_cell_cycle_system5.GetInitialConditions();
        //std::cout << "mutation " << mutation << " beta-cat = " << initial_conditions[6] << "\n";

        TS_ASSERT_DELTA(initial_conditions[0], 7.357000000000000e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[1], 1.713000000000000e-01, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[2], 6.900000000000001e-02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[3], 3.333333333333334e-03, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[4], 1.000000000000000e-04, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[5],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[6], 3.333333333333333e-02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[7],                      0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[8],                    25, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[9],                      0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[10],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[11],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[12],                     10, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[13], 1.785714285714286e+02, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[14],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[15],                   25, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[16],                   25, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[17],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[18],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[19],                     0, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[20], 3.333333333333333e+00, 1e-7);
        TS_ASSERT_DELTA(initial_conditions[21], 1, 1e-7);
        
        std::vector<double> derivs(initial_conditions.size());
        wnt_cell_cycle_system5.EvaluateYDerivatives(time, initial_conditions, derivs);
        
        // Test derivatives are correct at t=0 for these initial conditions
        // (figures from MatLab code)
        TS_ASSERT_DELTA(derivs[0],-1.586627673253325e-02, 1e-5);
        TS_ASSERT_DELTA(derivs[1],-5.532201118824132e-05, 1e-5);
        TS_ASSERT_DELTA(derivs[2],9.917397507286381e+00, 1e-4);
        TS_ASSERT_DELTA(derivs[3],-7.449833887043188e-03, 1e-5);
        TS_ASSERT_DELTA(derivs[4],1.549680000000000e-02, 1e-5);
                
        for (unsigned i=5; i<initial_conditions.size() ; i++)
        {
            TS_ASSERT_DELTA(derivs[i],0.0, 1e-9);
        }
    }
    
    void TestIngeWntSwatCellCycleSolver() throw(Exception)
    {
        double wnt_level = 1.0;
        IngeWntSwatCellCycleOdeSystem wnt_system(wnt_level,LABELLED);
        // Solve system using rk4 solver
        // Matlab's strictest bit uses 0.01 below and relaxes it on flatter bits.
        
        double h_value=0.001;
        
        RungeKutta4IvpOdeSolver rk4_solver;
        RungeKuttaFehlbergIvpOdeSolver rkf_solver;
        BackwardEulerIvpOdeSolver back_solver(9);
        
        OdeSolution solutions;
        //OdeSolution solutions2;
        
        std::vector<double> initial_conditions = wnt_system.GetInitialConditions();
                
        double start_time, end_time, elapsed_time = 0.0;
        start_time = std::clock();
        solutions = rk4_solver.Solve(&wnt_system, initial_conditions, 0.0, 100.0, h_value, h_value);
        end_time = std::clock();
        elapsed_time = (end_time - start_time)/(CLOCKS_PER_SEC);
        std::cout <<  "1. Runge-Kutta Elapsed time = " << elapsed_time << "\n";
        
        // Test solutions are OK for a small time increase...
        int end = solutions.rGetSolutions().size() - 1;
        // Tests the simulation is ending at the right time...(going into S phase at 5.971 hours)
        TS_ASSERT_DELTA(solutions.rGetTimes()[end] , 6.193774457302713e+00 , 1e-2);
        // Proper values from MatLab ode15s - shocking tolerances to pass though.
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][0], 2.937457584307182e-01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][1], 1.000117721173146e+00, 1.01e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][2], 2.400566063511684e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][3], 1.392886957364369e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][4], 1.358673176849681e-01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][5], 1.428571428571429e-01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][6], 2.857142857142857e-02, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][7], 2.120643654085166e-01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][8], 1.439678172957273e+01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][9], 0.0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][10], 0.0, 1.01e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][11], 0.0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][12], 1.000000000000039e+01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][13], 1.028341552112378e+02, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][14], 0.0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][15], 2.500000000000000e+01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][16], 1.439678172957305e+01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][17], 0.0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][18], 0.0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][19], 0.0, 1.01e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][20], 2.235636835087647e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][21], 1.0, 1e-3);
    }
    
    void TestIngeWntSwatCellCycleSolverWithAPCSingleHit() throw(Exception)
    {
        double wnt_level = 1.0;
        IngeWntSwatCellCycleOdeSystem wnt_system(wnt_level,APC_ONE_HIT);
        // Solve system using rk4 solver
        // Matlab's strictest bit uses 0.01 below and relaxes it on flatter bits.
        
        double h_value=0.0001;
        
        RungeKutta4IvpOdeSolver rk4_solver;        
        OdeSolution solutions;
        
        std::vector<double> initial_conditions = wnt_system.GetInitialConditions();
                
        solutions = rk4_solver.Solve(&wnt_system, initial_conditions, 0.0, 100.0, h_value, h_value);
        
        // Test solutions are OK for a small time increase...
        int end = solutions.rGetSolutions().size() - 1;
        // Tests the simulation is ending at the right time...(going into S phase at 3.94 hours)
        TS_ASSERT_DELTA(solutions.rGetTimes()[end] , 4.722377242770206e+00 , 1e-2);
        // Proper values from MatLab ode15s - shocking tolerances to pass though.
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][0], 2.449671985497571e-01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][1], 1.00, 1.01e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][2], 2.970519972449688e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][3], 1.962479928865283e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][4], 1.594820065531480e-01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][5], 7.692307692307693e-02, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][6], 3.076923076923077e-02, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][7], 1.216821534917367e-01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][8], 1.891589232541249e+01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][9], 0.0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][10], 0.0, 1.01e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][11], 0.0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][12], 1.000000000000039e+01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][13], 1.351135166100946e+02, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][14], 0.0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][15], 2.500000000000000e+01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][16], 1.891589232541281e+01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][17], 0.0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][18], 0.0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][19], 0.0, 1.01e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][20], 2.744779424184835e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][21], 1.0, 1e-3);
    }
    
    void TestIngeWntSwatCellCycleSolverWithBetaCateninHit() throw(Exception)
    {
        double wnt_level = 1.0;
        IngeWntSwatCellCycleOdeSystem wnt_system(wnt_level,BETA_CATENIN_ONE_HIT);
        // Solve system using rk4 solver
        // Matlab's strictest bit uses 0.01 below and relaxes it on flatter bits.
        
        double h_value=0.0001;
        
        RungeKutta4IvpOdeSolver rk4_solver;        
        OdeSolution solutions;
        
        std::vector<double> initial_conditions = wnt_system.GetInitialConditions();
        
        solutions = rk4_solver.Solve(&wnt_system, initial_conditions, 0.0, 100.0, h_value, h_value);
        
        // Test solutions are OK for a small time increase...
        int end = solutions.rGetSolutions().size() - 1;
        // Tests the simulation is ending at the right time...(going into S phase at 7.81 hours)
        TS_ASSERT_DELTA(solutions.rGetTimes()[end] , 6.109381124487460, 1e-2);
        // Proper values from MatLab ode15s - shocking tolerances to pass though.
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][0], 2.885925504994788e-01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][1], 1.0, 1.01e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][2], 2.461454077577112e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][3], 1.443615084885885e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][4], 1.383019697069664e-01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][5], 1.428571428571428e-01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][6], 2.857142857142857e-02, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][7], 1.826340175985522e-01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][8], 8.847117964569135e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][9], 0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][10], 6.199836467090596e+00, 1.01e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][11], 0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][12], 9.735946067044630e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][13], 6.153733439469387e+01, 1e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][14], 4.310129026467735e+01, 1e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][15], 2.476909200705334e+01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][16], 8.766188466378457e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][17], 0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][18], 6.141626362262133e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][19], 0, 1.01e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][20], 2.283368993736725e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][21], 1, 1e-3);
    }
    
    void TestIngeWntSwatCellCycleSolverWithAPCDoubleHit() throw(Exception)
    {
        double wnt_level = 1.0;
        IngeWntSwatCellCycleOdeSystem wnt_system(wnt_level,APC_TWO_HIT);
        // Solve system using rk4 solver
        // Matlab's strictest bit uses 0.01 below and relaxes it on flatter bits.
        
        double h_value=0.0001;
        
        RungeKutta4IvpOdeSolver rk4_solver;        
        OdeSolution solutions;
        
        std::vector<double> initial_conditions = wnt_system.GetInitialConditions();
        
        solutions = rk4_solver.Solve(&wnt_system, initial_conditions, 0.0, 100.0, h_value, h_value);
        
        // Test solutions are OK for a small time increase...
        int end = solutions.rGetSolutions().size() - 1;
        // Tests the simulation is ending at the right time...(going into S phase at 3.94 hours)
        TS_ASSERT_DELTA(solutions.rGetTimes()[end] , 3.912928619944499e+00 , 1e-2);
        // Proper values from MatLab ode15s - shocking tolerances to pass though.
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][0], 2.040531616988712e-01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][1], 1.000000000000046e+00 , 1.01e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][2], 3.738096511672503e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][3], 2.726370715594771e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][4], 1.844725907694925e-01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][5], 0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][6],  3.333333333333333e-02, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][7], 0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][8], 2.499999999999840e+01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][9], 0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][10], 0, 1.01e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][11], 0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][12], 1.000000000000064e+01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][13], 1.785714285714286e+02, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][14], 0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][15], 2.500000000000076e+01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][16], 2.499999999999917e+01, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][17], 0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][18], 0, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][19], 0, 1.01e-2);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][20], 3.333333333333310e+00, 1e-3);
        TS_ASSERT_DELTA(solutions.rGetSolutions()[end][21], 1, 1e-3);
     
    }
    
};

#endif /*TESTINGEWNTSWATCELLCYCLEODESYSTEM_HPP_*/
