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

#ifndef TESTABSTRACTPURKINJECELLFACTORY_HPP_
#define TESTABSTRACTPURKINJECELLFACTORY_HPP_

#include <cxxtest/TestSuite.h>
#include "AbstractPurkinjeCellFactory.hpp"
#include "TrianglesMeshReader.hpp"
#include "MixedDimensionMesh.hpp"
#include "FakeBathCell.hpp"
#include "SimpleStimulus.hpp"
#include "LuoRudy1991.hpp"
#include "DiFrancescoNoble1985.hpp"
#include "PetscSetupAndFinalize.hpp"

class PurkinjeCellFactory : public AbstractPurkinjeCellFactory<2>
{
private:
    boost::shared_ptr<SimpleStimulus> mpStimulus;

public:
    PurkinjeCellFactory()
        : AbstractPurkinjeCellFactory<2>(),
          mpStimulus(new SimpleStimulus(-6000.0, 0.5))
    {
    }

    AbstractCardiacCell* CreateCardiacCellForTissueNode(unsigned node)
    {
        ChastePoint<2> location = GetMesh()->GetNode(node)->GetPoint();

        if (fabs(location[0])<1e-6)
        {
            return new CellLuoRudy1991FromCellML(mpSolver, mpStimulus);
        }
        else
        {
            return new CellLuoRudy1991FromCellML(mpSolver, mpZeroStimulus);
        }
    }

    AbstractCardiacCell* CreatePurkinjeCellForTissueNode(unsigned node)
    {
        return new CellDiFrancescoNoble1985FromCellML(mpSolver, mpZeroStimulus);
    }
};


class TestAbstractPurkinjeCellFactory : public CxxTest::TestSuite
{
public:
    void TestPurkinjeCellFactory() throw (Exception)
    {
        TrianglesMeshReader<2,2> reader("mesh/test/data/mixed_dimension_meshes/2D_0_to_1mm_200_elements");
        MixedDimensionMesh<2,2> mixed_mesh;
        mixed_mesh.ConstructFromMeshReader(reader);

        PurkinjeCellFactory cell_factory;
        cell_factory.SetMixedMesh(&mixed_mesh);

        AbstractCardiacCell* p_cell = cell_factory.CreateCardiacCellForNode(0);
        TS_ASSERT(dynamic_cast<CellLuoRudy1991FromCellML*>(p_cell) != NULL);
        delete p_cell;

        for(unsigned i=0; i<mixed_mesh.GetNumNodes(); i++)
        {
            AbstractCardiacCell* p_cell = cell_factory.CreatePurkinjeCellForNode(i);
            if(i>=56 && i<=65)
            {
            	TS_ASSERT(dynamic_cast<CellDiFrancescoNoble1985FromCellML*>(p_cell) != NULL);
            }
            else
            {
//            	TS_ASSERT(dynamic_cast<FakeBathCell*>(p_cell) != NULL);
            }
          	delete p_cell;
        }

        TS_ASSERT_EQUALS(cell_factory.GetMixedDimensionMesh(), &mixed_mesh);
    }
};

#endif // TESTABSTRACTPURKINJECELLFACTORY_HPP_
