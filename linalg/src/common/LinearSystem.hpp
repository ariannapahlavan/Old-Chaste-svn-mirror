/*

Copyright (C) University of Oxford, 2005-2009

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Chaste is free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Chaste is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details. The offer of Chaste under the terms of the
License is subject to the License being interpreted in accordance with
English Law and subject to any action against the University of Oxford
being under the jurisdiction of the English Courts.

You should have received a copy of the GNU Lesser General Public License
along with Chaste. If not, see <http://www.gnu.org/licenses/>.

*/


#ifndef _LINEARSYSTEM_HPP_
#define _LINEARSYSTEM_HPP_

#include "UblasCustomFunctions.hpp"
#include <petscvec.h>
#include <petscmat.h>
#include <petscksp.h>

#include <string>

/**
 * Linear System class. Stores and solves a linear equation of the form Ax=b,
 * where A is a square matrix and x and b are column vectors.
 * The class uses PETSc.
 */
class LinearSystem
{
	friend class TestLinearSystem;

private:

    Mat mLhsMatrix;  /**< The left-hand side matrix. */
    Vec mRhsVector;  /**< The right-hand side vector. */
    PetscInt mSize;  /**< The size of the linear system. */

    /** \todo
     * Verify claim that ownership range for Vec and Mat is same.
     * This should only matter for efficiency if the claim is false.
     */
    PetscInt mOwnershipRangeLo; /**< For parallel code.  Stores lowest index of vectors and lowest row of matrix*/
    PetscInt mOwnershipRangeHi; /**< Stores <b>one more than</b> the highest index stored locally*/

    MatNullSpace mMatNullSpace;

    /** Whether we need to destroy the Petsc matrix and vector in our destructor */
    bool mDestroyMatAndVec;

    KSP mKspSolver;
    bool mKspIsSetup;  /**< Used by Solve method to track whether KSP has been used. */
    double mNonZerosUsed;  /**< Yes, it really is stored as a double. */
    bool mMatrixIsConstant;
    double mTolerance;
    bool mUseAbsoluteTolerance;
    char mKspType[30];
    char mPcType[30];

    Vec mDirichletBoundaryConditionsVector; /**< Storage for efficient application of Dirichlet BCs, see boundary conditions container*/
    
public:

    /**
     * Constructor.
     * 
     * @param lhsVectorSize
     * @param matType defaults to MATMPIAIJ
     */
    LinearSystem(PetscInt lhsVectorSize, MatType matType=(MatType) MATMPIAIJ);

    /**
     * Alternative constructor.
     * 
     * Create a linear system, where the size is based on the size of a given
     * PETSc vec.
     * 
     * The LHS & RHS vectors will be created by duplicating this vector's
     * settings.  This should avoid problems with using VecScatter on
     * bidomain simulation results.
     * 
     * @param templateVector
     */
    LinearSystem(Vec templateVector);

    /**
     * Alternative constructor.
     * 
     * Create a linear system which wraps the provided PETSc objects so we can
     * access them using our API.  Either of the objects may be NULL, but at
     * least one of them must not be.
     *
     * Useful for storing residuals and jacobians when solving nonlinear PDEs.
     * 
     * @param residualVector
     * @param jacobianMatrix
     */
    LinearSystem(Vec residualVector, Mat jacobianMatrix);

    /**
     * Destructor.
     */
    ~LinearSystem();

//    bool IsMatrixEqualTo(Mat testMatrix);
//    bool IsRhsVectorEqualTo(Vec testVector);
    void SetMatrixElement(PetscInt row, PetscInt col, double value);
    void AddToMatrixElement(PetscInt row, PetscInt col, double value);

    void AssembleFinalLinearSystem();         // Call before solve
    void AssembleIntermediateLinearSystem();  // Should be called before AddToMatrixElement
    void AssembleFinalLhsMatrix();
    void AssembleIntermediateLhsMatrix();
    void AssembleRhsVector();

    /**
     * Force PETSc to treat the matrix in this linear system as symmetric from now on.
     */
    void SetMatrixIsSymmetric();

    /**
     * Set mMatrixIsConstant.
     * 
     * @param matrixIsConstant
     */
    void SetMatrixIsConstant(bool matrixIsConstant);

    /**
     * Set the relative tolerance.
     * 
     * @param relativeTolerance
     */
    void SetRelativeTolerance(double relativeTolerance);

    /**
     * Set the absolute tolerance.
     * 
     * @param absoluteTolerance
     */
    void SetAbsoluteTolerance(double absoluteTolerance);

    void SetKspType(const char*);
    void SetPcType(const char*);

    /**
     * Display the left-hand side matrix.
     */
    void DisplayMatrix();

    /**
     * Display the right-hand side vector.
     */
    void DisplayRhs();

    /**
     * Set all entries in a given row of a matrix to a certain value.
     * 
     * @param row
     * @param value
     */
    void SetMatrixRow(PetscInt row, double value);

    /**
     * Zero a row of the left-hand side matrix.
     * 
     * @param row
     */
    void ZeroMatrixRow(PetscInt row);

    /**
     * Zero a column of the left-hand side matrix.
     * 
     * Unfortunately there is no equivalent method in Petsc, so this has to be 
     * done carefully to ensure that the sparsity structure of the matrix
     * is not broken. Only owned entries which are non-zero are zeroed.
     * 
     * @param col
     */
    void ZeroMatrixColumn(PetscInt col);

    /**
     * Zero all entries of the left-hand side matrix.
     */
    void ZeroLhsMatrix();

    /**
     * Zero all entries of the right-hand side vector.
     */
    void ZeroRhsVector();

    /**
     * Zero all entries of the left-hand side matrix and right-hand side vector.
     */
    void ZeroLinearSystem();

    /**
     * Solve the linear system.
     * 
     * @param lhsGuess  an optional initial guess for the solution (defaults to NULL)
     */
    Vec Solve(Vec lhsGuess=NULL);

    /**
     * Set an element of the right-hand side vector to a given value.
     * 
     * @param row
     * @param value
     */
    void SetRhsVectorElement(PetscInt row, double value);

    /**
     * Add a value to an element of the right-hand side vector.
     * 
     * @param row
     * @param value
     */
    void AddToRhsVectorElement(PetscInt row, double value);

    /**
     * Get method for mSize.
     */
    unsigned GetSize();

    /**
     * 
     * @param nullbasis
     * @param numberOfBases
     */
    void SetNullBasis(Vec nullbasis[], unsigned numberOfBases);

    /**
     * Get access to the rhs vector directly. Shouldn't generally need to be called.
     */
    Vec& rGetRhsVector();

    /**
     * Get access to the lhs matrix directly. Shouldn't generally need to be called.
     */
    Mat& rGetLhsMatrix();

    /**
     * Gets access to the dirichlet boundary conditions vector. 
     * 
     * Should only be used by the BoundaryConditionsContainer.
     */
    Vec& rGetDirichletBoundaryConditionsVector();

    // DEBUGGING CODE:
    /**
     * Get this process's ownership range of the contents of the system.
     * 
     * @param lo
     * @param hi
     */
    void GetOwnershipRange(PetscInt& lo, PetscInt& hi);

    /**
     * Return an element of the matrix.
     * May only be called for elements you own.
     * 
     * @param row
     * @param col
     */
    double GetMatrixElement(PetscInt row, PetscInt col);

    /**
     * Return an element of the RHS vector.
     * May only be called for elements you own.
     * 
     * @param row
     */
    double GetRhsVectorElement(PetscInt row);

    /**
     * Add multiple values to the matrix of linear system.
     * 
     * @param matrixRowAndColIndices mapping from index of the ublas matrix (see param below)
     *  to index of the Petsc matrix of this linear system
     * @param smallMatrix Ublas matrix containing the values to be added
     *
     * N.B. Values which are not local (ie the row is not owned) will be skipped.
     */
    template<size_t MATRIX_SIZE>
    void AddLhsMultipleValues(unsigned* matrixRowAndColIndices, c_matrix<double, MATRIX_SIZE, MATRIX_SIZE>& smallMatrix)
    {
        PetscInt matrix_row_indices[MATRIX_SIZE];
        PetscInt num_rows_owned=0;
		PetscInt global_row;

        for (unsigned row = 0; row<MATRIX_SIZE; row++)
        {
            global_row = matrixRowAndColIndices[row];
            if (global_row >=mOwnershipRangeLo && global_row <mOwnershipRangeHi)
            {
                matrix_row_indices[num_rows_owned++] = global_row;
            }
        }
        
        if ( num_rows_owned == MATRIX_SIZE)
        {
	        MatSetValues(mLhsMatrix,
	                     num_rows_owned,
	                     matrix_row_indices,
	                     MATRIX_SIZE,
	                     (PetscInt*) matrixRowAndColIndices,
	                     smallMatrix.data(),
	                     ADD_VALUES);
        }
        else
        {
        	// We need continuous data, if some of the rows do not belong to the processor their values
        	// are not passed to MatSetValues 
        	double values[MATRIX_SIZE*MATRIX_SIZE];
        	unsigned num_values_owned = 0;
			for (unsigned row = 0; row<MATRIX_SIZE; row++)
			{
				global_row = matrixRowAndColIndices[row];
				if (global_row >=mOwnershipRangeLo && global_row <mOwnershipRangeHi)
				{
					for (unsigned col=0; col<MATRIX_SIZE; col++)
					{
						values[num_values_owned++] = smallMatrix(row,col);
					}
				}
			}
			
	        MatSetValues(mLhsMatrix,
	                     num_rows_owned,
	                     matrix_row_indices,
	                     MATRIX_SIZE,
	                     (PetscInt*) matrixRowAndColIndices,
	                     values,
	                     ADD_VALUES);			
        }
    };

    /**
     * Add multiple values to the RHS vector.
     * 
     * @param vectorIndices mapping from index of the ublas vector (see param below)
     *  to index of the vector of this linear system
     * @param smallVector Ublas vector containing the values to be added
     *
     * N.B. Values which are not local (ie the row is not owned) will be skipped.
     */
    template<size_t VECTOR_SIZE>
    void AddRhsMultipleValues(unsigned* vectorIndices, c_vector<double, VECTOR_SIZE>& smallVector)
    {
        PetscInt indices_owned[VECTOR_SIZE];
        PetscInt num_indices_owned=0;
		PetscInt global_row;

        for (unsigned row = 0; row<VECTOR_SIZE; row++)
        {
            global_row = vectorIndices[row];
            if (global_row >=mOwnershipRangeLo && global_row <mOwnershipRangeHi)
            {
                indices_owned[num_indices_owned++] = global_row;
            }
        }
        
        if (num_indices_owned == VECTOR_SIZE)
        {
	        VecSetValues(mRhsVector,
	                     num_indices_owned,
	                     indices_owned,
	                     smallVector.data(),
	                     ADD_VALUES);
        }
        else
        {
        	// We need continuous data, if some of the rows do not belong to the processor their values
        	// are not passed to MatSetValues 
        	double values[VECTOR_SIZE];
        	unsigned num_values_owned = 0;
        	
	        for (unsigned row = 0; row<VECTOR_SIZE; row++)
    	    {
	            global_row = vectorIndices[row];
	            if (global_row >=mOwnershipRangeLo && global_row <mOwnershipRangeHi)
	            {
	                values[num_values_owned++] = smallVector(row);
	            }
	        }
	        
	        VecSetValues(mRhsVector,
	                     num_indices_owned,
	                     indices_owned,
	                     values,
	                     ADD_VALUES);	        			        	
        }
    }
};

#endif //_LINEARSYSTEM_HPP_
