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
#include "FixedDurationGenerationBasedCellCycleModel.hpp"

FixedDurationGenerationBasedCellCycleModel::FixedDurationGenerationBasedCellCycleModel()
{
}

AbstractCellCycleModel* FixedDurationGenerationBasedCellCycleModel::CreateCellCycleModel()
{
    // Create a new cell cycle model
    FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();

    // Set the values of the new cell cycle model's member variables
    p_model->SetGeneration(mGeneration);
    p_model->SetMaxTransitGenerations(mMaxTransitGenerations);
    p_model->SetCellProliferativeType(mCellProliferativeType);

    return p_model;
}

void FixedDurationGenerationBasedCellCycleModel::OutputCellCycleModelParameters(out_stream& rParamsFile)
{
    // No new parameters to output.

    // Call direct parent class
    AbstractSimpleGenerationBasedCellCycleModel::OutputCellCycleModelParameters(rParamsFile);
}

// Serialization for Boost >= 1.36
#include "SerializationExportWrapperForCpp.hpp"
CHASTE_CLASS_EXPORT(FixedDurationGenerationBasedCellCycleModel)
