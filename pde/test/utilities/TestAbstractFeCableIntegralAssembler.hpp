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
#ifndef TESTABSTRACTFECABLEINTEGRALASSEMBLER_HPP_
#define TESTABSTRACTFECABLEINTEGRALASSEMBLER_HPP_


#include <cxxtest/TestSuite.h>

#include "AbstractFeCableIntegralAssembler.hpp"
#include "MixedDimensionMesh.hpp"
#include "PetscMatTools.hpp"
#include "PetscSetupAndFinalize.hpp"
#include "PetscVecTools.hpp"
#include "ReplicatableVector.hpp"
#include "TrianglesMeshReader.hpp"

template<unsigned DIM>
class BasicCableAssembler : public AbstractFeCableIntegralAssembler<DIM,DIM,1,true,true,NORMAL>
{
private:
    double mCoefficient;

    c_vector<double,1*2> ComputeCableVectorTerm(
        c_vector<double, 2>& rPhi,
        c_matrix<double, DIM, 2>& rGradPhi,
        ChastePoint<DIM>& rX,
        c_vector<double,1>& rU,
        c_matrix<double, 1, DIM>& rGradU,
        Element<1,DIM>* pElement)
    {
        return -mCoefficient*rPhi;
    }

    c_matrix<double,1*2,1*2> ComputeCableMatrixTerm(
        c_vector<double, 2>& rPhi,
        c_matrix<double, DIM, 2>& rGradPhi,
        ChastePoint<DIM>& rX,
        c_vector<double,1>& rU,
        c_matrix<double, 1, DIM>& rGradU,
        Element<1,DIM>* pElement)
    {
        c_matrix<double, 2, 2> mass_matrix = outer_prod(rPhi, rPhi);
        return mCoefficient*mass_matrix;
    }

public:
    BasicCableAssembler(MixedDimensionMesh<DIM,DIM>* pMesh, double coefficient)
        : AbstractFeCableIntegralAssembler<DIM,DIM,1,true,true,NORMAL>(pMesh),
          mCoefficient(coefficient)
    {
    }
};

template<unsigned DIM>
class SimpleVectorAssembler : public AbstractFeCableIntegralAssembler<DIM,DIM,1,true,false,NORMAL>
{
private:
    c_vector<double,1*2> ComputeCableVectorTerm(
        c_vector<double, 2>& rPhi,
        c_matrix<double, DIM, 2>& rGradPhi,
        ChastePoint<DIM>& rX,
        c_vector<double,1>& rU,
        c_matrix<double, 1, DIM>& rGradU,
        Element<1,DIM>* pElement)
    {
        assert(fabs(rU(0)-10.0)<1e-6); // check u has been passed through correctly as 10 - see TestExceptionsAndSetCurrentSolution() below
        return zero_vector<double>(2);
    }
public:
    SimpleVectorAssembler(MixedDimensionMesh<DIM,DIM>* pMesh)
        : AbstractFeCableIntegralAssembler<DIM,DIM,1,true,false,NORMAL>(pMesh)
    {
    }
};

template<unsigned DIM>
class SimpleMatrixAssembler : public AbstractFeCableIntegralAssembler<DIM,DIM,1,false,true,NORMAL>
{
private:
    c_matrix<double,1*2,1*2> ComputeCableMatrixTerm(
        c_vector<double, 2>& rPhi,
        c_matrix<double, DIM, 2>& rGradPhi,
        ChastePoint<DIM>& rX,
        c_vector<double,1>& rU,
        c_matrix<double, 1, DIM>& rGradU,
        Element<1,DIM>* pElement)
    {
        return zero_matrix<double>(2,2);
    }
public:
    SimpleMatrixAssembler(MixedDimensionMesh<DIM,DIM>* pMesh)
        : AbstractFeCableIntegralAssembler<DIM,DIM,1,false,true,NORMAL>(pMesh)
    {
    }
};


class TestAbstractFeCableIntegralAssembler : public CxxTest::TestSuite
{
public:
    void TestBasicCableAssemblers() throw(Exception)
    {
        std::string mesh_base("mesh/test/data/mixed_dimension_meshes/2D_0_to_1mm_200_elements");
        TrianglesMeshReader<2,2> reader(mesh_base);
        MixedDimensionMesh<2,2> mesh(DistributedTetrahedralMeshPartitionType::DUMB);
        mesh.ConstructFromMeshReader(reader);

        double h = 0.01; //All cable elements in the mesh are of this length

        Vec vec = PetscTools::CreateVec(mesh.GetNumNodes());

        Mat mat;
        PetscTools::SetupMat(mat, mesh.GetNumNodes(), mesh.GetNumNodes(), 2);

        double coefficient = 2.0;
        BasicCableAssembler<2> basic_cable_assembler(&mesh, coefficient);

        basic_cable_assembler.SetMatrixToAssemble(mat);
        basic_cable_assembler.SetVectorToAssemble(vec,true);
        basic_cable_assembler.Assemble();

        PetscMatTools::Finalise(mat);
        PetscVecTools::Finalise(vec);

        /*
         * Cables:
       index   node0   node1
        0       55      56      1
        1       56      57      2
        2       57      58      3
        3       58      59      4
        4       59      60      5
        5       60      61      6
        6       61      62      7
        7       62      63      8
        8       63      64      9
        9       64      65      10
         *
         */

        //Test vector assembly
        ReplicatableVector vec_repl(vec);
        for (unsigned i = 0; i < 55; ++i)
        {
            TS_ASSERT_DELTA(vec_repl[i], 0.0, 1e-4);
        }
        TS_ASSERT_DELTA(vec_repl[55], -h*coefficient*0.5, 1e-4);
        for (unsigned i=56; i<65; i++)
        {
            TS_ASSERT_DELTA(vec_repl[i], -h*coefficient, 1e-4);
        }
        TS_ASSERT_DELTA(vec_repl[65], -h*coefficient*0.5, 1e-4);
        for (unsigned i = 66; i < mesh.GetNumNodes(); ++i)
        {
            TS_ASSERT_DELTA(vec_repl[i], 0.0, 1e-4);
        }

        VecDestroy(vec);

        //Test matrix assembly
        int lo, hi;
        MatGetOwnershipRange(mat, &lo, &hi);
        for (unsigned i=lo; i<(unsigned)hi; i++)
        {
            //Central cable nodes
            if( i > 55 && i < 65)
            {
                double value = PetscMatTools::GetElement(mat,i,i);
                TS_ASSERT_DELTA(value, (2.0/3.0)*h*coefficient, 1e-4);

                value = PetscMatTools::GetElement(mat,i,i-1);
                TS_ASSERT_DELTA(value, (1.0/6.0)*h*coefficient, 1e-4);

                value = PetscMatTools::GetElement(mat,i,i+1);
                TS_ASSERT_DELTA(value, (1.0/6.0)*h*coefficient, 1e-4);
            }
            else if( i == 55)
            {
                double value = PetscMatTools::GetElement(mat,i,i);
                TS_ASSERT_DELTA(value, (1.0/3.0)*h*coefficient, 1e-4);

                value = PetscMatTools::GetElement(mat,i,i+1);
                TS_ASSERT_DELTA(value, (1.0/6.0)*h*coefficient, 1e-4);
            }
            else if( i == 65 )
            {
                double value = PetscMatTools::GetElement(mat,i,i);
                TS_ASSERT_DELTA(value, (1.0/3.0)*h*coefficient, 1e-4);

                value = PetscMatTools::GetElement(mat,i,i-1);
                TS_ASSERT_DELTA(value, (1.0/6.0)*h*coefficient, 1e-4);
            }
            else
            {
                double value = PetscMatTools::GetElement(mat,i,i);
                TS_ASSERT_DELTA(value, 0.0, 1e-4);
            }
        }

        MatDestroy(mat);
    }


    void TestExceptionsAndSetCurrentSolution() throw(Exception)
    {
        std::string mesh_base("mesh/test/data/mixed_dimension_meshes/2D_0_to_1mm_200_elements");
        TrianglesMeshReader<2,2> reader(mesh_base);
        MixedDimensionMesh<2,2> mesh;
        mesh.ConstructFromMeshReader(reader);

        SimpleMatrixAssembler<2> matrix_assembler(&mesh);
        TS_ASSERT_THROWS_THIS(matrix_assembler.Assemble(), "Matrix to be assembled has not been set");

        SimpleVectorAssembler<2> vector_assembler(&mesh);
        TS_ASSERT_THROWS_THIS(vector_assembler.Assemble(), "Vector to be assembled has not been set");

        // Now cover and test the code that sets the current solution before assembling.
        // We create a current solution of u=10 (for all nodes)...
        Vec current_soln = PetscTools::CreateAndSetVec(mesh.GetNumNodes(), 10.0);
        vector_assembler.SetCurrentSolution(current_soln);
        // .. and call Assemble(). Note that the concrete assembler (defined above) checks
        // that u=10 in ComputeCableVectorTerm().
        Vec vec = PetscTools::CreateVec(mesh.GetNumNodes());
        vector_assembler.SetVectorToAssemble(vec,true);
        vector_assembler.Assemble();

        VecDestroy(vec);
        VecDestroy(current_soln);
    }
};




#endif /*TESTABSTRACTFECABLEINTEGRALASSEMBLER_HPP_*/
