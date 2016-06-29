/*

Copyright (c) 2005-2016, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "AbstractNumericalMethod.hpp"
#include "AbstractMesh.hpp"
#include "StepSizeException.hpp"
#include "Warnings.hpp"
#include "AbstractCentreBasedCellPopulation.hpp"
#include "NodeBasedCellPopulationWithBuskeUpdate.hpp"
#include "MeshBasedCellPopulationWithGhostNodes.hpp"
#include "CellBasedEventHandler.hpp"

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
AbstractNumericalMethod<ELEMENT_DIM,SPACE_DIM>::AbstractNumericalMethod()
    : mpCellPopulation(NULL),
      mpForceCollection(NULL),
      mpIsAdaptiveTimestep(NULL)
{
    /*
     * mpCellPopulation and mpForceCollection are initialized by OffLatticeSimulation in
     * its constructor. For now we set some safe defaults for the other member variables.
     */

    mUseUpdateNodeLocation = false;
    mGhostNodeForcesEnabled = true;
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
AbstractNumericalMethod<ELEMENT_DIM,SPACE_DIM>::~AbstractNumericalMethod()
{
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void AbstractNumericalMethod<ELEMENT_DIM,SPACE_DIM>::SetCellPopulation(AbstractOffLatticeCellPopulation<ELEMENT_DIM,SPACE_DIM>* pPopulation)
{
    if (!mpCellPopulation)
    {
        mpCellPopulation = pPopulation;
    }
    else
    {
        EXCEPTION("The cell population referred to by a numerical method should not be reset");
    }    

    // Set other member variables according to the type of the cell population
    if (dynamic_cast<NodeBasedCellPopulationWithBuskeUpdate<SPACE_DIM>*>(mpCellPopulation))
    {
        mUseUpdateNodeLocation = true;
        WARNING("Non-Euler steppers are not yet implemented for NodeBasedCellPopulationWithBuskeUpdate");
    }
    else
    {
        mUseUpdateNodeLocation = false;
    }

    if (dynamic_cast<MeshBasedCellPopulationWithGhostNodes<SPACE_DIM>*>(mpCellPopulation))
    {
        mGhostNodeForcesEnabled = true;
    }
    else
    {
        mGhostNodeForcesEnabled = false;
    }
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void AbstractNumericalMethod<ELEMENT_DIM,SPACE_DIM>::SetForceCollection(std::vector<boost::shared_ptr<AbstractForce<ELEMENT_DIM, SPACE_DIM> > >* pForces)
{
    if (!mpForceCollection)
    {
        mpForceCollection = pForces;
    }
    else
    {
        EXCEPTION("The force collection referred to by a numerical method should not be reset");
    }    
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void AbstractNumericalMethod<ELEMENT_DIM,SPACE_DIM>::SetIsAdaptiveTimestep(bool* pIsAdaptiveTimestep)
{
    if (mpIsAdaptiveTimestep == NULL)
    {
        mpIsAdaptiveTimestep = pIsAdaptiveTimestep;
    }
    else
    {
        EXCEPTION("The numerical method function mpIsAdaptiveTimestep should only be called once by OffLatticeSimulation in its constructor");
    }
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
std::vector<c_vector<double, SPACE_DIM> > AbstractNumericalMethod<ELEMENT_DIM,SPACE_DIM>::ComputeForcesIncludingDamping()
{
    CellBasedEventHandler::BeginEvent(CellBasedEventHandler::FORCE);

    for (typename AbstractMesh<ELEMENT_DIM, SPACE_DIM>::NodeIterator node_iter = mpCellPopulation->rGetMesh().GetNodeIteratorBegin();
         node_iter != mpCellPopulation->rGetMesh().GetNodeIteratorEnd(); ++node_iter)
    {
        node_iter->ClearAppliedForce();
    }
    
    for (typename std::vector<boost::shared_ptr<AbstractForce<ELEMENT_DIM, SPACE_DIM> > >::iterator iter = mpForceCollection->begin();
        iter != mpForceCollection->end(); ++iter)
    {
        (*iter)->AddForceContribution(*mpCellPopulation);
    }

    /**
     * Here we deal with the special case forces on ghost nodes. Note that 'particles'
     * are dealt with like normal cells.
     *
     * \todo #2087 Consider removing dynamic_cast and instead adding a virtual method ApplyForcesToNonCellNodes()
     */
    if (mGhostNodeForcesEnabled)
    {
        dynamic_cast<MeshBasedCellPopulationWithGhostNodes<SPACE_DIM>*>(mpCellPopulation)->ApplyGhostForces();
    }

    // Store applied forces in a vector
    std::vector<c_vector<double, SPACE_DIM> > forces_as_vector;
    forces_as_vector.reserve(mpCellPopulation->GetNumNodes());

    for (typename AbstractMesh<ELEMENT_DIM, SPACE_DIM>::NodeIterator node_iter = mpCellPopulation->rGetMesh().GetNodeIteratorBegin();
         node_iter != mpCellPopulation->rGetMesh().GetNodeIteratorEnd(); ++node_iter)
    {
        double damping = mpCellPopulation->GetDampingConstant(node_iter->GetIndex());
        forces_as_vector.push_back(node_iter->rGetAppliedForce()/damping);
    }
    
    CellBasedEventHandler::EndEvent(CellBasedEventHandler::FORCE);

    return forces_as_vector;
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
std::vector<c_vector<double, SPACE_DIM> > AbstractNumericalMethod<ELEMENT_DIM,SPACE_DIM>::SaveCurrentLocations()
{
    std::vector<c_vector<double, SPACE_DIM> > current_locations;
    current_locations.reserve(mpCellPopulation->GetNumNodes());

    unsigned index = 0;
    for (typename AbstractMesh<ELEMENT_DIM, SPACE_DIM>::NodeIterator node_iter = mpCellPopulation->rGetMesh().GetNodeIteratorBegin();
         node_iter != mpCellPopulation->rGetMesh().GetNodeIteratorEnd();
         ++index, ++node_iter)
    {
        current_locations[index] = node_iter->rGetLocation();
    }

    return current_locations;
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>  
void AbstractNumericalMethod<ELEMENT_DIM,SPACE_DIM>::SafeNodePositionUpdate( unsigned nodeIndex, c_vector<double, SPACE_DIM> newPosition)
{
    ChastePoint<SPACE_DIM> new_point(newPosition);
    mpCellPopulation->SetNode(nodeIndex, new_point);
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void AbstractNumericalMethod<ELEMENT_DIM,SPACE_DIM>::DetectStepSizeExceptions(unsigned nodeIndex, c_vector<double,SPACE_DIM>& displacement, double dt)
{    
    try
    {
        mpCellPopulation->CheckForStepSizeException(nodeIndex, displacement, dt);
    }
    catch (StepSizeException* e)
    {
        if (!(e->mIsTerminal) && (*mpIsAdaptiveTimestep==false))
        {
            // If adaptivity is turned off but the simulation can continue, just produce a warning.
            // Only the case for vertex based populations, which can alter node displacement directly 
            // to avoid cell rearrangement problems.
            WARN_ONCE_ONLY(e->what());
        }
        else
        {
            throw e;
        }
    }   
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void AbstractNumericalMethod<ELEMENT_DIM,SPACE_DIM>::OutputNumericalMethodInfo(out_stream& rParamsFile)
{
    std::string numerical_method_type = GetIdentifier();

    *rParamsFile << "\t\t<" << numerical_method_type << ">\n";
    OutputNumericalMethodParameters(rParamsFile);
    *rParamsFile << "\t\t</" << numerical_method_type << ">\n";
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void AbstractNumericalMethod<ELEMENT_DIM,SPACE_DIM>::OutputNumericalMethodParameters(out_stream& rParamsFile)
{
    // No parameters to return here
}

// Explicit instantiation
template class AbstractNumericalMethod<1,1>;
template class AbstractNumericalMethod<1,2>;
template class AbstractNumericalMethod<2,2>;
template class AbstractNumericalMethod<1,3>;
template class AbstractNumericalMethod<2,3>;
template class AbstractNumericalMethod<3,3>;
