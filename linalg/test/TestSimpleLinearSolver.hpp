#ifndef _TESTSIMPLELINEARSOLVER_HPP_
#define _TESTSIMPLELINEARSOLVER_HPP_

#include "SimpleLinearSolver.hpp"
#include <cxxtest/TestSuite.h>
#include <petsc.h>
#include <petsc.h>
 
#include "PetscSetupAndFinalize.hpp"

class TestSimpleLinearSolver : public CxxTest::TestSuite 
{
public:
	void TestLinearSolverEasy( void )
	{
		// Solve Ax=b. 2x2 matrix
	
		SimpleLinearSolver solver;
	
		// Set rhs vector
		Vec rhs_vector;
		VecCreate(PETSC_COMM_WORLD, &rhs_vector);
		VecSetSizes(rhs_vector,PETSC_DECIDE,2);
		//VecSetType(rhs_vector, VECSEQ);
	   	VecSetFromOptions(rhs_vector);
	   	
	   	VecSetValue(rhs_vector, 0, (PetscReal) 1, INSERT_VALUES);
	   	VecSetValue(rhs_vector, 1, (PetscReal) 1, INSERT_VALUES);
	   	
	   	//Set Matrix
	   	Mat lhs_matrix;
	   	MatCreate(PETSC_COMM_WORLD, PETSC_DECIDE, PETSC_DECIDE, 2, 2, &lhs_matrix);
	   	//MatSetType(lhs_matrix, MATSEQDENSE);
	   	MatSetType(lhs_matrix, MATMPIDENSE);
	   	
	   	// Set Matrix to Identity matrix
	   	MatSetValue(lhs_matrix, (PetscInt) 0, (PetscInt) 0, (PetscReal) 1, INSERT_VALUES);
		MatSetValue(lhs_matrix, (PetscInt) 0, (PetscInt) 1, (PetscReal) 0, INSERT_VALUES);
		MatSetValue(lhs_matrix, (PetscInt) 1, (PetscInt) 0, (PetscReal) 0, INSERT_VALUES);
		MatSetValue(lhs_matrix, (PetscInt) 1, (PetscInt) 1, (PetscReal) 1, INSERT_VALUES);
		
		// Assemble matrix
	   	MatAssemblyBegin(lhs_matrix, MAT_FINAL_ASSEMBLY);
	   	MatAssemblyEnd(lhs_matrix, MAT_FINAL_ASSEMBLY);
	   	
		// Call solver
		Vec lhs_vector;
		TS_ASSERT_THROWS_NOTHING(lhs_vector = solver.Solve(lhs_matrix, rhs_vector, 2));
		
		// Check result
		PetscScalar *p_lhs_elements_array;
		VecGetArray(lhs_vector, &p_lhs_elements_array);
        int lo, hi;
        VecGetOwnershipRange(lhs_vector, &lo, &hi);
        
        for(int global_index=0; global_index<2; global_index++)
        {
            int local_index = global_index-lo;
            if(lo<=global_index && global_index<hi)
            {    
                TS_ASSERT_DELTA(p_lhs_elements_array[local_index], 1.0, 0.000001);
            }
        }

		// Free memory
		VecDestroy(rhs_vector);
		VecDestroy(lhs_vector);
		MatDestroy(lhs_matrix);
	}
	
	void TestLinearSolverThrowsIfDoesNotConverge( void )
	{
		// Solve Ax=b. 2x2 matrix
		SimpleLinearSolver solver;
	
		// Set rhs vector
		Vec rhs_vector;
		VecCreate(PETSC_COMM_WORLD, &rhs_vector);
		VecSetSizes(rhs_vector,PETSC_DECIDE,2);
		//VecSetType(rhs_vector, VECSEQ);
	   	VecSetFromOptions(rhs_vector);
	   	VecSetValue(rhs_vector, 0, (PetscReal) 1, INSERT_VALUES);
	   	VecSetValue(rhs_vector, 1, (PetscReal) 1, INSERT_VALUES);
	   	
	   	//Set Matrix
	   	Mat lhs_matrix;
	   	MatCreate(PETSC_COMM_WORLD,  PETSC_DECIDE, PETSC_DECIDE, 2, 2, &lhs_matrix);
	   	//MatSetType(lhs_matrix, MATSEQDENSE);
	   	MatSetType(lhs_matrix, MATMPIDENSE);
	   	
	   	// Set Matrix to Zero matrix
	   	MatSetValue(lhs_matrix, (PetscInt) 0, (PetscInt) 0, (PetscReal) 0, INSERT_VALUES);
		MatSetValue(lhs_matrix, (PetscInt) 0, (PetscInt) 1, (PetscReal) 0, INSERT_VALUES);
		MatSetValue(lhs_matrix, (PetscInt) 1, (PetscInt) 0, (PetscReal) 0, INSERT_VALUES);
		MatSetValue(lhs_matrix, (PetscInt) 1, (PetscInt) 1, (PetscReal) 0, INSERT_VALUES);
		
		// Assemble matrix
	   	MatAssemblyBegin(lhs_matrix, MAT_FINAL_ASSEMBLY);
	   	MatAssemblyEnd(lhs_matrix, MAT_FINAL_ASSEMBLY);
	   	
		// Call solver
		Vec lhs_vector;
		
		TS_ASSERT_THROWS_ANYTHING(lhs_vector = solver.Solve(lhs_matrix, rhs_vector, 2));
		
		// Free memory
		VecDestroy(rhs_vector);
		MatDestroy(lhs_matrix);
	}
	
	void TestLinearSolverHarder( void )
	{
		// Solve Ax=b. 2x2 matrix
		SimpleLinearSolver solver;
	
		// Set rhs vector
		Vec rhs_vector;
		VecCreate(PETSC_COMM_WORLD, &rhs_vector);
		VecSetSizes(rhs_vector,PETSC_DECIDE,2);
		//VecSetType(rhs_vector, VECSEQ);
		VecSetFromOptions(rhs_vector);
		
	   	VecSetValue(rhs_vector, 0, (PetscReal) 17, INSERT_VALUES);
	   	VecSetValue(rhs_vector, 1, (PetscReal) 39, INSERT_VALUES);
	   	
	   	//Set Matrix
	   	Mat lhs_matrix;
	   	MatCreate(PETSC_COMM_WORLD, PETSC_DECIDE, PETSC_DECIDE, 2, 2, &lhs_matrix);
	   	//MatSetType(lhs_matrix, MATSEQDENSE);
	   	MatSetType(lhs_matrix, MATMPIDENSE);
	   	
	   	// Set Matrix to Zero matrix
	   	MatSetValue(lhs_matrix, (PetscInt) 0, (PetscInt) 0, (PetscReal) 1, INSERT_VALUES);
		MatSetValue(lhs_matrix, (PetscInt) 0, (PetscInt) 1, (PetscReal) 2, INSERT_VALUES);
		MatSetValue(lhs_matrix, (PetscInt) 1, (PetscInt) 0, (PetscReal) 3, INSERT_VALUES);
		MatSetValue(lhs_matrix, (PetscInt) 1, (PetscInt) 1, (PetscReal) 4, INSERT_VALUES);
		
		// Assemble matrix
	   	MatAssemblyBegin(lhs_matrix, MAT_FINAL_ASSEMBLY);
	   	MatAssemblyEnd(lhs_matrix, MAT_FINAL_ASSEMBLY);
	   	
		// Call solver
		Vec lhs_vector;
		TS_ASSERT_THROWS_NOTHING(lhs_vector = solver.Solve(lhs_matrix, rhs_vector, 2));
		
		// Check result
		PetscScalar *p_lhs_elements_array;
		VecGetArray(lhs_vector, &p_lhs_elements_array);
        int lo, hi;
        VecGetOwnershipRange(lhs_vector, &lo, &hi);


        // p_lhs_elements_array should be equal to [5,6]
        for(int global_index=0; global_index<2; global_index++)
        {
            int local_index = global_index-lo;
            if(lo<=global_index && global_index<hi)
            {    
                TS_ASSERT_DELTA(p_lhs_elements_array[local_index], global_index+5.0, 0.000001);
            }
        }

		// Free memory
		VecDestroy(rhs_vector);
		VecDestroy(lhs_vector);
		MatDestroy(lhs_matrix);
	}
};

#endif //_TESTSIMPLELINEARSOLVER_HPP_
