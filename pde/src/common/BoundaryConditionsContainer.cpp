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

#include "BoundaryConditionsContainer.hpp"
#include "DistributedVector.hpp"
#include "ConstBoundaryCondition.hpp"

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::BoundaryConditionsContainer()
            : AbstractBoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>()
{
    for (unsigned index_of_unknown=0; index_of_unknown<PROBLEM_DIM; index_of_unknown++)
    {
        mpNeumannMap[index_of_unknown] = new std::map< const BoundaryElement<ELEM_DIM-1, SPACE_DIM> *, const AbstractBoundaryCondition<SPACE_DIM>*>;

        mAnyNonZeroNeumannConditionsForUnknown[index_of_unknown] = false;
        mLastNeumannCondition[index_of_unknown] = mpNeumannMap[index_of_unknown]->begin();
    }
}

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::~BoundaryConditionsContainer()
{
    // Keep track of what boundary condition objects we've deleted
    std::set<const AbstractBoundaryCondition<SPACE_DIM>*> deleted_conditions;
    for (unsigned i=0; i<PROBLEM_DIM; i++)
    {
        NeumannMapIterator neumann_iterator = mpNeumannMap[i]->begin();
        while (neumann_iterator != mpNeumannMap[i]->end() )
        {
            if (deleted_conditions.count(neumann_iterator->second) == 0)
            {
                deleted_conditions.insert(neumann_iterator->second);
                delete neumann_iterator->second;
            }
            neumann_iterator++;
        }

        delete(mpNeumannMap[i]);
    }

    this->DeleteDirichletBoundaryConditions(deleted_conditions);
}

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
void BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::AddDirichletBoundaryCondition( const Node<SPACE_DIM> *  pBoundaryNode,
                                        const AbstractBoundaryCondition<SPACE_DIM> * pBoundaryCondition,
                                        unsigned indexOfUnknown,
                                        bool checkIfBoundaryNode)
{
    assert(indexOfUnknown < PROBLEM_DIM);
    if (checkIfBoundaryNode)
    {
        assert( pBoundaryNode->IsBoundaryNode());
    }

    (*(this->mpDirichletMap[indexOfUnknown]))[pBoundaryNode] = pBoundaryCondition;
}

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
void BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::AddNeumannBoundaryCondition( const BoundaryElement<ELEM_DIM-1, SPACE_DIM> * pBoundaryElement,
                                      const AbstractBoundaryCondition<SPACE_DIM> * pBoundaryCondition,
                                      unsigned indexOfUnknown)
{
    assert(indexOfUnknown < PROBLEM_DIM);

    // we assume that this could be a non-zero boundary condition
    mAnyNonZeroNeumannConditionsForUnknown[indexOfUnknown] = true;

    (*(mpNeumannMap[indexOfUnknown]))[pBoundaryElement] = pBoundaryCondition;
}

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
void BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::DefineZeroDirichletOnMeshBoundary(AbstractMesh<ELEM_DIM,SPACE_DIM>* pMesh,
                                           unsigned indexOfUnknown)
{
    this->DefineConstantDirichletOnMeshBoundary(pMesh, 0.0, indexOfUnknown);
}

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
void BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::DefineConstantDirichletOnMeshBoundary(AbstractMesh<ELEM_DIM,SPACE_DIM>* pMesh,
                                               double value,
                                               unsigned indexOfUnknown)
{
    assert(indexOfUnknown < PROBLEM_DIM);
    //In applying a condition to the boundary, we need to be sure that the boundary exists
    assert(pMesh->GetNumBoundaryNodes() > 0);

    ConstBoundaryCondition<SPACE_DIM>* p_boundary_condition =
        new ConstBoundaryCondition<SPACE_DIM>( value );

    typename AbstractMesh<ELEM_DIM, SPACE_DIM>::BoundaryNodeIterator iter;
    iter = pMesh->GetBoundaryNodeIteratorBegin();
    while (iter != pMesh->GetBoundaryNodeIteratorEnd())
    {
        AddDirichletBoundaryCondition(*iter, p_boundary_condition, indexOfUnknown);
        iter++;
    }
}

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
void BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::DefineZeroNeumannOnMeshBoundary(AbstractMesh<ELEM_DIM,SPACE_DIM>* pMesh,
                                         unsigned indexOfUnknown)
{
    assert(indexOfUnknown < PROBLEM_DIM);
    //In applying a condition to the boundary, we need to be sure that the boundary exists
    assert(pMesh->GetNumBoundaryElements() > 0);
    ConstBoundaryCondition<SPACE_DIM>* p_zero_boundary_condition =
        new ConstBoundaryCondition<SPACE_DIM>( 0.0 );

    typename AbstractMesh<ELEM_DIM, SPACE_DIM>::BoundaryElementIterator iter;
    iter = pMesh->GetBoundaryElementIteratorBegin();
    while (iter != pMesh->GetBoundaryElementIteratorEnd())
    {
        AddNeumannBoundaryCondition(*iter, p_zero_boundary_condition, indexOfUnknown);
        iter++;
    }

    mAnyNonZeroNeumannConditionsForUnknown[indexOfUnknown] = false;
}

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
void BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::ApplyDirichletToLinearProblem(LinearSystem& rLinearSystem,
                                       bool applyToMatrix)
{
    for (unsigned index_of_unknown=0; index_of_unknown<PROBLEM_DIM; index_of_unknown++)
    {
        this->mDirichIterator = this->mpDirichletMap[index_of_unknown]->begin();

        while (this->mDirichIterator != this->mpDirichletMap[index_of_unknown]->end() )
        {
            unsigned node_index = this->mDirichIterator->first->GetIndex();
            double value = this->mDirichIterator->second->GetValue(this->mDirichIterator->first->GetPoint());

            unsigned row = PROBLEM_DIM*node_index + index_of_unknown;

            //old version equivalent to:
            //unsigned row = node_index+index_of_unknown*mNumNodes;

            if (applyToMatrix)
            {
                rLinearSystem.ZeroMatrixRow(row);
                rLinearSystem.SetMatrixElement(row, row, 1);
            }
            rLinearSystem.SetRhsVectorElement(row, value);

            this->mDirichIterator++;
        }
    }
}

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
void BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::ApplyDirichletToNonlinearResidual(const Vec currentSolution, Vec residual)
{
    for (unsigned index_of_unknown=0; index_of_unknown<PROBLEM_DIM; index_of_unknown++)
    {
        this->mDirichIterator = this->mpDirichletMap[index_of_unknown]->begin();

        DistributedVector solution_distributed(currentSolution);
        DistributedVector residual_distributed(residual);


        while (this->mDirichIterator != this->mpDirichletMap[index_of_unknown]->end() )
        {
            DistributedVector::Stripe solution_stripe(solution_distributed, index_of_unknown);
            DistributedVector::Stripe residual_stripe(residual_distributed, index_of_unknown);

            unsigned node_index = this->mDirichIterator->first->GetIndex();

            double value = this->mDirichIterator->second->GetValue(this->mDirichIterator->first->GetPoint());

            if (DistributedVector::IsGlobalIndexLocal(node_index))
            {
                residual_stripe[node_index]=solution_stripe[node_index] - value;
            }
            this->mDirichIterator++;
        }
        solution_distributed.Restore();
        residual_distributed.Restore();
    }
}
    
template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>    
void BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::ApplyDirichletToNonlinearJacobian(Mat jacobian)
{
    for (unsigned index_of_unknown=0; index_of_unknown<PROBLEM_DIM; index_of_unknown++)
    {
        this->mDirichIterator = this->mpDirichletMap[index_of_unknown]->begin();
        PetscInt irows, icols;
        double value;
        MatGetSize(jacobian, &irows, &icols);
        unsigned cols=icols;

        while (this->mDirichIterator != this->mpDirichletMap[index_of_unknown]->end() )
        {
            unsigned node_index = this->mDirichIterator->first->GetIndex();

            unsigned row_index = PROBLEM_DIM*node_index + index_of_unknown;
            assert(row_index<(unsigned)irows);

            for (unsigned col_index=0; col_index<cols; col_index++)
            {
                value = (col_index == row_index) ? 1.0 : 0.0;
                MatSetValue(jacobian, row_index, col_index, value, INSERT_VALUES);
            }
            this->mDirichIterator++;
        }
    }
}    

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
bool BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::Validate(AbstractMesh<ELEM_DIM,SPACE_DIM> *pMesh)
{
    bool valid = true;

    for (unsigned index_of_unknown=0; index_of_unknown<PROBLEM_DIM; index_of_unknown++)
    {
        // Iterate over surface elements
        typename AbstractMesh<ELEM_DIM,SPACE_DIM>::BoundaryElementIterator elt_iter
        = pMesh->GetBoundaryElementIteratorBegin();
        while (valid && elt_iter != pMesh->GetBoundaryElementIteratorEnd())
        {
            if (!HasNeumannBoundaryCondition(*elt_iter, index_of_unknown))
            {
                // Check for Dirichlet conditions on this element's nodes
                for (unsigned i=0; i<(*elt_iter)->GetNumNodes(); i++)
                {
                    if (!HasDirichletBoundaryCondition((*elt_iter)->GetNode(i)))
                    {
                        valid = false;
                    }
                }
            }
            elt_iter++;
        }
    }
    return valid;
}

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
double BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::GetNeumannBCValue(const BoundaryElement<ELEM_DIM-1,SPACE_DIM>* pSurfaceElement,
                             const ChastePoint<SPACE_DIM>& x,
                             unsigned indexOfUnknown)
{
    assert(indexOfUnknown < PROBLEM_DIM);

    // Did we see this condition on the last search we did?
    if (mLastNeumannCondition[indexOfUnknown] == mpNeumannMap[indexOfUnknown]->end() ||
        mLastNeumannCondition[indexOfUnknown]->first != pSurfaceElement)
    {
        mLastNeumannCondition[indexOfUnknown] = mpNeumannMap[indexOfUnknown]->find(pSurfaceElement);
    }
    if (mLastNeumannCondition[indexOfUnknown] == mpNeumannMap[indexOfUnknown]->end())
    {
        // No Neumann condition is equivalent to a zero Neumann condition
        return 0.0;
    }
    else
    {
        return mLastNeumannCondition[indexOfUnknown]->second->GetValue(x);
    }
}

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
bool BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::HasNeumannBoundaryCondition(const BoundaryElement<ELEM_DIM-1,SPACE_DIM>* pSurfaceElement,
                                     unsigned indexOfUnknown)
{
    assert(indexOfUnknown < PROBLEM_DIM);

    mLastNeumannCondition[indexOfUnknown] = mpNeumannMap[indexOfUnknown]->find(pSurfaceElement);

    return (mLastNeumannCondition[indexOfUnknown] != mpNeumannMap[indexOfUnknown]->end());
}

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
bool BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::AnyNonZeroNeumannConditions()
{
    bool ret = false;
    for (unsigned index_of_unknown=0; index_of_unknown<PROBLEM_DIM; index_of_unknown++)
    {
        if (mAnyNonZeroNeumannConditionsForUnknown[index_of_unknown] == true)
        {
            ret = true;
        }
    }
    return ret;
}

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
typename BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::NeumannMapIterator BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::BeginNeumann()
{
    return mpNeumannMap[0]->begin();
}

template<unsigned ELEM_DIM, unsigned SPACE_DIM, unsigned PROBLEM_DIM>
typename BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::NeumannMapIterator BoundaryConditionsContainer<ELEM_DIM,SPACE_DIM,PROBLEM_DIM>::EndNeumann()
{
    return mpNeumannMap[0]->end();
}


////////////////////////////////////////////////////////////////////////////////////
// Explicit instantiation
////////////////////////////////////////////////////////////////////////////////////

template class BoundaryConditionsContainer<1,1,1>;
template class BoundaryConditionsContainer<2,2,1>;
template class BoundaryConditionsContainer<3,3,1>;

template class BoundaryConditionsContainer<1,1,2>;
template class BoundaryConditionsContainer<2,2,2>;
template class BoundaryConditionsContainer<3,3,2>;

template class BoundaryConditionsContainer<3,3,3>;
