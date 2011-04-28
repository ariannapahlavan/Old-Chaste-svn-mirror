/*

Copyright (C) University of Oxford, 2005-2011

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

#ifndef ABSTRACTNONLINEARELASTICITYSOLVER_HPP_
#define ABSTRACTNONLINEARELASTICITYSOLVER_HPP_

#include <vector>
#include <cmath>
#include "LinearSystem.hpp"
#include "OutputFileHandler.hpp"
#include "LogFile.hpp"
#include "PetscTools.hpp"
#include "MechanicsEventHandler.hpp"
#include "ReplicatableVector.hpp"
#include "FourthOrderTensor.hpp"
#include "QuadraticMesh.hpp"
#include "GaussianQuadratureRule.hpp"
#include "CmguiDeformedSolutionsWriter.hpp"
#include "AbstractMaterialLaw.hpp"
#include "Warnings.hpp"


//#include "PCBlockDiagonalMechanics.hpp"
//#include "PCLDUFactorisationMechanics.hpp"


//#define MECH_VERBOSE      // Print output on how nonlinear solve is progressing
//#define MECH_VERY_VERBOSE // See number of elements done whilst assembling vectors or matrices
//#define MECH_USE_HYPRE    // uses HYPRE to solve linear systems, requires Petsc to be installed with HYPRE


//#define MECH_USE_MUMPS    // uses the direct solver MUMPTS to solve linear systems, requires Petsc to be installed with MUMPS
//#define MECH_KSP_MONITOR  // Print residual norm each iteration in linear solve (ie -ksp_monitor).



//EMTODO: allow change of max_iter, no linear systems, better preconditioner


#ifdef MECH_VERBOSE
#include "Timer.hpp"
#endif

typedef enum CompressibilityType_
{
    COMPRESSIBLE,
    INCOMPRESSIBLE
} CompressibilityType;


/**
 * Abstract nonlinear elasticity solver.
 */
template<unsigned DIM>
class AbstractNonlinearElasticitySolver
{
protected:
    /** Number of vertices per element */
    static const size_t NUM_VERTICES_PER_ELEMENT = DIM+1;
    /** Number of nodes per element */
    static const size_t NUM_NODES_PER_ELEMENT = (DIM+1)*(DIM+2)/2;      // assuming quadratic
    /** Number of nodes per boundary element */
    static const size_t NUM_NODES_PER_BOUNDARY_ELEMENT = DIM*(DIM+1)/2; // assuming quadratic


    /** Maximum absolute tolerance for Newton solve. The Newton solver uses the absolute tolerance
     *  corresponding to the specified relative tolerance, but has a max and min allowable absolute
     *  tolerance. (ie: if max_abs = 1e-7, min_abs = 1e-10, rel=1e-4: then if the norm of the 
     *  initial_residual (=a) is 1e-2, it will solve with tolerance 1e-7; if a=1e-5, it will solve
     *  with tolerance 1e-9; a=1e-9, it will solve with tolerance 1e-10.  */
    static double MAX_NEWTON_ABS_TOL;

    /** Minimum absolute tolerance for Newton solve. See documentation for MAX_NEWTON_ABS_TOL. */
    static double MIN_NEWTON_ABS_TOL;

    /** Relative tolerance for Newton solve. See documentation for MAX_NEWTON_ABS_TOL. */
    static double NEWTON_REL_TOL;


    /**
     *  The mesh to be solved on. Requires 6 nodes per triangle (or 10 per tetrahedron)
     *  as quadratic bases are used.
     */
    QuadraticMesh<DIM>* mpQuadMesh;

    /** Boundary elements with (non-zero) surface tractions defined on them */
    std::vector<BoundaryElement<DIM-1,DIM>*> mBoundaryElements;

    /** Gaussian quadrature rule */
    GaussianQuadratureRule<DIM>* mpQuadratureRule;

    /** Boundary Gaussian quadrature rule */
    GaussianQuadratureRule<DIM-1>* mpBoundaryQuadratureRule;
    

    /** Absolute tolerance for linear systems. Can be set by calling 
     *  SetKspAbsoluteTolerances(), but default to -1, in which case 
     *  a relative tolerance is used. */
    double mKspAbsoluteTol;

    /**
     * Number of degrees of freedom (equal to, in the incompressible case:
     * DIM*N + M if quadratic-linear bases are used, where there are N total 
     * nodes and M vertices; or DIM*N in the compressible case).
     */
    unsigned mNumDofs;


    /**
     *  Residual vector for the full nonlinear system, also the RHS vector in the linear
     *  system used to solve the nonlinear problem using Newton's method.
     */
    Vec mResidualVector;

    /**
     *  Jacobian matrix of the nonlinear system, LHS matrix for the linear system.
     */
    Mat mJacobianMatrix;

    /**
     *  Precondition matrix for the linear system
     *
     *  In the incompressible case:
     *  the preconditioner is the petsc LU factorisation of
     *
     *  Jp = [A B] in displacement-pressure block form,
     *       [C M]
     *
     *  where the A, B and C are the matrices in the normal jacobian,
     *  ie
     *
     *  J  = [A B]
     *       [C 0]
     *
     *  and M is the MASS MATRIX (ie integral phi_i phi_j dV, where phi_i are the
     *  pressure basis functions).
     */
    Mat mPreconditionMatrix;

    /** Body force vector */
    c_vector<double,DIM> mBodyForce;

    /** Mass density of the undeformed body (equal to the density of deformed body in the incompressible case) */
    double mDensity;

    /** All nodes (including non-vertices) which are fixed */
    std::vector<unsigned> mFixedNodes;

    /** The displacements of those nodes with displacement boundary conditions */
    std::vector<c_vector<double,DIM> > mFixedNodeDisplacements;

    /** Whether to write any output */
    bool mWriteOutput;

    /** Where to write output, relative to CHASTE_TESTOUTPUT */
    std::string mOutputDirectory;

    /** Output file handler */
    OutputFileHandler* mpOutputFileHandler;

    /** By default only the initial and final solutions are written. However, we may want to
     *  write the solutions after every Newton iteration, in which case
     *  the following should be set to true */
    bool mWriteOutputEachNewtonIteration;


    /**
     *  The current solution, in the form (assuming 2d):
     *    Incompressible problem: [u1 v1 u2 v2 ... uN vN p1 p2 .. pM]
     *    Compressible problem:   [u1 v1 u2 v2 ... uN vN]
     *  where there are N total nodes and M vertices
     */
    std::vector<double> mCurrentSolution;

    /**
     *  Storage space for a 4th order tensor used in assembling the Jacobian (to avoid repeated memory allocation)
     */
    FourthOrderTensor<DIM,DIM,DIM,DIM> dTdE;

    /** Number of Newton iterations taken in last solve */
    unsigned mNumNewtonIterations;

    /** Deformed position: mDeformedPosition[i](j) = x_j for node i */
    std::vector<c_vector<double,DIM> > mDeformedPosition;


    /**
     *  The surface tractions (which should really be non-zero) for the boundary elements in mBoundaryElements.
     */
    std::vector<c_vector<double,DIM> > mSurfaceTractions;

    /** An optionally provided (pointer to a) function, giving the body force as a function of undeformed position. */
    c_vector<double,DIM> (*mpBodyForceFunction)(c_vector<double,DIM>& X, double t);

    /**
     * An optionally provided (pointer to a) function, giving the surface traction as a function of
     * undeformed position.
     */
    c_vector<double,DIM> (*mpTractionBoundaryConditionFunction)(c_vector<double,DIM>& X, double t);

    /** Whether the functional version of the body force is being used or not */
    bool mUsingBodyForceFunction;

    /** Whether the functional version of the surface traction is being used or not */
    bool mUsingTractionBoundaryConditionFunction;
    
    /** This solver is for static problems, however the body force or surface tractions
     *  could be a function of time. The user should call SetCurrentTime() if this is
     *  the case
     */
    double mCurrentTime;

    /** This is equal to either COMPRESSIBLE or INCOMPRESSIBLE (see enumeration defined at top of file)
     * and is only used in computing mNumDofs and allocating matrix memory.
     */
    CompressibilityType mCompressibilityType;

    /**
     * Assemble the residual vector and/or Jacobian matrix (using the current solution stored
     * in mCurrentSolution, output going to mResidualVector and/or mJacobianMatrix).
     *
     * Must be overridden in concrete derived classes.
     *
     * @param assembleResidual A bool stating whether to assemble the residual vector.
     * @param assembleJacobian A bool stating whether to assemble the Jacobian matrix.
     */
    virtual void AssembleSystem(bool assembleResidual, bool assembleJacobian)=0;



    /**
     * Initialise the solver.
     *
     * @param pFixedNodeLocations
     */
    void Initialise(std::vector<c_vector<double,DIM> >* pFixedNodeLocations);


    /**
     * Allocates memory for the Jacobian and preconditioner matrices (larger number of non-zeros per row than with say linear problems)
     */
    void AllocateMatrixMemory();


    /**
     * Apply the Dirichlet boundary conditions to the linear system.
     *
     * @param applyToMatrix Whether to apply the boundary conditions to the matrix (as well as the vector)
     */
    void ApplyBoundaryConditions(bool applyToMatrix);

    /**
     *  Set up the residual vector (using the current solution), and get its
     *  scaled norm (Calculate |r|_2 / length(r), where r is residual vector)
     *  
     *  @param allowException Sometimes the current solution solution will be such 
     *   that the residual vector cannot be computed, as (say) the material law
     *   will throw an exception as the strains are too large. If this bool is
     *   set to true, the exception will be caught, and DBL_MAX returned as the 
     *   residual norm
     */
    double ComputeResidualAndGetNorm(bool allowException);

    /** Calculate |r|_2 / length(r), where r is the current residual vector */
    double CalculateResidualNorm();

    /**
     *  Simple helper function, computes Z = X + aY, where X and Z are std::vectors and Y a ReplicatableVector
     *  @param rX X
     *  @param rY Y (replicatable vector)
     *  @param a a
     *  @param rZ Z the returned vector
     */
    void VectorSum(std::vector<double>& rX, ReplicatableVector& rY, double a, std::vector<double>& rZ);

    /**
     *  Print to std::cout the residual norm for this s, ie ||f(x+su)|| where f is the residual vector,
     *  x the current solution and u the update vector
     *  @param s s
     *  @param residNorm residual norm.
     */
    void PrintLineSearchResult(double s, double residNorm);


    /**
     *  Take one newton step, by solving the linear system -Ju=f, (J the jacobian, f
     *  the residual, u the update), and picking s such that a_new = a_old + su (a
     *  the current solution) such |f(a)| is the smallest.
     *
     *  @return The current norm of the residual after the newton step.
     */
    double TakeNewtonStep();

    /**
     *  Using the update vector (of Newton's method), choose s such that ||f(x+su)|| is most decreased,
     *  where f is the residual vector, x the current solution (mCurrentSolution) and u the update vector.
     *  This checks s=1 first (most likely to be the current solution, then 0.9, 0.8.. until ||f|| starts
     *  increasing.
     *  @param solution The solution of the linear solve in newton's method, ie the update vector u.
     */
    double UpdateSolutionUsingLineSearch(Vec solution);


    /**
     * This function may be overloaded by subclasses. It is called after each Newton
     * iteration.
     *
     * @param counter current newton iteration number
     * @param normResidual norm of the residual
     */
    virtual void PostNewtonStep(unsigned counter, double normResidual);


    /**
     *  Simple (one-line function which just calls ComputeStressAndStressDerivative() on the material law given, using C,
     *  inv(C), and p as the input and with rT and rDTdE as the output. Overloaded by other assemblers (eg cardiac
     *  mechanics) which need to add extra terms to the stress.
     *
     *  @param pMaterialLaw The material law for this element
     *  @param rC The Lagrangian deformation tensor (F^T F)
     *  @param rInvC The inverse of C. Should be computed by the user.
     *  @param pressure The current pressure
     *  @param elementIndex Index of the current element
     *  @param currentQuadPointGlobalIndex The index (assuming an outer loop over elements and an inner
     *    loop over quadrature points), of the current quadrature point.
     *  @param rT The stress will be returned in this parameter
     *  @param rDTdE the stress derivative will be returned in this parameter, assuming
     *    the final parameter is true
     *  @param computeDTdE A boolean flag saying whether the stress derivative is
     *    required or not.
     */
    virtual void ComputeStressAndStressDerivative(AbstractMaterialLaw<DIM>* pMaterialLaw,
                                                  c_matrix<double,DIM,DIM>& rC,
                                                  c_matrix<double,DIM,DIM>& rInvC,
                                                  double pressure,
                                                  unsigned elementIndex,
                                                  unsigned currentQuadPointGlobalIndex,
                                                  c_matrix<double,DIM,DIM>& rT,
                                                  FourthOrderTensor<DIM,DIM,DIM,DIM>& rDTdE,
                                                  bool computeDTdE)
    {
        // just call the method on the material law
        pMaterialLaw->ComputeStressAndStressDerivative(rC,rInvC,pressure,rT,rDTdE,computeDTdE);
    }

public:

    /**
     * Constructor.
     *
     * @param pQuadMesh  the quadratic mesh
     * @param bodyForce  Body force density (for example, acceleration due to gravity)
     * @param density    density
     * @param outputDirectory output directory
     * @param fixedNodes std::vector of nodes which have a dirichlet boundary condition imposed on them
     * @param compressibilityType Should be equal to COMPRESSIBLE or INCOMPRESSIBLE (see enumeration defined at top of file)
     *   (depending on which concrete class is inheriting from this) and is only used in computing mNumDofs and allocating
     *   matrix memory.
     */
    AbstractNonlinearElasticitySolver(QuadraticMesh<DIM>* pQuadMesh,
                                      c_vector<double,DIM> bodyForce,
                                      double density,
                                      std::string outputDirectory,
                                      std::vector<unsigned>& fixedNodes,
                                      CompressibilityType compressibilityType);


    /**
     * Destructor.
     */
    virtual ~AbstractNonlinearElasticitySolver();

    /**
     * Solve the problem.
     *
     * @param tol tolerance used in Newton solve (defaults to -1.0)
     * @param maxNumNewtonIterations (defaults to INT_MAX)
     * @param quitIfNoConvergence (defaults to true)
     */
    void Solve(double tol=-1.0,
               unsigned maxNumNewtonIterations=INT_MAX,
               bool quitIfNoConvergence=true);

    /**
     * Write the current deformation of the nodes.
     *
     * @param fileName (stem)
     * @param counterToAppend append a counter to the file name
     *
     * For example:
     * WriteCurrentDeformation("solution") --> file called "solution.nodes"
     * WriteCurrentDeformation("solution",3) --> file called "solution_3.nodes"
     */
    void WriteCurrentDeformation(std::string fileName, int counterToAppend=-1);

    /**
     * Get number of Newton iterations taken in last solve.
     */
    unsigned GetNumNewtonIterations();

    /**
     * Set a function which gives body force as a function of X (undeformed position)
     * Whatever body force was provided in the constructor will now be ignored.
     *
     * @param pFunction the function, which should be a function of space and time
     *  Note that SetCurrentTime() should be called each timestep if the force changes with time
     */
    void SetFunctionalBodyForce(c_vector<double,DIM> (*pFunction)(c_vector<double,DIM>& X, double t));

    /**
     * Set whether to write any output.
     *
     * @param writeOutput (defaults to true)
     */
    void SetWriteOutput(bool writeOutput=true);

    /**
     *  By default only the original and converged solutions are written. Call this
     *  by get node positions written after every Newton step (mostly for
     *  debugging).
     *  @param writeOutputEachNewtonIteration whether to write each iteration
     */
    void SetWriteOutputEachNewtonIteration(bool writeOutputEachNewtonIteration=true)
    {
        mWriteOutputEachNewtonIteration = writeOutputEachNewtonIteration;
    }

    /** 
     *  Set the absolute tolerance to be used when solving the linear system.
     *  If this is not called a relative tolerance is used.
     *  @param kspAbsoluteTolerance the tolerance
     */
    void SetKspAbsoluteTolerance(double kspAbsoluteTolerance)
    {
        assert(kspAbsoluteTolerance>0);
        mKspAbsoluteTol = kspAbsoluteTolerance;
    }

    /**
     *  Get the current solution vector (advanced use only - for getting the deformed position use
     *  rGetDeformedPosition()
     */
    std::vector<double>& rGetCurrentSolution()
    {
        return mCurrentSolution;
    }

    /**
     * Specify traction boundary conditions (if this is not called zero surface
     * tractions are assumed. This method takes in a list of boundary elements
     * and a corresponding list of surface tractions.
     *
     * @param rBoundaryElements the boundary elements
     * @param rSurfaceTractions the corresponding tractions
     */
    void SetSurfaceTractionBoundaryConditions(std::vector<BoundaryElement<DIM-1,DIM>*>& rBoundaryElements,
                                              std::vector<c_vector<double,DIM> >& rSurfaceTractions);

    /**
     * Set a function which gives the surface traction as a function of X (undeformed position),
     * together with the surface elements which make up the Neumann part of the boundary.
     *
     * @param rBoundaryElements the boundary elements
     * @param pFunction the function, which should be a function of space and time.
     *  Note that SetCurrentTime() should be called each timestep if the traction changes with time
     */
    void SetFunctionalTractionBoundaryCondition(std::vector<BoundaryElement<DIM-1,DIM>*> rBoundaryElements,
                                                c_vector<double,DIM> (*pFunction)(c_vector<double,DIM>& X, double t));

    /**
     *  Get the deformed position. Note returnvalue[i](j) = x_j for node i.
     */
    std::vector<c_vector<double,DIM> >& rGetDeformedPosition();

    /** This solver is for static problems, however the body force or surface tractions
     *  could be a function of time. This method is for setting the time.
     *  @param time current time
     */
    void SetCurrentTime(double time)
    {
        mCurrentTime = time;
    }

    /** Convert the output to Cmgui format (placed in a folder called cmgui in the output directory). Writes the original mesh
     *  as solution_0.exnode and the (current) solution as solution_1.exnode
     */
    void CreateCmguiOutput();
};


///////////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////////


template<unsigned DIM>
void AbstractNonlinearElasticitySolver<DIM>::Initialise(std::vector<c_vector<double,DIM> >* pFixedNodeLocations)
{
    assert(mpQuadMesh);

    AllocateMatrixMemory();

    for (unsigned i=0; i<mFixedNodes.size(); i++)
    {
        assert(mFixedNodes[i] < mpQuadMesh->GetNumNodes());
    }

    mpQuadratureRule = new GaussianQuadratureRule<DIM>(3);
    mpBoundaryQuadratureRule = new GaussianQuadratureRule<DIM-1>(3);

    mCurrentSolution.resize(mNumDofs, 0.0);

    // compute the displacements at each of the fixed nodes, given the
    // fixed nodes locations.
    if (pFixedNodeLocations == NULL)
    {
        mFixedNodeDisplacements.clear();
        for (unsigned i=0; i<mFixedNodes.size(); i++)
        {
            mFixedNodeDisplacements.push_back(zero_vector<double>(DIM));
        }
    }
    else
    {
        assert(pFixedNodeLocations->size()==mFixedNodes.size());
        for (unsigned i=0; i<mFixedNodes.size(); i++)
        {
            unsigned index = mFixedNodes[i];
            c_vector<double,DIM> displacement = (*pFixedNodeLocations)[i] - mpQuadMesh->GetNode(index)->rGetLocation();
            mFixedNodeDisplacements.push_back(displacement);
        }
    }
    assert(mFixedNodeDisplacements.size()==mFixedNodes.size());
}



template<unsigned DIM>
void AbstractNonlinearElasticitySolver<DIM>::AllocateMatrixMemory()
{
    if(DIM==2)
    {
        // 2D: N elements around a point => 7N+3 non-zeros in that row? Assume N<=10 (structured mesh would have N_max=6) => 73.
        unsigned num_non_zeros = std::min(75u, mNumDofs);

        mResidualVector = PetscTools::CreateVec(mNumDofs);
        PetscTools::SetupMat(mJacobianMatrix, mNumDofs, mNumDofs, num_non_zeros, PETSC_DECIDE, PETSC_DECIDE);
        PetscTools::SetupMat(mPreconditionMatrix, mNumDofs, mNumDofs, num_non_zeros, PETSC_DECIDE, PETSC_DECIDE);

    }
    else
    {
        assert(DIM==3);

        mResidualVector = PetscTools::CreateVec(mNumDofs);

        // 3D: N elements around a point. nz < (3*10+6)N (lazy estimate). Better estimate is 23N+4?. Assume N<20 => 500ish

        // in 3d we get the number of containing elements for each node and use that to obtain an upper bound
        // for the number of non-zeros for each DOF associated with that node.

        int* num_non_zeros_each_row = new int[mNumDofs];
        for(unsigned i=0; i<mNumDofs; i++)
        {
            num_non_zeros_each_row[i] = 0;
        }

        for(unsigned i=0; i<mpQuadMesh->GetNumNodes(); i++)
        {
            // this upper bound neglects the fact that two containing elements will share the same nodes..
            // 4 = max num dofs associated with this node
            // 30 = 3*9+3 = 3 dimensions x 9 other nodes on this element   +  3 vertices with a pressure unknown
            unsigned num_non_zeros_upper_bound = 4 + 30*mpQuadMesh->GetNode(i)->GetNumContainingElements();

            num_non_zeros_upper_bound = std::min(num_non_zeros_upper_bound, mNumDofs);

            num_non_zeros_each_row[DIM*i + 0] = num_non_zeros_upper_bound;
            num_non_zeros_each_row[DIM*i + 1] = num_non_zeros_upper_bound;
            num_non_zeros_each_row[DIM*i + 2] = num_non_zeros_upper_bound;

            if(mCompressibilityType==INCOMPRESSIBLE)
            {
                //Could do !mpQuadMesh->GetNode(i)->IsInternal()
                if(i<mpQuadMesh->GetNumVertices()) // then this is a vertex
                {
                    num_non_zeros_each_row[DIM*mpQuadMesh->GetNumNodes() + i] = num_non_zeros_upper_bound;
                }
            }
        }

        // NOTE: PetscTools::SetupMat() or the below creates a MATAIJ matrix, which means the matrix will
        // be of type MATSEQAIJ if num_procs=1 and MATMPIAIJ otherwise. In the former case
        // MatSeqAIJSetPreallocation MUST be called [MatMPIAIJSetPreallocation will have
        // no effect (silently)], and vice versa in the latter case

        /// We want to allocate different numbers of non-zeros per row, which means
        /// PetscTools::SetupMat isn't that useful. We could call
        //PetscTools::SetupMat(mJacobianMatrix, mNumDofs, mNumDofs, 0, PETSC_DECIDE, PETSC_DECIDE);
        //PetscTools::SetupMat(mPreconditionMatrix, mNumDofs, mNumDofs, 0, PETSC_DECIDE, PETSC_DECIDE);
        /// but we would get warnings due to the lack allocation

        // possible todo: create a PetscTools::SetupMatNoAllocation()

        // In the future, when parallelising, remember to think about MAT_IGNORE_OFF_PROC_ENTRIES (see #1682)

#if (PETSC_VERSION_MAJOR == 2 && PETSC_VERSION_MINOR == 2) //PETSc 2.2
        MatCreate(PETSC_COMM_WORLD,PETSC_DECIDE,PETSC_DECIDE,mNumDofs,mNumDofs,&mJacobianMatrix);
        MatCreate(PETSC_COMM_WORLD,PETSC_DECIDE,PETSC_DECIDE,mNumDofs,mNumDofs,&mPreconditionMatrix);
#else //New API
        MatCreate(PETSC_COMM_WORLD,&mJacobianMatrix);
        MatCreate(PETSC_COMM_WORLD,&mPreconditionMatrix);
        MatSetSizes(mJacobianMatrix,PETSC_DECIDE,PETSC_DECIDE,mNumDofs,mNumDofs);
        MatSetSizes(mPreconditionMatrix,PETSC_DECIDE,PETSC_DECIDE,mNumDofs,mNumDofs);
#endif

        if(PetscTools::IsSequential())
        {
            MatSeqAIJSetPreallocation(mJacobianMatrix,     PETSC_NULL, num_non_zeros_each_row);
            MatSeqAIJSetPreallocation(mPreconditionMatrix, PETSC_NULL, num_non_zeros_each_row);
        }
        else
        {
            PetscInt lo, hi;
            VecGetOwnershipRange(mResidualVector, &lo, &hi);

            int* num_non_zeros_each_row_this_proc = new int[hi-lo];
            int* zero = new int[hi-lo];
            for(unsigned i=0; i<unsigned(hi-lo); i++)
            {
                num_non_zeros_each_row_this_proc[i] = num_non_zeros_each_row[lo+i];
                zero[i] = 0;
            }

            MatMPIAIJSetPreallocation(mJacobianMatrix,     PETSC_NULL, num_non_zeros_each_row_this_proc, PETSC_NULL, num_non_zeros_each_row_this_proc);
            MatMPIAIJSetPreallocation(mPreconditionMatrix, PETSC_NULL, num_non_zeros_each_row_this_proc, PETSC_NULL, num_non_zeros_each_row_this_proc);
        }

        MatSetFromOptions(mJacobianMatrix);
        MatSetFromOptions(mPreconditionMatrix);


        //unsigned total_non_zeros = 0;
        //for(unsigned i=0; i<mNumDofs; i++)
        //{
        //   total_non_zeros += num_non_zeros_each_row[i];
        //}
        //std::cout << total_non_zeros << " versus " << 500*mNumDofs << "\n" << std::flush;

        delete [] num_non_zeros_each_row;
    }
}



template<unsigned DIM>
void AbstractNonlinearElasticitySolver<DIM>::ApplyBoundaryConditions(bool applyToMatrix)
{
    assert(mFixedNodeDisplacements.size()==mFixedNodes.size());

    // The boundary conditions on the NONLINEAR SYSTEM are x=boundary_values
    // on the boundary nodes. However:
    // The boundary conditions on the LINEAR SYSTEM  Ju=f, where J is the
    // u the negative update vector and f is the residual is
    // u=current_soln-boundary_values on the boundary nodes

    std::vector<unsigned> rows;
    if(applyToMatrix)
    {
        rows.resize(DIM*mFixedNodes.size());
    }

    for (unsigned i=0; i<mFixedNodes.size(); i++)
    {
        unsigned node_index = mFixedNodes[i];
        for (unsigned j=0; j<DIM; j++)
        {
            unsigned dof_index = DIM*node_index+j;

            if(applyToMatrix)
            {
                rows[DIM*i+j] = dof_index;
            }

            double value = mCurrentSolution[dof_index] - mFixedNodeDisplacements[i](j);

            PetscVecTools::SetElement(mResidualVector, dof_index, value);
        }
    }

    if(applyToMatrix)
    {
        PetscMatTools::ZeroRowsWithValueOnDiagonal(mJacobianMatrix, rows, 1.0);
        PetscMatTools::ZeroRowsWithValueOnDiagonal(mPreconditionMatrix, rows, 1.0);
    }
}

template<unsigned DIM>
double AbstractNonlinearElasticitySolver<DIM>::ComputeResidualAndGetNorm(bool allowException)
{
    if(!allowException)
    {
        // assemble the residual
        AssembleSystem(true, false);
    }
    else
    {
        try
        {
            // try to assemble the residual using this current solution
            AssembleSystem(true, false);
        }
        catch(Exception& e)
        {
            // if fail (because eg ODEs fail to solve, or strains are too large for material law), 
            // return infinity
            return DBL_MAX;
        }
    }

    // return the scaled norm of the residual
    return CalculateResidualNorm();
}

template<unsigned DIM>
double AbstractNonlinearElasticitySolver<DIM>::CalculateResidualNorm()
{
    double norm;
    VecNorm(mResidualVector, NORM_2, &norm);
    return norm/mNumDofs;
}

template<unsigned DIM>
void AbstractNonlinearElasticitySolver<DIM>::VectorSum(std::vector<double>& rX,
                                                       ReplicatableVector& rY,
                                                       double a,
                                                       std::vector<double>& rZ)
{
    assert(rX.size()==rY.GetSize());
    assert(rY.GetSize()==rZ.size());
    for(unsigned i=0; i<rX.size(); i++)
    {
        rZ[i] = rX[i] + a*rY[i];
    }
}


template<unsigned DIM>
double AbstractNonlinearElasticitySolver<DIM>::TakeNewtonStep()
{
    #ifdef MECH_VERBOSE
    Timer::Reset();
    #endif

    /////////////////////////////////////////////////////////////
    // Assemble Jacobian (and preconditioner)
    /////////////////////////////////////////////////////////////
    MechanicsEventHandler::BeginEvent(MechanicsEventHandler::ASSEMBLE);
    AssembleSystem(true, true);
    MechanicsEventHandler::EndEvent(MechanicsEventHandler::ASSEMBLE);
    #ifdef MECH_VERBOSE
    Timer::PrintAndReset("AssembleSystem");
    #endif


    ///////////////////////////////////////////////////////////////////
    //
    // Solve the linear system.
    // Three alternatives
    //   (a) GMRES with ILU preconditioner (or bjacobi=ILU on each process) [default]. Very poor on large problems.
    //   (b) GMRES with AMG preconditioner. Uncomment #define MECH_USE_HYPRE above. Requires Petsc3 with HYPRE installed.
    //   (c) Solve using MUMPS direct solver Uncomment #define MECH_USE_MUMPS above. Requires Petsc 3 with MUMPS installed.
    //
    ///////////////////////////////////////////////////////////////////
    MechanicsEventHandler::BeginEvent(MechanicsEventHandler::SOLVE);

    Vec solution;
    VecDuplicate(mResidualVector,&solution);

    KSP solver;
    KSPCreate(PETSC_COMM_WORLD,&solver);
    PC pc;
    KSPGetPC(solver, &pc);
    
    #ifdef MECH_USE_MUMPS
        // no need for the precondition matrix
        KSPSetOperators(solver, mJacobianMatrix, mJacobianMatrix, DIFFERENT_NONZERO_PATTERN /*in precond between successive solves*/);

        KSPSetType(solver, KSPPREONLY);
        PCSetType(pc, PCLU);
        PCFactorSetMatSolverPackage(pc, MAT_SOLVER_MUMPS);

        // Allow matrices with zero diagonals to be solved
        PCFactorSetShiftNonzero(pc, PETSC_DECIDE);
        // "Do LU factorization in-place (saves memory)"
        PCASMSetUseInPlace(pc);
    #else
        KSPSetOperators(solver, mJacobianMatrix, mPreconditionMatrix, DIFFERENT_NONZERO_PATTERN /*in precond between successive solves*/);

        unsigned num_restarts = 100;
        KSPSetType(solver,KSPGMRES);
        KSPGMRESSetRestart(solver,num_restarts); // gmres num restarts
    
        if(mKspAbsoluteTol < 0)
        {
            double ksp_rel_tol = 1e-6;
            KSPSetTolerances(solver, ksp_rel_tol, PETSC_DEFAULT, PETSC_DEFAULT, PETSC_DEFAULT /* max iters */); // Note, max iters seems to be 1000 whatever we give here
        }
        else
        {
            KSPSetTolerances(solver, 1e-16, mKspAbsoluteTol, PETSC_DEFAULT, PETSC_DEFAULT /* max iters */); // Note, max iters seems to be 1000 whatever we give here
        }

        #ifndef MECH_USE_HYPRE 
            PCSetType(pc, PCBJACOBI); // BJACOBI = ILU on each block (block = part of matrix on each process)
        #else
            /////////////////////////////////////////////////////////////////////////////////////////////////////
            // Speed up linear solve time massively for larger simulations (in fact GMRES may stagnate without
            // this for larger problems), by using a AMG preconditioner -- needs HYPRE installed 
            /////////////////////////////////////////////////////////////////////////////////////////////////////
            PetscOptionsSetValue("-pc_hypre_type", "boomeramg");
            // PetscOptionsSetValue("-pc_hypre_boomeramg_max_iter", "1");
            // PetscOptionsSetValue("-pc_hypre_boomeramg_strong_threshold", "0.0");
        
            PCSetType(pc, PCHYPRE);
            KSPSetPreconditionerSide(solver, PC_RIGHT);
            
            // other possible preconditioners..
            //PCBlockDiagonalMechanics* p_custom_pc = new PCBlockDiagonalMechanics(solver, mPreconditionMatrix, mBlock1Size, mBlock2Size);
            //PCLDUFactorisationMechanics* p_custom_pc = new PCLDUFactorisationMechanics(solver, mPreconditionMatrix, mBlock1Size, mBlock2Size);
        	//remember to delete memory..
            //KSPSetPreconditionerSide(solver, PC_RIGHT);
        #endif

        #ifdef MECH_KSP_MONITOR
        PetscOptionsSetValue("-ksp_monitor","");
        #endif

        KSPSetFromOptions(solver);
        KSPSetUp(solver);
    #endif     

    #ifdef MECH_VERBOSE
    Timer::PrintAndReset("KSP Setup");
    #endif
    
    KSPSolve(solver,mResidualVector,solution);
    
    int num_iters;
    KSPGetIterationNumber(solver, &num_iters);
    if(num_iters==0)
    {
        VecDestroy(solution);
        KSPDestroy(solver);
        EXCEPTION("KSP Absolute tolerance was too high, linear system wasn't solved - there will be no decrease in Newton residual. Decrease KspAbsoluteTolerance");
    }

    // See comment on max_iters above
    if(num_iters==1000)
    {
        #define COVERAGE_IGNORE
        WARNING("Linear solver in mechanics solve may not have converged");
        #undef COVERAGE_IGNORE
    }

    
    #ifdef MECH_VERBOSE
    Timer::PrintAndReset("KSP Solve");
    std::cout << "[" << PetscTools::GetMyRank() << "]: Num iterations = " << num_iters << "\n" << std::flush;
    #endif

    MechanicsEventHandler::EndEvent(MechanicsEventHandler::SOLVE);


    ///////////////////////////////////////////////////////////////////////////
    // Update the solution
    //  Newton method:       sol = sol - update, where update=Jac^{-1}*residual
    //  Newton with damping: sol = sol - s*update, where s is chosen
    //   such that |residual(sol)| is minimised. Damping is important to
    //   avoid initial divergence.
    //
    // Normally, finding the best s from say 0.05,0.1,0.2,..,1.0 is cheap,
    // but this is not the case in cardiac electromechanics calculations.
    // Therefore, we initially check s=1 (expected to be the best most of the
    // time, then s=0.9. If the norm of the residual increases, we assume
    // s=1 is the best. Otherwise, check s=0.8 to see if s=0.9 is a local min.
    ///////////////////////////////////////////////////////////////////////////
    MechanicsEventHandler::BeginEvent(MechanicsEventHandler::UPDATE);
    double new_norm_resid = UpdateSolutionUsingLineSearch(solution);
    MechanicsEventHandler::EndEvent(MechanicsEventHandler::UPDATE);

    VecDestroy(solution);
    KSPDestroy(solver);

    return new_norm_resid;
}

template<unsigned DIM>
void AbstractNonlinearElasticitySolver<DIM>::PrintLineSearchResult(double s, double residNorm)
{
    #ifdef MECH_VERBOSE
    std::cout << "\tTesting s = " << s << ", |f| = " << residNorm << "\n" << std::flush;
    #endif
}

template<unsigned DIM>
double AbstractNonlinearElasticitySolver<DIM>::UpdateSolutionUsingLineSearch(Vec solution)
{
    double initial_norm_resid = CalculateResidualNorm();
    #ifdef MECH_VERBOSE
    std::cout << "\tInitial |f| [corresponding to s=0] is " << initial_norm_resid << "\n"  << std::flush;
    #endif

    ReplicatableVector update(solution);

    std::vector<double> old_solution = mCurrentSolution;

    std::vector<double> damping_values; // = {1.0, 0.9, .., 0.2, 0.1, 0.05} ie size 11
    for (unsigned i=10; i>=1; i--)
    {
        damping_values.push_back((double)i/10.0);
    }
    damping_values.push_back(0.05);
    assert(damping_values.size()==11);


    //// Try s=1 and see what the residual-norm is
    // let mCurrentSolution = old_solution - damping_val[0]*update; and compute residual
    unsigned index = 0;
    VectorSum(old_solution, update, -damping_values[index], mCurrentSolution);
    double current_resid_norm = ComputeResidualAndGetNorm(true);
    PrintLineSearchResult(damping_values[index], current_resid_norm);

    //// Try s=0.9 and see what the residual-norm is
    // let mCurrentSolution = old_solution - damping_val[1]*update; and compute residual
    index = 1;
    VectorSum(old_solution, update, -damping_values[index], mCurrentSolution);
    double next_resid_norm = ComputeResidualAndGetNorm(true);
    PrintLineSearchResult(damping_values[index], next_resid_norm);

    index = 2;
    // While f(s_next) < f(s_current), [f = residnorm], keep trying new damping values,
    // ie exit thus loop when next norm of the residual first increases
    while (    (next_resid_norm==DBL_MAX) // the residual is returned as infinity if the deformation is so large to cause exceptions in the material law/EM contraction model
            || ( (next_resid_norm < current_resid_norm) && (index<damping_values.size()) ) )
    {
        current_resid_norm = next_resid_norm;

        // let mCurrentSolution = old_solution - damping_val*update; and compute residual
        VectorSum(old_solution, update, -damping_values[index], mCurrentSolution);
        next_resid_norm = ComputeResidualAndGetNorm(true);
        PrintLineSearchResult(damping_values[index], next_resid_norm);

        index++;
    }

    unsigned best_index;

    if(index==damping_values.size() && (next_resid_norm < current_resid_norm))
    {
        // Difficult to come up with large forces/tractions such that it had to
        // test right down to s=0.05, but overall doesn't fail.
        // The possible damping values have been manually temporarily altered to
        // get this code to be called, it appears to work correctly. Even if it
        // didn't tests wouldn't fail, they would just be v. slightly less efficient.
        #define COVERAGE_IGNORE
        // if we exited because we got to the end of the possible damping values, the
        // best one was the last one (excl the final index++ at the end)
        current_resid_norm = next_resid_norm;
        best_index = index-1;
        #undef COVERAGE_IGNORE
    }
    else
    {
        // else the best one must have been the second last one (excl the final index++ at the end)
        // (as we would have exited when the resid norm first increased)
        best_index = index-2;
    }

    // check out best was better than the original residual-norm
    if (initial_norm_resid < current_resid_norm)
    {
        #define COVERAGE_IGNORE
        // Have to use an assert/exit here as the following exception causes a seg fault (in cardiac mech problems?)
        // Don't know why
        std::cout << "CHASTE ERROR: (AbstractNonlinearElasticitySolver.hpp): Residual does not appear to decrease in newton direction, quitting.\n" << std::flush;
        exit(0);
        //EXCEPTION("Residual does not appear to decrease in newton direction, quitting");
        #undef COVERAGE_IGNORE
    }

    #ifdef MECH_VERBOSE
    std::cout << "\tBest s = " << damping_values[best_index] << "\n"  << std::flush;
    #endif
    VectorSum(old_solution, update, -damping_values[best_index], mCurrentSolution);

    return current_resid_norm;

//// old (standard) version - check all s=0.05,0.1,0.2,..,0.9,1,1.1; and pick the best
//        double best_norm_resid = DBL_MAX;
//        double best_damping_value = 0.0;
//
//        std::vector<double> damping_values;
//        damping_values.reserve(12);
//        damping_values.push_back(0.0);
//        damping_values.push_back(0.05);
//        for (unsigned i=1; i<=10; i++)
//        {
//            damping_values.push_back((double)i/10.0);
//        }
//
//        for (unsigned i=0; i<damping_values.size(); i++)
//        {
//            for (unsigned j=0; j<mNumDofs; j++)
//            {
//                mCurrentSolution[j] = old_solution[j] - damping_values[i]*update[j];
//            }
//
//            // compute residual
//            double norm_resid = ComputeResidualAndGetNorm();
//
//            std::cout << "\tTesting s = " << damping_values[i] << ", |f| = " << norm_resid << "\n" << std::flush;
//            if (norm_resid < best_norm_resid)
//            {
//                best_norm_resid = norm_resid;
//                best_damping_value = damping_values[i];
//            }
//        }
//
//        if (best_damping_value == 0.0)
//        {
//            #define COVERAGE_IGNORE
//            assert(0);
//            EXCEPTION("Residual does not decrease in newton direction, quitting");
//            #undef COVERAGE_IGNORE
//        }
//        else
//        {
//            std::cout << "\tBest s = " << best_damping_value << "\n"  << std::flush;
//        }
//        //Timer::PrintAndReset("Find best damping");
//
//        // implement best update and recalculate residual
//        for (unsigned j=0; j<mNumDofs; j++)
//        {
//            mCurrentSolution[j] = old_solution[j] - best_damping_value*update[j];
//        }
}



template<unsigned DIM>
void AbstractNonlinearElasticitySolver<DIM>::PostNewtonStep(unsigned counter, double normResidual)
{
}


template<unsigned DIM>
AbstractNonlinearElasticitySolver<DIM>::AbstractNonlinearElasticitySolver(QuadraticMesh<DIM>* pQuadMesh,
                                                                          c_vector<double,DIM> bodyForce,
                                                                          double density,
                                                                          std::string outputDirectory,
                                                                          std::vector<unsigned>& fixedNodes,
                                                                          CompressibilityType compressibilityType)
    : mpQuadMesh(pQuadMesh),
      mpQuadratureRule(NULL),
      mpBoundaryQuadratureRule(NULL),
      mKspAbsoluteTol(-1),
      mResidualVector(NULL),
      mJacobianMatrix(NULL),
      mPreconditionMatrix(NULL),
      mBodyForce(bodyForce),
      mDensity(density),
      mFixedNodes(fixedNodes),
      mOutputDirectory(outputDirectory),
      mpOutputFileHandler(NULL),
      mWriteOutputEachNewtonIteration(false),
      mNumNewtonIterations(0),
      mUsingBodyForceFunction(false),
      mUsingTractionBoundaryConditionFunction(false),
      mCurrentTime(0.0),
      mCompressibilityType(compressibilityType)
{
    assert(DIM==2 || DIM==3);
    assert(density > 0);
    assert(fixedNodes.size() > 0);
    assert(pQuadMesh != NULL);

    if(mCompressibilityType==COMPRESSIBLE)
    {
        mNumDofs = DIM*mpQuadMesh->GetNumNodes();
    }
    else
    {
        mNumDofs = DIM*mpQuadMesh->GetNumNodes() + mpQuadMesh->GetNumVertices();
    }

    mWriteOutput = (mOutputDirectory != "");
    if(mWriteOutput)
    {
        mpOutputFileHandler = new OutputFileHandler(mOutputDirectory);
    }
}



template<unsigned DIM>
AbstractNonlinearElasticitySolver<DIM>::~AbstractNonlinearElasticitySolver()
{
    if(mResidualVector)
    {
        VecDestroy(mResidualVector);
        MatDestroy(mJacobianMatrix);
        MatDestroy(mPreconditionMatrix);
    }

    if(mpQuadratureRule)
    {
        delete mpQuadratureRule;
        delete mpBoundaryQuadratureRule;
    }
    if(mpOutputFileHandler)
    {
        delete mpOutputFileHandler;
    }
}


template<unsigned DIM>
void AbstractNonlinearElasticitySolver<DIM>::Solve(double tol,
                                                   unsigned maxNumNewtonIterations,
                                                   bool quitIfNoConvergence)
{
    WriteCurrentDeformation("initial");


    if(mWriteOutputEachNewtonIteration)
    {
        WriteCurrentDeformation("newton_iteration", 0);
    }

    // compute residual
    double norm_resid = ComputeResidualAndGetNorm(false);
    #ifdef MECH_VERBOSE
    std::cout << "\nNorm of residual is " << norm_resid << "\n";
    #endif

    mNumNewtonIterations = 0;
    unsigned iteration_number = 1;

    if (tol < 0) // ie if wasn't passed in as a parameter
    {
        tol = NEWTON_REL_TOL*norm_resid;

        #define COVERAGE_IGNORE // not going to have tests in cts for everything
        if (tol > MAX_NEWTON_ABS_TOL)
        {
            tol = MAX_NEWTON_ABS_TOL;
        }
        if (tol < MIN_NEWTON_ABS_TOL)
        {
            tol = MIN_NEWTON_ABS_TOL;
        }
        #undef COVERAGE_IGNORE
    }

    #ifdef MECH_VERBOSE
    std::cout << "Solving with tolerance " << tol << "\n";
    #endif

    while (norm_resid > tol && iteration_number<=maxNumNewtonIterations)
    {
        #ifdef MECH_VERBOSE
        std::cout <<  "\n-------------------\n"
                  <<   "Newton iteration " << iteration_number
                  << ":\n-------------------\n";
        #endif

        // take newton step (and get returned residual)
        norm_resid = TakeNewtonStep();

        #ifdef MECH_VERBOSE
        std::cout << "Norm of residual is " << norm_resid << "\n";
        #endif
        if (mWriteOutputEachNewtonIteration)
        {
            WriteCurrentDeformation("newton_iteration", iteration_number);
        }

        mNumNewtonIterations = iteration_number;

        PostNewtonStep(iteration_number,norm_resid);

        iteration_number++;
        if (iteration_number==20)
        {
            #define COVERAGE_IGNORE
            EXCEPTION("Not converged after 20 newton iterations, quitting");
            #undef COVERAGE_IGNORE
        }
    }

    if ((norm_resid > tol) && quitIfNoConvergence)
    {
        #define COVERAGE_IGNORE
        EXCEPTION("Failed to converge");
        #undef COVERAGE_IGNORE
    }

    // write the final solution
    WriteCurrentDeformation("solution");
}


template<unsigned DIM>
void AbstractNonlinearElasticitySolver<DIM>::WriteCurrentDeformation(std::string fileName, int counterToAppend)
{
    // only write output if the flag mWriteOutput has been set
    if (!mWriteOutput)
    {
        return;
    }

    std::stringstream file_name;
    file_name << fileName;
    if(counterToAppend >= 0)
    {
        file_name << "_" << counterToAppend;
    }
    file_name << ".nodes";

    out_stream p_file = mpOutputFileHandler->OpenOutputFile(file_name.str());

    std::vector<c_vector<double,DIM> >& r_deformed_position = rGetDeformedPosition();
    for (unsigned i=0; i<r_deformed_position.size(); i++)
    {
        for (unsigned j=0; j<DIM; j++)
        {
           * p_file << r_deformed_position[i](j) << " ";
        }
       * p_file << "\n";
    }
    p_file->close();
}


template<unsigned DIM>
unsigned AbstractNonlinearElasticitySolver<DIM>::GetNumNewtonIterations()
{
    return mNumNewtonIterations;
}


template<unsigned DIM>
void AbstractNonlinearElasticitySolver<DIM>::SetFunctionalBodyForce(c_vector<double,DIM> (*pFunction)(c_vector<double,DIM>& X, double t))
{
    mUsingBodyForceFunction = true;
    mpBodyForceFunction = pFunction;
}


template<unsigned DIM>
void AbstractNonlinearElasticitySolver<DIM>::SetWriteOutput(bool writeOutput)
{
    if (writeOutput && (mOutputDirectory==""))
    {
        EXCEPTION("Can't write output if no output directory was given in constructor");
    }
    mWriteOutput = writeOutput;
}

template<unsigned DIM>
void AbstractNonlinearElasticitySolver<DIM>::SetSurfaceTractionBoundaryConditions(
            std::vector<BoundaryElement<DIM-1,DIM>*>& rBoundaryElements,
            std::vector<c_vector<double,DIM> >& rSurfaceTractions)
{
    assert(rBoundaryElements.size()==rSurfaceTractions.size());
    mBoundaryElements = rBoundaryElements;
    mSurfaceTractions = rSurfaceTractions;
}

template<unsigned DIM>
void AbstractNonlinearElasticitySolver<DIM>::SetFunctionalTractionBoundaryCondition(
            std::vector<BoundaryElement<DIM-1,DIM>*> rBoundaryElements,
            c_vector<double,DIM> (*pFunction)(c_vector<double,DIM>& X, double t))
{
    mBoundaryElements = rBoundaryElements;
    mUsingTractionBoundaryConditionFunction = true;
    mpTractionBoundaryConditionFunction = pFunction;
}


template<unsigned DIM>
std::vector<c_vector<double,DIM> >& AbstractNonlinearElasticitySolver<DIM>::rGetDeformedPosition()
{
    mDeformedPosition.resize(mpQuadMesh->GetNumNodes(), zero_vector<double>(DIM));
    for (unsigned i=0; i<mpQuadMesh->GetNumNodes(); i++)
    {
        for (unsigned j=0; j<DIM; j++)
        {
            mDeformedPosition[i](j) = mpQuadMesh->GetNode(i)->rGetLocation()[j] + mCurrentSolution[DIM*i+j];
        }
    }
    return mDeformedPosition;
}


template<unsigned DIM>
void AbstractNonlinearElasticitySolver<DIM>::CreateCmguiOutput()
{
    if(mOutputDirectory=="")
    {
        EXCEPTION("No output directory was given so no output was written, cannot convert to cmgui format");
    }

    CmguiDeformedSolutionsWriter<DIM> writer(mOutputDirectory + "/cmgui",
                                             "solution",
                                             *mpQuadMesh,
                                             WRITE_QUADRATIC_MESH);

    std::vector<c_vector<double,DIM> >& r_deformed_positions = rGetDeformedPosition();
    writer.WriteInitialMesh(); // this writes solution_0.exnode and .exelem
    writer.WriteDeformationPositions(r_deformed_positions, 1); // this writes the final solution as solution_1.exnode
    writer.WriteCmguiScript(); // writes LoadSolutions.com
}

//
// Constant setting definitions
//
template<unsigned DIM>
double AbstractNonlinearElasticitySolver<DIM>::MAX_NEWTON_ABS_TOL = 1e-7;

template<unsigned DIM>
double AbstractNonlinearElasticitySolver<DIM>::MIN_NEWTON_ABS_TOL = 1e-10;

template<unsigned DIM>
double AbstractNonlinearElasticitySolver<DIM>::NEWTON_REL_TOL = 1e-4;


#endif /*ABSTRACTNONLINEARELASTICITYSOLVER_HPP_*/
