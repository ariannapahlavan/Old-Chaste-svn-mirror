/*

Copyright (C) University of Oxford, 2008

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


#ifndef PETSCTOOLS_HPP_
#define PETSCTOOLS_HPP_


#include <vector>
#include <petsc.h>
#include <petscvec.h>
#include <petscmat.h>
#include <iostream>
#include <assert.h>
#include "DistributedVector.hpp"
#include "ReplicatableVector.hpp"

#define EXIT_IF_PARALLEL if(!PetscTools::IsSequential()){TS_TRACE("This test does not pass in parallel yet.");return;}

/**
 *  A helper class of static methods.
 */
class PetscTools
{
public :
    /**
     *  Just returns whether there is one process or not
     */
    static bool IsSequential()
    {
        return (NumProcs()==1);
    }
    /**
     *  Returns total number of processors
     */
    static unsigned NumProcs()
    {
        PetscTruth is_there;
        PetscInitialized(&is_there);
        if (!is_there)
        {
            return 1;
        }
        int num_procs;
        MPI_Comm_size(PETSC_COMM_WORLD, &num_procs);
        return num_procs;
    }
    /**
     *  Return our rank.
     *  Assumes PETSc has been initialized
     */
    static int GetMyRank()
    {
        PetscInt my_rank;
        MPI_Comm_rank(PETSC_COMM_WORLD, &my_rank);
        return my_rank;
    }
    /**
     *  Just returns whether it is the master process or not
     */
    static bool AmMaster()
    {
        PetscInt my_rank;
        MPI_Comm_rank(PETSC_COMM_WORLD, &my_rank);
        return (my_rank == 0);
    }
    /**
     * If MPI is set up, perform a barrier synchronisation.
     * If not, it's a noop.
     */
    static void Barrier()
    {
        PetscTruth is_there;
        PetscInitialized(&is_there);
        if (is_there)
        {
            PetscBarrier(PETSC_NULL);
        }
    }
 
    /**
     *  Create a vector of the specified size. SetFromOptions is called.
     */
    static Vec CreateVec(int size)
    {
        assert(size>0);
        Vec ret;
        VecCreate(PETSC_COMM_WORLD, &ret);
        VecSetSizes(ret, PETSC_DECIDE, size);
        VecSetFromOptions(ret);
        return ret;
    }
    
    /**
     *  Create a vector of the specified size with all values set to be the given
     *  constant. SetFromOptions is called.
     */
    static Vec CreateVec(int size, double value)
    {
        assert(size>0);
        Vec ret = CreateVec(size);
        
        #if (PETSC_VERSION_MINOR == 2) //Old API
        VecSet(&value, ret);
        #else
        VecSet(ret, value);
        #endif
        
        VecAssemblyBegin(ret);
        VecAssemblyEnd(ret);
        return ret;
    }
    
    /** 
     *  Create a Vec from the given data.
     */
    static Vec CreateVec(std::vector<double> data)
    {
        assert(data.size()>0);
        Vec ret = CreateVec(data.size());

        double* p_ret;
        VecGetArray(ret, &p_ret);
        int lo, hi;
        VecGetOwnershipRange(ret, &lo, &hi);
        
        for (int global_index=lo; global_index < hi; global_index++)
        {
            int local_index = global_index - lo;
            p_ret[local_index] = data[global_index];
        }
        VecRestoreArray(ret, &p_ret);
        VecAssemblyBegin(ret);
        VecAssemblyEnd(ret);
        
        return ret;
    }
    
    /**
     *  Set up a matrix - set the size using the given parameters, the type (default MATMPIAIJ). The
     *  number of local rows and columns is by default PETSC_DECIDE. SetFromOptions is called.
     * 
     *  @param maxColsPerRow The maximum number of non zeros per row. This value is problem dependend. An upper bound is (3^ELEMENT_DIM) * PROBLEM_DIM. The default value (3D bidomain problem) should be big enough for any of the problems being solved. 
     */
    static void SetupMat(Mat& rMat, int numRows, int numColumns, 
                         MatType matType=(MatType) MATMPIAIJ, 
                         int numLocalRows=PETSC_DECIDE, 
                         int numLocalColumns=PETSC_DECIDE,
                         int maxColsPerRow=54)
    {
        assert(numRows>0);
        assert(numColumns>0);
        
        #if (PETSC_VERSION_MINOR == 2) //Old API
        MatCreate(PETSC_COMM_WORLD,numLocalRows,numLocalColumns,numRows,numColumns,&rMat);
        #else //New API
        MatCreate(PETSC_COMM_WORLD,&rMat);
        MatSetSizes(rMat,numLocalRows,numLocalColumns,numRows,numColumns);
        #endif
    
        MatSetType(rMat, matType);        
        
        if (strcmp(matType,MATMPIAIJ)==0)
        {
            MatMPIAIJSetPreallocation(rMat, maxColsPerRow, PETSC_NULL, (PetscInt) (maxColsPerRow*0.5), PETSC_NULL);
        } 
        
        MatSetFromOptions(rMat);
    }
    
    /**
     * Ensure exceptions are handled cleanly in parallel code, by causing all processes to
     * throw if any one does.
     * 
     * @param flag is set to true if this process has thrown.
     */
    static void ReplicateException(bool flag)
    {
        unsigned my_error = (unsigned) flag;
        unsigned anyones_error;
        MPI_Allreduce(&my_error, &anyones_error, 1, MPI_UNSIGNED, MPI_SUM, PETSC_COMM_WORLD);
        if (flag)
        {
            // Return control to exception thrower
            return;
        }
        if (anyones_error)
        {
            EXCEPTION("Another process threw an exception; bailing out.");
        }
    }
    
    /**
     *  Another helper method to get a single value from a vector
     *  in 1 line than Petsc's usual 4 or 5. DOES NOT check that
     *  the requested component is local, DOES do bound-checking.
     */
    static double GetVecValue(Vec vec, unsigned index)
    {
        assert(vec);
        PetscInt size;
        VecGetSize(vec, &size);
        assert((int)index<size);
        
        double* p_data;
        VecGetArray(vec, &p_data);
        double ret = p_data[(int)index];
        VecRestoreArray(vec, &p_data);
        
        return ret;
    }
};

#endif /*PETSCTOOLS_HPP_*/
