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

#ifndef CARDIACELECTROMECHPROBREGULARGEOM_HPP_
#define CARDIACELECTROMECHPROBREGULARGEOM_HPP_

#include "CardiacElectroMechanicsProblem.hpp"

/**
 *  Child class of CardiacElectroMechanicsProblem for setting up cardiac electromechanics
 *  problems on a square (currently just 2d). The user just has to specify the number
 *  of elements in each direction. Note: the x=0 surface is fixed in the deformation.
 */
template<unsigned DIM>
class CardiacElectroMechProbRegularGeom : public CardiacElectroMechanicsProblem<DIM>
{
public:
    /**
     *  Constructor
     *  @param width Width and height of the square.
     *  @param numMechanicsElementsEachDir Num elements in each direction in the mechanics mesh.
     *  @param numElectricsElementsEachDir Num elements in each direction in the electrics mesh
     */
    CardiacElectroMechProbRegularGeom(double width,
                                      unsigned numMechanicsElementsEachDir,
                                      unsigned numElectricsElementsEachDir,
                                      AbstractCardiacCellFactory<DIM>* pCellFactory,
                                      double endTime,
                                      unsigned numElecTimeStepsPerMechTimestep,
                                      double nhsOdeTimeStep,
                                      std::string outputDirectory = "")
        : CardiacElectroMechanicsProblem<DIM>(NULL, NULL, std::vector<unsigned>(), // all these set below
                                              pCellFactory, endTime,
                                              numElecTimeStepsPerMechTimestep,
                                              nhsOdeTimeStep, outputDirectory)
    {
        assert(DIM==2); // the below assumes DIM==2

        assert(width > 0.0);
        assert(numMechanicsElementsEachDir > 0);
        assert(numElectricsElementsEachDir > 0);

        // create electrics mesh
        this->mpElectricsMesh = new TetrahedralMesh<DIM,DIM>();

        this->mpElectricsMesh->ConstructRectangularMesh(numElectricsElementsEachDir,numElectricsElementsEachDir);
        this->mpElectricsMesh->Scale(width/numElectricsElementsEachDir,width/numElectricsElementsEachDir);

        // create mechanics mesh
        this->mpMechanicsMesh = new QuadraticMesh<DIM>(width,width,numMechanicsElementsEachDir,numMechanicsElementsEachDir);
        LOG(2, "Width of meshes is " << width);
        LOG(2, "Num nodes in electrical and mechanical meshes are: " << this->mpElectricsMesh->GetNumNodes() << ", " << this->mpMechanicsMesh->GetNumNodes() << "\n");

        // fix the nodes on x=0
        this->mFixedNodes.clear();
        for(unsigned i=0; i<this->mpMechanicsMesh->GetNumNodes(); i++)
        {
            if( fabs(this->mpMechanicsMesh->GetNode(i)->rGetLocation()[0])<1e-6)
            {
                this->mFixedNodes.push_back(i);
            }
        }

        LOG(2, "Fixed the " << this->mFixedNodes.size() << " nodes on x=0");
    }

    ~CardiacElectroMechProbRegularGeom()
    {
        delete this->mpElectricsMesh;
        delete this->mpMechanicsMesh;
    }
};

#endif /*CARDIACELECTROMECHPROBREGULARGEOM_HPP_*/
