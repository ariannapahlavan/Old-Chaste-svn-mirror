/*

Copyright (C) University of Oxford, 2005-2010

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

#ifndef MONODOMAINASSEMBLER_HPP_
#define MONODOMAINASSEMBLER_HPP_


#include "AbstractFeObjectAssembler.hpp"
#include "MonodomainCellCollection.hpp"

/**
 *  Assembler for assembling the LHS matrix and RHS vector of the linear
 *  systems solved in monodomain problems
 */
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
class MonodomainAssembler
   : public AbstractFeObjectAssembler<ELEMENT_DIM,SPACE_DIM,1,true,true,CARDIAC>
{
protected:
    /** The PDE to be solved. */
    MonodomainCellCollection<ELEMENT_DIM,SPACE_DIM>* mpMonodomainCellCollection;

    /** Local cache of the configuration singleton instance*/
    HeartConfig* mpConfig;

    /** Ionic current to be interpolated from cache*/
    double mIionic;
    /** Intracellular stimulus to be interpolated from cache*/
    double mIIntracellularStimulus;

    /** Timestep to use in LHS matrix or RHS vector */
    double mDt;

    /**
     * ComputeMatrixTerm()
     *
     * This method is called by AssembleOnElement() and tells the assembler
     * the contribution to add to the element stiffness matrix.
     *
     * @param rPhi The basis functions, rPhi(i) = phi_i, i=1..numBases
     * @param rGradPhi Basis gradients, rGradPhi(i,j) = d(phi_j)/d(X_i)
     * @param rX The point in space
     * @param rU The unknown as a vector, u(i) = u_i
     * @param rGradU The gradient of the unknown as a matrix, rGradU(i,j) = d(u_i)/d(X_j)
     * @param pElement Pointer to the element
     */
    c_matrix<double,1*(ELEMENT_DIM+1),1*(ELEMENT_DIM+1)> ComputeMatrixTerm(
                c_vector<double, ELEMENT_DIM+1> &rPhi,
                c_matrix<double, SPACE_DIM, ELEMENT_DIM+1> &rGradPhi,
                ChastePoint<SPACE_DIM> &rX,
                c_vector<double,1> &rU,
                c_matrix<double, 1, SPACE_DIM> &rGradU /* not used */,
                Element<ELEMENT_DIM,SPACE_DIM>* pElement);

    /**
     *  ComputeVectorTerm()
     *
     *  This method is called by AssembleOnElement() and tells the assembler
     *  the contribution to add to the element stiffness vector.
     *
     * @param rPhi The basis functions, rPhi(i) = phi_i, i=1..numBases
     * @param rGradPhi Basis gradients, rGradPhi(i,j) = d(phi_j)/d(X_i)
     * @param rX The point in space
     * @param rU The unknown as a vector, u(i) = u_i
     * @param rGradU The gradient of the unknown as a matrix, rGradU(i,j) = d(u_i)/d(X_j)
     * @param pElement Pointer to the element
     */
    c_vector<double,1*(ELEMENT_DIM+1)> ComputeVectorTerm(
                c_vector<double, ELEMENT_DIM+1> &rPhi,
                c_matrix<double, SPACE_DIM, ELEMENT_DIM+1> &rGradPhi /* not used */,
                ChastePoint<SPACE_DIM> &rX /* not used */,
                c_vector<double,1> &rU,
                c_matrix<double, 1, SPACE_DIM> &rGradU /* not used */,
                Element<ELEMENT_DIM,SPACE_DIM>* pElement /* not used */);

    /**
     * ComputeVectorSurfaceTerm()
     *
     * This method is called by AssembleOnSurfaceElement() and tells the
     * assembler what to add to the element stiffness matrix arising
     * from surface element contributions.
     *
     * @param rSurfaceElement the element which is being considered.
     * @param rPhi The basis functions, rPhi(i) = phi_i, i=1..numBases
     * @param rX The point in space
     */
    c_vector<double, ELEMENT_DIM> ComputeVectorSurfaceTerm(
                const BoundaryElement<ELEMENT_DIM-1,SPACE_DIM>& rSurfaceElement,
                c_vector<double, ELEMENT_DIM>& rPhi,
                ChastePoint<SPACE_DIM>& rX);

    
    /**
     * Overridden ResetInterpolatedQuantities() method.
     */
    void ResetInterpolatedQuantities( void )
    {
        mIionic = 0;
        mIIntracellularStimulus = 0;
    }
    
    /**
     * Overridden IncrementInterpolatedQuantities() method.
     *
     * @param phiI
     * @param pNode
     */
    void IncrementInterpolatedQuantities(double phiI, const Node<SPACE_DIM>* pNode);

public:

    /**
     * Constructor
     *
     * @param pMesh pointer to the mesh
     * @param pPde pointer to the PDE
     * @param dt timestep
     * @param numQuadPoints number of quadrature points (defaults to 2)
     */
    MonodomainAssembler(AbstractTetrahedralMesh<ELEMENT_DIM,SPACE_DIM>* pMesh,
                        MonodomainCellCollection<ELEMENT_DIM,SPACE_DIM>* pPde,
                        double dt,
                        unsigned numQuadPoints = 2);
};

#endif /*MONODOMAINASSEMBLER_HPP_*/
