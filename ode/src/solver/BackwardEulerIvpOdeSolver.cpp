/**
 * Concrete BackwardEulerIvpOdeSolver class.
 */
#include "BackwardEulerIvpOdeSolver.hpp"
#include "AbstractIvpOdeSolver.hpp"
#include "AbstractOdeSystem.hpp"
#include "OdeSolution.hpp"
#include <vector>

#include "SimpleNonlinearSolver.hpp"
#include <cxxtest/TestSuite.h>
#include <petsc.h>
#include <cmath>
#include "ReplicatableVector.hpp"
#include "PetscException.hpp"
#include <cassert>
#include <iostream>

#include <iostream>

typedef struct
{
    double TimeStep;
    double Time;
    AbstractOdeSystem *pAbstractOdeSystem;
    std::vector<double> currentYValue;
}
BackwardEulerStructure;

//This one's a hack, so that we don't have to keep recalculating ownership
static unsigned mLo;
static unsigned mHi;


PetscErrorCode ComputeResidual(SNES snes,Vec solutionGuess,Vec residual,void *pContext);
PetscErrorCode ComputeJacobian(SNES snes,Vec input,Mat *pJacobian ,Mat *pPreconditioner,MatStructure *pMatStructure ,void *pContext);

/**
 * Solves a system of n ODEs using the Backward Euler method
 *
 * To be used in the form:
 *
 * BackwardEulerIvpOdeSolver mySolver;
 * OdeSolution solution=mySolver->Solve(pMyOdeSystem, StartTime, EndTime, TimeStep, yInit);
 *
*/

std::vector<double> BackwardEulerIvpOdeSolver::CalculateNextYValue(AbstractOdeSystem* pAbstractOdeSystem,
        double timeStep,
        double time,
        std::vector<double> currentYValue)
{
    /*
     * for each timestep in AbstractOneStepIvpSolver calculates a vector containing 
     * the next Y value from the current one for each equation in the system.
     */
    
    int num_equations = pAbstractOdeSystem->GetNumberOfStateVariables();
    
    std::vector<double> dy(num_equations);
    dy = pAbstractOdeSystem->EvaluateYDerivatives(time, currentYValue);
    
    
    //\todo only reserve memory if returning this
    std::vector<double> next_y_value(num_equations);
    
    Vec initial_guess;
    VecCreate(PETSC_COMM_WORLD, &initial_guess);
    VecSetSizes(initial_guess, PETSC_DECIDE,num_equations);
    //VecSetType(initial_guess, VECSEQ);
    VecSetFromOptions(initial_guess);
    
    //If this class has not been used before then find out and set the ownership
    if (mHi == 0)
    {
        int lo, hi;
        VecGetOwnershipRange(initial_guess, &lo, &hi);
        mLo=lo;
        mHi=hi;
    }
    
    for (unsigned global_index=mLo; global_index<mHi; global_index++)
    {
        VecSetValue(initial_guess, global_index, currentYValue[global_index], INSERT_VALUES);
    }
    VecAssemblyBegin(initial_guess);
    VecAssemblyEnd(initial_guess);
    
    Vec answer;
    Vec residual;
    VecDuplicate(initial_guess, &residual);
    VecDuplicate(initial_guess, &answer);
    
    BackwardEulerStructure *p_backward_euler_structure = new BackwardEulerStructure;
    p_backward_euler_structure->pAbstractOdeSystem = pAbstractOdeSystem;
    p_backward_euler_structure->TimeStep = timeStep;
    p_backward_euler_structure->Time = time;
    p_backward_euler_structure->currentYValue = std::vector<double>(currentYValue);
    
    SimpleNonlinearSolver solver;
    answer = solver.Solve(&ComputeResidual, &ComputeJacobian,
                          residual, initial_guess, p_backward_euler_structure);
                          
    ReplicatableVector answer_replicated;
    answer_replicated.ReplicatePetscVector(answer);
    for (int i=0; i<num_equations; i++)
    {
        next_y_value[i] = answer_replicated[i];
        
    }
    
    delete p_backward_euler_structure;
    return next_y_value;
}



PetscErrorCode ComputeResidual(SNES snes,Vec solutionGuess,Vec residual,void *pContext)
{
    BackwardEulerStructure *p_backward_euler_structure = (BackwardEulerStructure*)pContext;
    AbstractOdeSystem *p_ode_system = p_backward_euler_structure->pAbstractOdeSystem;
    double time_step = p_backward_euler_structure->TimeStep;
    double time = p_backward_euler_structure->Time;
    std::vector<double> current_y_value = p_backward_euler_structure->currentYValue;
    
    unsigned num_equations = p_ode_system->GetNumberOfStateVariables();
    PetscScalar *p_solution_guess_array;
    VecGetArray(solutionGuess, &p_solution_guess_array);
    std::vector<double> current_guess;
    for (unsigned i=0; i<num_equations; i++)
    {
        current_guess.push_back(p_solution_guess_array[i]);
    }
    VecRestoreArray(solutionGuess, &p_solution_guess_array);
    
    ReplicatableVector solution_guess_replicated;
    solution_guess_replicated.ReplicatePetscVector(solutionGuess);
    
    std::vector<double> dy(num_equations);
    dy = p_ode_system->EvaluateYDerivatives(time, current_guess);
    
    VecGetArray(solutionGuess, &p_solution_guess_array);
    for (unsigned global_index=mLo; global_index<mHi; global_index++)
    {
        double res_i;
        unsigned local_index=global_index - mLo;
        res_i = (p_solution_guess_array[local_index]-current_y_value[global_index])/time_step - dy[global_index];
        VecSetValue(residual,global_index,res_i,INSERT_VALUES);
    }
    VecRestoreArray(solutionGuess, &p_solution_guess_array);
    
    VecAssemblyBegin(residual);
    VecAssemblyEnd(residual);
    
    return 0;
}

PetscErrorCode ComputeJacobian(SNES snes,Vec solutionGuess, Mat *pJacobian ,Mat *pPreconditioner,MatStructure *pMatStructure ,void *pContext)
{
    BackwardEulerStructure *p_backward_euler_structure = (BackwardEulerStructure*)pContext;
    AbstractOdeSystem *p_ode_system = p_backward_euler_structure->pAbstractOdeSystem;
    std::vector<double> current_y_value = p_backward_euler_structure->currentYValue;
    unsigned num_equations = p_ode_system->GetNumberOfStateVariables();
    
    Vec residual, residual_perturbed, solution_perturbed, jacobian_column;
    // duplicate the sizes of vectors
    VecDuplicate(solutionGuess, &residual);
    VecDuplicate(solutionGuess, &residual_perturbed);
    VecDuplicate(solutionGuess, &solution_perturbed);
    VecDuplicate(solutionGuess, &jacobian_column);
    
    
    PETSCEXCEPT(ComputeResidual(snes, solutionGuess, residual, pContext));
    
    // this epsilon is somewhat arbitrary
    double eps = 1.0e-6;
    for (unsigned global_column=0; global_column<num_equations; global_column++)
    {
    
        VecCopy(solutionGuess, solution_perturbed);
        if (mLo<=global_column && global_column<mHi)
        {
            VecSetValue(solution_perturbed, global_column, eps, ADD_VALUES);
        }
        VecAssemblyBegin(solution_perturbed);
        VecAssemblyEnd(solution_perturbed);
        
        PETSCEXCEPT(ComputeResidual(snes, solution_perturbed, residual_perturbed, pContext));
        
        // compute residual_perturbed - residual
	    double one_over_eps=1.0/eps;
	    double subtract=-1;
        #if (PETSC_VERSION_MINOR == 2) //Old API
            PETSCEXCEPT( VecWAXPY(&subtract, residual, residual_perturbed, jacobian_column));
            PETSCEXCEPT( VecScale(&one_over_eps, jacobian_column));
        #else
            PETSCEXCEPT( VecWAXPY(jacobian_column, subtract, residual, residual_perturbed));
            PETSCEXCEPT( VecScale(jacobian_column,  one_over_eps));
        #endif
       
        
        PetscScalar *p_jacobian_column_array;
        VecGetArray(jacobian_column, &p_jacobian_column_array);
        for (unsigned global_row=mLo; global_row<mHi; global_row++)
        {
            unsigned local_row=global_row-mLo;
            MatSetValue(*pJacobian, global_row, global_column, p_jacobian_column_array[local_row], INSERT_VALUES);
        }
        // not sure if this line is needed
        VecRestoreArray(jacobian_column, &p_jacobian_column_array);
    }
    
    MatAssemblyBegin(*pJacobian,MAT_FINAL_ASSEMBLY);
    MatAssemblyEnd(*pJacobian,MAT_FINAL_ASSEMBLY);
    
    VecDestroy(residual);
    VecDestroy(residual_perturbed);
    VecDestroy(solution_perturbed);
    VecDestroy(jacobian_column);
    return 0;
    
}



