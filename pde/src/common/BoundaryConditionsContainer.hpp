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
#ifndef _BOUNDARYCONDITIONSCONTAINER_HPP_
#define _BOUNDARYCONDITIONSCONTAINER_HPP_

#include <map>

#include "AbstractBoundaryConditionsContainer.hpp"
#include "AbstractBoundaryCondition.hpp"
#include "AbstractTetrahedralMesh.hpp"
#include "BoundaryElement.hpp"
#include "Node.hpp"
#include "LinearSystem.hpp"
#include "PetscException.hpp"
#include "ChastePoint.hpp"
#include "ConstBoundaryCondition.hpp"
#include "DistributedVectorFactory.hpp"


/**
 * Boundary Conditions Container
 *
 * This class contains a list of nodes on the Dirichlet boundary and associated Dirichlet
 * boundary conditions, and a list of surface elements on the Neumann boundary and associated
 * Neumann boundary conditions.
 *
 * \todo
 * Various operations are currently very inefficient - there is certainly scope for
 * optimisation here!
 */
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
class BoundaryConditionsContainer : public AbstractBoundaryConditionsContainer<ELEMENT_DIM,SPACE_DIM,PROBLEM_DIM>
{
public:

    /** Type of a read-only iterator over Neumann boundary conditions. */
    typedef typename std::map< const BoundaryElement<ELEMENT_DIM-1, SPACE_DIM>*, const AbstractBoundaryCondition<SPACE_DIM>* >::const_iterator
        NeumannMapIterator;

private:

    std::map< const BoundaryElement<ELEMENT_DIM-1, SPACE_DIM> *, const AbstractBoundaryCondition<SPACE_DIM>* >
        *mpNeumannMap[PROBLEM_DIM]; /**< List (map) of Neumann boundary conditions. */

    /**
     * Neumann boundary condition iterator.
     */
    NeumannMapIterator mLastNeumannCondition[PROBLEM_DIM];

    /**
     * Array storing whether there are any Neumann boundary conditions for each unknown.
     */
    bool mAnyNonZeroNeumannConditionsForUnknown[PROBLEM_DIM];

    /** A zero boundary condition, used for other unknowns in ApplyNeumannBoundaryCondition */
    ConstBoundaryCondition<SPACE_DIM> *mpZeroBoundaryCondition;

public:

    /**
     * Constructor calls base constuctor and allocates memory for the Neumann boundary
     * conditions lists.
     */
    BoundaryConditionsContainer();

    /**
     * Note that the destructor will delete memory for each boundary condition object, as
     * well as for the internal bookkeeping of this class.
     */
    ~BoundaryConditionsContainer();

    /**
     * Add a Dirichlet boundary condition specifying two parameters, a pointer to a node,
     * and a pointer to a boundary condition object associated with that node.
     *
     * The destructor for the BoundaryConditionsContainer will destroy the boundary
     * conditions objects.
     *
     * @param pBoundaryNode Pointer to a node on the boundary.
     * @param pBoundaryCondition Pointer to the Dirichlet boundary condition at that node.
     * @param indexOfUnknown defaults to 0
     * @param checkIfBoundaryNode defaults to true
     */
    void AddDirichletBoundaryCondition(const Node<SPACE_DIM>* pBoundaryNode,
                                       const AbstractBoundaryCondition<SPACE_DIM>* pBoundaryCondition,
                                       unsigned indexOfUnknown = 0,
                                       bool checkIfBoundaryNode = true);

    /**
     * Add a Neumann boundary condition specifying two parameters, a pointer to a
     * surface element, and a pointer to a boundary condition object associated with
     * that element.
     *
     * The destructor for the BoundaryConditionsContainer will destroy the boundary
     * conditions objects.
     *
     * Note that the value of a Neumann boundary condition should specify
     * D * grad(u).n, not just grad(u).n.
     *
     * Take care if using non-zero Neumann boundary conditions in 1d. If applied at
     * the left hand end you need to multiply the value by -1 to get the right answer.
     *
     * @param pBoundaryElement Pointer to an element on the boundary
     * @param pBoundaryCondition Pointer to the Neumann boundary condition on that element
     * @param indexOfUnknown defaults to 0
     */
    void AddNeumannBoundaryCondition(const BoundaryElement<ELEMENT_DIM-1, SPACE_DIM>* pBoundaryElement,
                                     const AbstractBoundaryCondition<SPACE_DIM>* pBoundaryCondition,
                                     unsigned indexOfUnknown = 0);

    /**
     * This function defines zero Dirichlet boundary conditions on every boundary node
     * of the mesh.
     *
     * @param pMesh Pointer to a mesh object, from which we extract the boundary
     * @param indexOfUnknown defaults to 0
     */
    void DefineZeroDirichletOnMeshBoundary(AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh,
                                           unsigned indexOfUnknown = 0);

    /**
     * This function defines constant Dirichlet boundary conditions on every boundary node
     * of the mesh.
     *
     * @param pMesh Pointer to a mesh object, from which we extract the boundary
     * @param value the value of the constant Dirichlet boundary condition
     * @param indexOfUnknown defaults to 0
     */
    void DefineConstantDirichletOnMeshBoundary(AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh,
                                               double value,
                                               unsigned indexOfUnknown = 0);

    /**
     * This function defines zero Neumann boundary conditions on every boundary element
     * of the mesh.
     *
     * @param pMesh Pointer to a mesh object, from which we extract the boundary
     * @param indexOfUnknown defaults to 0
     */
    void DefineZeroNeumannOnMeshBoundary(AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh,
                                         unsigned indexOfUnknown = 0);

    /**
     *  Alter the given linear system to satisfy Dirichlet boundary conditions.
     *
     *  If the number of unknowns is greater than one, it is assumed the solution vector is
     *  of the form (in the case of two unknowns u and v, and N nodes):
     *  solnvec = (U_1, V_1, U_2, V_2, ...., U_N, V_N)
     *
     *  @param rLinearSystem Linear system on which to apply boundary conditions
     *
     *  @param applyToMatrix This optional parameter can be set as false to
     *  ensure that the matrix of the linear system is not updated. To
     *  be used when the matrix does not change between time steps.
     */
    void ApplyDirichletToLinearProblem(LinearSystem& rLinearSystem,
                                       bool applyToMatrix = true);

    /**
     * Alter the residual vector for a nonlinear system to satisfy
     * Dirichlet boundary conditions.
     *
     * If the number of unknowns is greater than one, it is assumed the solution vector is
     * of the form (in the case of two unknowns u and v, and N nodes):
     * solnvec = (U_1, V_1, U_2, V_2, ...., U_N, V_N)
     *
     * @param currentSolution
     * @param residual
     * @param rFactory  the factory to use to create DistributedVector objects
     */
    void ApplyDirichletToNonlinearResidual(const Vec currentSolution, Vec residual,
                                           DistributedVectorFactory& rFactory);

    /**
     * Alter the Jacobian matrix vector for a nonlinear system to satisfy
     * Dirichlet boundary conditions.
     *
     * If the number of unknowns is greater than one, it is assumed the solution vector is
     * of the form (in the case of two unknowns u and v, and N nodes):
     * solnvec = (U_1, V_1, U_2, V_2, ...., U_N, V_N)
     *
     * @param jacobian
     */
    void ApplyDirichletToNonlinearJacobian(Mat jacobian);

    /**
     * Check that we have boundary conditions defined everywhere on mesh boundary.
     *
     * We iterate over all surface elements, and check either that they have an
     * associated Neumann condition, or that each node in the element has an
     * associated Dirichlet condition.
     *
     * \todo Might we want to throw an exception specifying which node failed?
     * What about checking for multiple conditions at a point (might be intentional)?
     *
     * @param pMesh Pointer to the mesh to check for validity.
     * @return true iff all boundaries have boundary conditions defined.
     */
    bool Validate(AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh);

    /**
     * Obtain value of Neumann boundary condition at a specified point in a given surface element
     *
     * It is up to the user to ensure that the point x is contained in the surface element.
     *
     * @param pSurfaceElement pointer to a boundary element
     * @param rX a point
     * @param indexOfUnknown defaults to 0
     */
    double GetNeumannBCValue(const BoundaryElement<ELEMENT_DIM-1,SPACE_DIM>* pSurfaceElement,
                             const ChastePoint<SPACE_DIM>& rX,
                             unsigned indexOfUnknown = 0);

    /**
     * Test if there is a Neumann boundary condition defined on the given element.
     * Used by SimpleLinearEllipticAssembler.
     *
     * \todo
     * This is a horrendously inefficient fix. Perhaps have flag in element object?
     *
     * @param pSurfaceElement pointer to a boundary element
     * @param indexOfUnknown defaults to 0
     */
    bool HasNeumannBoundaryCondition(const BoundaryElement<ELEMENT_DIM-1,SPACE_DIM>* pSurfaceElement,
                                     unsigned indexOfUnknown = 0);

    /**
     * @return whether there are any non-zero Neuman boundary conditions
     */
    bool AnyNonZeroNeumannConditions();

    /**
     * @return iterator pointing to the first Neumann boundary condition
     */
    NeumannMapIterator BeginNeumann();

    /**
     * @return iterator pointing to one past the last Neumann boundary condition
     */
    NeumannMapIterator EndNeumann();
};


#endif //_BOUNDARYCONDITIONSCONTAINER_HPP_
