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
#ifndef CHEMOTACTICFORCE_HPP_
#define CHEMOTACTICFORCE_HPP_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "AbstractForce.hpp"
#include "CellwiseDataGradient.hpp"


template<unsigned DIM>
class ChemotacticForce  : public AbstractForce<DIM>
{
friend class TestForces;

private:

    double GetChemotacticForceMagnitude(const double concentration, const double concentrationGradientMagnitude);

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
        // If Archive is an output archive, then '&' resolves to '<<'
        // If Archive is an input archive, then '&' resolves to '>>'
        archive & boost::serialization::base_object<AbstractForce<DIM> >(*this);
    }
    
    /** Whether to use spring constant proportional to cell-cell contact length/area (defaults to false) */
    bool mUseEdgeBasedSpringConstant;

    /** Whether to use different stiffnesses depending on whether either cell is a mutant */
    bool mUseMutantSprings;

    /** Multiplier for spring stiffness if mutant */
    double mMutantMutantMultiplier;

    /** Multiplier for spring stiffness if mutant */
    double mNormalMutantMultiplier;

    /** Use springs which are dependent on beta-catenin levels */
    bool mUseBCatSprings;

    /** Use springs which are dependent on whether cells are necrotic */
    bool mUseApoptoticSprings;

public:

    ChemotacticForce();
    
    ~ChemotacticForce();

    /**
     * Calculates the forces on each node
     *
     * @return the velocity components on each node. Of size NUM_NODES x DIM.
     * The velocities are those that would be returned by the Meineke2001SpringSystem,
     * with a velocity due to the force by chemotaxis added on.
     *
     * Fc = chi(C,|gradC|) gradC/|gradC|  (if |gradC|>0, else Fc = 0)
     *
     */
    /// \todo eventually this should be a force contribution (see #627)
    void AddVelocityContribution(std::vector<c_vector<double, DIM> >& rNodeVelocities,
                                 AbstractTissue<DIM>& rTissue);
                                         
    /**
     * Use an area based viscosity
     */
    void SetAreaBasedViscosity(bool useAreaBasedViscosity); 

};

template<unsigned DIM>
ChemotacticForce<DIM>::ChemotacticForce()
   : AbstractForce<DIM>()
{
}

template<unsigned DIM>
ChemotacticForce<DIM>::~ChemotacticForce()
{
}

template<unsigned DIM>
void ChemotacticForce<DIM>::SetAreaBasedViscosity(bool useAreaBasedViscosity)
{
    assert(DIM == 2);
    this->mUseAreaBasedViscosity = useAreaBasedViscosity;
}

template<unsigned DIM>
double ChemotacticForce<DIM>::GetChemotacticForceMagnitude(const double concentration, const double concentrationGradientMagnitude)
{
    return concentration; // temporary force law - can be changed to something realistic
                          // without tests failing
}

template<unsigned DIM>
void ChemotacticForce<DIM>::AddVelocityContribution(std::vector<c_vector<double, DIM> >& rNodeVelocities,
                                                    AbstractTissue<DIM>& rTissue)
{
    CellwiseDataGradient<DIM> gradients;
    gradients.SetupGradients();

    for (typename AbstractTissue<DIM>::Iterator cell_iter = rTissue.Begin();
         cell_iter != rTissue.End();
         ++cell_iter)
    {
        // Only LABELLED cells move chemotactically
        if (cell_iter->GetMutationState() == LABELLED)
        {
            TissueCell& cell = *cell_iter;
            unsigned node_global_index = cell.GetLocationIndex();

            c_vector<double,DIM>& r_gradient = gradients.rGetGradient(cell.GetLocationIndex());
            double nutrient_concentration = CellwiseData<DIM>::Instance()->GetValue(&cell,0);
            double magnitude_of_gradient = norm_2(r_gradient);

            double force_magnitude = GetChemotacticForceMagnitude(nutrient_concentration, magnitude_of_gradient);

            double damping_constant = this->GetDampingConstant(cell, rTissue);

            // velocity += viscosity * chi * gradC/|gradC|
            if (magnitude_of_gradient > 0)
            {
                rNodeVelocities[node_global_index] += (force_magnitude/(damping_constant*magnitude_of_gradient))*r_gradient;
            }
            // else Fc=0
        }
    }
}

#endif /*CHEMOTACTICFORCE_HPP_*/
