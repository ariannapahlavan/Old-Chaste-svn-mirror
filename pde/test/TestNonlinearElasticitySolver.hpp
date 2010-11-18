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


#ifndef TESTNONLINEARELASTICITYSOLVER_HPP_
#define TESTNONLINEARELASTICITYSOLVER_HPP_

#include <cxxtest/TestSuite.h>
#include "UblasCustomFunctions.hpp"
#include "NonlinearElasticitySolver.hpp"
#include "PetscSetupAndFinalize.hpp"
#include "ExponentialMaterialLaw.hpp"
#include "MooneyRivlinMaterialLaw.hpp"
#include "NashHunterPoleZeroLaw.hpp"

double MATERIAL_PARAM = 1.0;
double ALPHA = 0.2;

// Body force corresponding to the deformation
// x = X+0.5*alpha*X^2, y=Y/(1+alpha*X), with p=2c
c_vector<double,2> MyBodyForce(c_vector<double,2>& X)
{
    assert(X(0)>=0 && X(0)<=1 && X(1)>=0 && X(1)<=1);

    c_vector<double,2> body_force;
    double lam = 1+ALPHA*X(0);
    body_force(0) = -2*MATERIAL_PARAM * ALPHA;
    body_force(1) = -2*MATERIAL_PARAM * 2*ALPHA*ALPHA*X(1)/(lam*lam*lam);
    return body_force;
}

// Surface traction on three sides of a cube, corresponding to
// x = X+0.5*alpha*X^2, y=Y/(1+alpha*X), with p=2c
c_vector<double,2> MyTraction(c_vector<double,2>& location)
{
    c_vector<double,2> traction = zero_vector<double>(2);

    double lam = 1+ALPHA*location(0);
    if (fabs(location(0)-1.0) <= DBL_EPSILON) //Right edge
    {
        traction(0) =  2*MATERIAL_PARAM * (lam - 1.0/lam);
        traction(1) = -2*MATERIAL_PARAM * location(1)*ALPHA/(lam*lam);
    }
    else if (fabs(location(1))  <= DBL_EPSILON) //Bottom edge
    {
        traction(0) =  2*MATERIAL_PARAM * location(1)*ALPHA/(lam*lam);
        traction(1) = -2*MATERIAL_PARAM * (-lam + 1.0/lam);
    }
    else if (fabs(location(1) - 1.0) <= DBL_EPSILON)//Top edge
    {
        traction(0) = -2*MATERIAL_PARAM * location(1)*ALPHA/(lam*lam);
        traction(1) =  2*MATERIAL_PARAM * (-lam + 1.0/lam);
    }
    else
    {
        NEVER_REACHED;
    }
    return traction;
}



class TestNonlinearElasticitySolver : public CxxTest::TestSuite
{
public:

    // Solve using quadratics..
    void TestAssembleSystem3D() throw (Exception)
    {
        //Currently this is purely for coverage of assembling a 3D system...
        QuadraticMesh<3> mesh;
        TrianglesMeshReader<3,3> mesh_reader1("mesh/test/data/3D_Single_tetrahedron_element_quadratic",2,1,false);

        mesh.ConstructFromMeshReader(mesh_reader1);

        ExponentialMaterialLaw<3> law(2.0, 3.0);
        std::vector<unsigned> fixed_nodes;
        fixed_nodes.push_back(0);

        NonlinearElasticitySolver<3> solver(&mesh,
                                            &law,
                                            zero_vector<double>(3),
                                            1.0,
                                            "",
                                            fixed_nodes);
        solver.AssembleSystem(true, true);
    }

    void TestAssembleSystem() throw (Exception)
    {
        QuadraticMesh<2> mesh(0.5, 1.0, 1.0);
        ExponentialMaterialLaw<2> law(2.0, 3.0);
        std::vector<unsigned> fixed_nodes;
        fixed_nodes.push_back(0);

        NonlinearElasticitySolver<2> solver(&mesh,
                                            &law,
                                            zero_vector<double>(2),
                                            1.0,
                                            "",
                                            fixed_nodes);
        solver.AssembleSystem(true, true);

        ///////////////////////////////////////////////////////////////////
        // test whether residual vector is currently zero (as
        // current solution should have been initialised to u=0, p=p0
        ///////////////////////////////////////////////////////////////////
        ReplicatableVector rhs_vec(solver.mpLinearSystem->rGetRhsVector());
        TS_ASSERT_EQUALS( rhs_vec.GetSize(), 2U*25U+9U );
        for (unsigned i=0; i<rhs_vec.GetSize(); i++)
        {
            TS_ASSERT_DELTA(rhs_vec[i], 0.0, 1e-12);
        }

        ///////////////////////////////////////////////////////////////////
        // compute numerical jacobian and compare with analytic jacobian
        // (about u=0, p=p0)
        ///////////////////////////////////////////////////////////////////
        unsigned num_dofs = rhs_vec.GetSize();
        double h = 1e-6;

        int lo, hi;
        MatGetOwnershipRange(solver.mpLinearSystem->rGetLhsMatrix(), &lo, &hi);

        for (unsigned j=0; j<num_dofs; j++)
        {
            solver.rGetCurrentSolution().clear();
            solver.FormInitialGuess();
            solver.rGetCurrentSolution()[j] += h;

            solver.AssembleSystem(true, false);

            ReplicatableVector perturbed_rhs( solver.mpLinearSystem->rGetRhsVector() );

            for (unsigned i=0; i<num_dofs; i++)
            {
                if ((lo<=(int)i) && ((int)i<hi))
                {
                    double analytic_matrix_val = solver.mpLinearSystem->GetMatrixElement(i,j);
                    double numerical_matrix_val = (perturbed_rhs[i] - rhs_vec[i])/h;
                    if ((fabs(analytic_matrix_val)>1e-6) && (fabs(numerical_matrix_val)>1e-6))
                    {
                        // relative error
                        TS_ASSERT_DELTA( (analytic_matrix_val-numerical_matrix_val)/analytic_matrix_val, 0.0, 1e-2);
                    }
                    else
                    {
                        // absolute error
                        TS_ASSERT_DELTA(analytic_matrix_val, numerical_matrix_val, 1e-2);
                    }
                }
            }
        }
        PetscTools::Barrier();

        //////////////////////////////////////////////////////////
        // compare numerical and analytic jacobians again, this
        // time using a non-zero displacement, u=lambda x, v = mu y
        // (lambda not equal to 1/nu), p = p0.
        //////////////////////////////////////////////////////////
        double lambda = 1.2;
        double mu = 1.0/1.3;

        solver.rGetCurrentSolution().clear();
        solver.FormInitialGuess();
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            solver.rGetCurrentSolution()[2*i]   = (lambda-1)*mesh.GetNode(i)->rGetLocation()[0];
            solver.rGetCurrentSolution()[2*i+1] = (mu-1)*mesh.GetNode(i)->rGetLocation()[1];
        }

        solver.AssembleSystem(true, true);
        ReplicatableVector rhs_vec2(solver.mpLinearSystem->rGetRhsVector());

        h=1e-8; // needs to be smaller for this one

        for (unsigned j=0; j<num_dofs; j++)
        {
            solver.rGetCurrentSolution()[j] += h;
            solver.AssembleSystem(true, false);

            ReplicatableVector perturbed_rhs( solver.mpLinearSystem->rGetRhsVector() );

            for (unsigned i=0; i<num_dofs; i++)
            {
                if ((lo<=(int)i) && ((int)i<hi))
                {
                    double analytic_matrix_val = solver.mpLinearSystem->GetMatrixElement(i,j);
                    double numerical_matrix_val = (perturbed_rhs[i] - rhs_vec2[i])/h;
                    if ((fabs(analytic_matrix_val)>1e-6) && (fabs(numerical_matrix_val)>1e-6))
                    {
                        // relative error
                        TS_ASSERT_DELTA( (analytic_matrix_val-numerical_matrix_val)/analytic_matrix_val, 0.0, 1e-2);
                    }
                    else
                    {
                        // absolute error
                        TS_ASSERT_DELTA(analytic_matrix_val, numerical_matrix_val, 1e-2);
                    }
                }
            }
            solver.rGetCurrentSolution()[j] -= h;
        }
    }

    void TestComputeResidualAndGetNormWithBadDeformation() throw(Exception)
    {
        QuadraticMesh<3> mesh;
        TrianglesMeshReader<3,3> mesh_reader1("mesh/test/data/3D_Single_tetrahedron_element_quadratic",2,1,false);
        mesh.ConstructFromMeshReader(mesh_reader1);
        NashHunterPoleZeroLaw<3> law;
        
        std::vector<unsigned> fixed_nodes;
        fixed_nodes.push_back(0);

        NonlinearElasticitySolver<3> solver(&mesh,
                                            &law,
                                            zero_vector<double>(3),
                                            1.0,
                                            "",
                                            fixed_nodes);

        // compute the residual norm - should be zero as no force or tractions
        TS_ASSERT_DELTA( solver.ComputeResidualAndGetNorm(false), 0.0, 1e-7);
        
        // the change the current solution (=displacement) to correspond to a small stretch
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            for (unsigned j=0; j<3; j++)
            {
                solver.rGetCurrentSolution()[3*i+j] = 0.01*mesh.GetNode(i)->rGetLocation()[j];
            }
        }

        // compute the residual norm - check computes fine
        TS_ASSERT_LESS_THAN( 0.0, solver.ComputeResidualAndGetNorm(false));

        // the change the current solution (=displacement) to correspond to a large stretch
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            for (unsigned j=0; j<3; j++)
            {
                solver.rGetCurrentSolution()[3*i+j] = mesh.GetNode(i)->rGetLocation()[j];
            }
        }

        // compute the residual norm - material law should complain - exception thrown...
        TS_ASSERT_THROWS_CONTAINS( solver.ComputeResidualAndGetNorm(false), "strain unacceptably large");
        // ..unless we set the allowException parameter to be true, in which case we should get
        // infinity returned.
        TS_ASSERT_EQUALS( solver.ComputeResidualAndGetNorm(true), DBL_MAX);
    }


    // A test where the solution should be zero displacement
    // It mainly tests that the initial guess was set up correctly to
    // the final correct solution, ie u=0, p=zero_strain_pressure (!=0)
    void TestWithZeroDisplacement() throw(Exception)
    {
        QuadraticMesh<2> mesh;
        TrianglesMeshReader<2,2> mesh_reader("mesh/test/data/square_128_elements_quadratic",2,1,false);
        mesh.ConstructFromMeshReader(mesh_reader);

        double c1 = 3.0;
        MooneyRivlinMaterialLaw<2> mooney_rivlin_law(c1);

        std::vector<unsigned> fixed_nodes;
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            if ( fabs(mesh.GetNode(i)->rGetLocation()[0])<1e-6)
            {
                fixed_nodes.push_back(i);
            }
        }

        NonlinearElasticitySolver<2> solver(&mesh,
                                            &mooney_rivlin_law,
                                            zero_vector<double>(2),
                                            1.0,
                                            "",
                                            fixed_nodes);

        // for coverage
        TS_ASSERT_THROWS_THIS(solver.SetWriteOutput(true),
                "Can\'t write output if no output directory was given in constructor");
        solver.SetWriteOutput(false);

        solver.Solve();
        TS_ASSERT_EQUALS(solver.GetNumNewtonIterations(), 0u);

        // get deformed position
        std::vector<c_vector<double,2> >& r_deformed_position
            = solver.rGetDeformedPosition();

        for (unsigned i=0; i<r_deformed_position.size(); i++)
        {
            TS_ASSERT_DELTA(mesh.GetNode(i)->rGetLocation()[0], r_deformed_position[i](0), 1e-8);
            TS_ASSERT_DELTA(mesh.GetNode(i)->rGetLocation()[1], r_deformed_position[i](1), 1e-8);
        }

        // check the final pressure
        std::vector<double>& r_pressures = solver.rGetPressures();
        TS_ASSERT_EQUALS(r_pressures.size(), mesh.GetNumVertices());
        for (unsigned i=0; i<r_pressures.size(); i++)
        {
            TS_ASSERT_DELTA(r_pressures[i], 2*c1, 1e-6);
        }
    }

    void TestSettingUpHeterogeneousProblem() throw(Exception)
    {
        // two element quad mesh on the square
        QuadraticMesh<2> mesh(1.0, 1.0, 1.0);

        MooneyRivlinMaterialLaw<2> law_1(1.0);
        MooneyRivlinMaterialLaw<2> law_2(5.0);
        std::vector<AbstractIncompressibleMaterialLaw<2>*> laws;
        laws.push_back(&law_1);
        laws.push_back(&law_2);

        std::vector<unsigned> fixed_nodes;
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            if ( fabs(mesh.GetNode(i)->rGetLocation()[0])<1e-6)
            {
                fixed_nodes.push_back(i);
            }
        }

        NonlinearElasticitySolver<2> solver(&mesh,
                                            laws,
                                            zero_vector<double>(2),
                                            1.0,
                                            "",
                                            fixed_nodes);

        TS_ASSERT_EQUALS(solver.mMaterialLaws.size(), 2u);
        TS_ASSERT_DELTA(solver.mMaterialLaws[0]->GetZeroStrainPressure(), 2.0, 1e-6);
        TS_ASSERT_DELTA(solver.mMaterialLaws[1]->GetZeroStrainPressure(), 10.0, 1e-6);

        unsigned num_nodes = 9;
        // pressure for node 0 (in elem 0)
        TS_ASSERT_DELTA(solver.rGetCurrentSolution()[2*num_nodes + 0], 2.0, 1e-6);
        // pressure for node 3 (in elem 1)
        TS_ASSERT_DELTA(solver.rGetCurrentSolution()[2*num_nodes + 3], 10.0, 1e-6);
    }

    void TestSolve() throw(Exception)
    {
        QuadraticMesh<2> mesh;
        TrianglesMeshReader<2,2> mesh_reader("mesh/test/data/square_128_elements_quadratic",2,1,false);
        mesh.ConstructFromMeshReader(mesh_reader);

        MooneyRivlinMaterialLaw<2> law(1.0);
        c_vector<double,2> body_force;
        body_force(0) = 3.0;
        body_force(1) = 0.0;

        std::vector<unsigned> fixed_nodes;
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            if ( fabs(mesh.GetNode(i)->rGetLocation()[0])<1e-6)
            {
                fixed_nodes.push_back(i);
            }
        }

        NonlinearElasticitySolver<2> solver(&mesh,
                                            &law,
                                            body_force,
                                            1.0,
                                            "simple_nonlin_elas",
                                            fixed_nodes);

        solver.Solve();
        TS_ASSERT_EQUALS(solver.GetNumNewtonIterations(), 4u); // 'hardcoded' answer, protects against jacobian getting messed up

        std::vector<c_vector<double,2> >& r_solution = solver.rGetDeformedPosition();

        double xend = 1.17199;
        double yend = 0.01001;

        ///////////////////////////////////////////////////////////
        // compare the solution at the corners with the values
        // obtained using the dealii finite elasticity solver
        //
        // Results have been visually checked to see they agree
        // (they do, virtually or completely overlapping.
        ///////////////////////////////////////////////////////////

        // bottom lhs corner should still be at (0,0)
        assert( fabs(mesh.GetNode(0)->rGetLocation()[0] - 0) < 1e-9 );
        assert( fabs(mesh.GetNode(0)->rGetLocation()[1] - 0) < 1e-9 );
        TS_ASSERT_DELTA( r_solution[0](0), 0.0, 1e-6 );
        TS_ASSERT_DELTA( r_solution[0](1), 0.0, 1e-6 );

        // top lhs corner should still be at (0,1)
        assert( fabs(mesh.GetNode(3)->rGetLocation()[0] - 0) < 1e-9 );
        assert( fabs(mesh.GetNode(3)->rGetLocation()[1] - 1) < 1e-9 );
        TS_ASSERT_DELTA( r_solution[3](0), 0.0, 1e-6 );
        TS_ASSERT_DELTA( r_solution[3](1), 1.0, 1e-6 );

        // DEALII value for bottom rhs corner is (1.17199,0.01001)
        assert( fabs(mesh.GetNode(1)->rGetLocation()[0] - 1) < 1e-9 );
        assert( fabs(mesh.GetNode(1)->rGetLocation()[1] - 0) < 1e-9 );
        TS_ASSERT_DELTA( r_solution[1](0), xend, 1e-3 );
        TS_ASSERT_DELTA( r_solution[1](1), yend, 1e-3 );

        // DEALII value for top rhs corner is (1.17199,0.98999)
        assert( fabs(mesh.GetNode(2)->rGetLocation()[0] - 1) < 1e-9 );
        assert( fabs(mesh.GetNode(2)->rGetLocation()[1] - 1) < 1e-9 );
        TS_ASSERT_DELTA( r_solution[2](0), xend,   1e-3 );
        TS_ASSERT_DELTA( r_solution[2](1), 1-yend, 1e-3 );

        MechanicsEventHandler::Headings();
        MechanicsEventHandler::Report();
    }



    /**
     *  Solve a problem with non-zero dirichlet boundary conditions
     *  and non-zero tractions. THIS TEST COMPARES AGAINST AN EXACT SOLUTION.
     *
     *  Choosing the deformation x=X/lambda, y=lambda*Y, with a
     *  Mooney-Rivlin material, then
     *   F = [1/lam 0; 0 lam], T = [2*c1-p*lam^2, 0; 0, 2*c1-p/lam^2],
     *   sigma = [2*c1/lam^2-p, 0; 0, 2*c1*lam^2-p].
     *  Choosing p=2*c1*lam^2, then sigma = [2*c1/lam^2-p 0; 0 0].
     *  The surface tractions are then
     *   TOP and BOTTOM SURFACE: 0
     *   RHS: s = SN = J*invF*sigma*N = [lam 0; 0 1/lam]*sigma*[1,0]
     *          = [2*c1(1/lam-lam^3), 0]
     *
     *  So, we have to specify displacement boundary conditions (y=lam*Y) on
     *  the LHS (X=0), and traction bcs (s=the above) on the RHS (X=1), and can
     *  compare the computed displacement and pressure against the true solution.
     *
     */
    void TestSolveWithNonZeroBoundaryConditions() throw(Exception)
    {
        double lambda = 0.85;
        double c1 = 1.0;
        c_vector<double,2> body_force = zero_vector<double>(2);
        unsigned num_elem = 5;

        QuadraticMesh<2> mesh(1.0/num_elem, 1.0, 1.0);
        MooneyRivlinMaterialLaw<2> law(c1);

        std::vector<unsigned> fixed_nodes;
        std::vector<c_vector<double,2> > locations;
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            if ( fabs(mesh.GetNode(i)->rGetLocation()[0])<1e-6)
            {
                fixed_nodes.push_back(i);
                c_vector<double,2> new_position;
                new_position(0) = 0;
                new_position(1) = lambda*mesh.GetNode(i)->rGetLocation()[1];
                locations.push_back(new_position);
            }
        }

        std::vector<BoundaryElement<1,2>*> boundary_elems;
        std::vector<c_vector<double,2> > tractions;
        c_vector<double,2> traction;
        traction(0) = 2*c1*(pow(lambda,-1) - lambda*lambda*lambda);
        traction(1) = 0;
        for (TetrahedralMesh<2,2>::BoundaryElementIterator iter
              = mesh.GetBoundaryElementIteratorBegin();
            iter != mesh.GetBoundaryElementIteratorEnd();
            ++iter)
        {
            if (fabs((*iter)->CalculateCentroid()[0] - 1.0)<1e-4)
            {
                BoundaryElement<1,2>* p_element = *iter;
                boundary_elems.push_back(p_element);
                tractions.push_back(traction);
            }
        }
        assert(boundary_elems.size()==num_elem);

        NonlinearElasticitySolver<2> solver(&mesh,
                                            &law,
                                            body_force,
                                            1.0,
                                            "nonlin_elas_non_zero_bcs",
                                            fixed_nodes,
                                            &locations);

        solver.SetSurfaceTractionBoundaryConditions(boundary_elems, tractions);

    	// coverage
        solver.SetKspAbsoluteTolerance(1e-10);

        solver.Solve();
        TS_ASSERT_EQUALS(solver.GetNumNewtonIterations(), 3u); // 'hardcoded' answer, protects against jacobian getting messed up

        std::vector<c_vector<double,2> >& r_solution = solver.rGetDeformedPosition();

        for (unsigned i=0; i<fixed_nodes.size(); i++)
        {
            unsigned index = fixed_nodes[i];
            TS_ASSERT_DELTA(r_solution[index](0), locations[i](0), 1e-8);
            TS_ASSERT_DELTA(r_solution[index](1), locations[i](1), 1e-8);
        }

        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            double exact_x = (1.0/lambda)*mesh.GetNode(i)->rGetLocation()[0];
            double exact_y = lambda*mesh.GetNode(i)->rGetLocation()[1];

            TS_ASSERT_DELTA( r_solution[i](0), exact_x, 1e-5 );
            TS_ASSERT_DELTA( r_solution[i](1), exact_y, 1e-5 );
        }

        for (unsigned i=0; i<mesh.GetNumVertices(); i++)
        {
            TS_ASSERT_DELTA( solver.rGetPressures()[i], 2*c1*lambda*lambda, 1e-5 );
        }


        MechanicsEventHandler::Headings();
        MechanicsEventHandler::Report();
    }

    /**
     *  Test with functional (rather than constant) body force and surface traction, against a known
     *  solution. Since a non-zero body force is used here and a known solution, this is the MOST
     *  IMPORTANT TEST.
     *
     *  Choose x=X+0.5*alpha*X*X, y=Y/(1+alpha*X), p=2c, then F has determinant 1, and S can be shown to
     *  be, where lam = 1 +alpha X (ie dx/dX)
     *    S = 2c[lam-1/lam,   -Y*alpha*lam^{-2}; -Y*alpha*lam^{-2}, 1/lam - lam]
     *  in which case the required body force and surface traction can be computed to be
     *
     *  b = 2c/density [ -alpha, -2*Y*alpha^2 * lam^{-3} ]
     *  s = 2c[lam-1/lam, -Y*alpha/(lam^2)] on X=1
     *  s = 2c[0, lam - 1/lam]              on Y=0
     *  s = 2c[Y*alpha/lam^2, 1/lam - lam]  on Y=1
     *
     */
    void TestWithFunctionalData() throw(Exception)
    {
        MechanicsEventHandler::Reset();

        c_vector<double,2> body_force = zero_vector<double>(2);

        unsigned num_elem = 5;
        QuadraticMesh<2> mesh(1.0/num_elem, 1.0, 1.0);

        MooneyRivlinMaterialLaw<2> law(MATERIAL_PARAM);

        std::vector<unsigned> fixed_nodes;
        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            if ( fabs(mesh.GetNode(i)->rGetLocation()[0])<1e-6)
            {
                fixed_nodes.push_back(i);
            }
        }

        std::vector<BoundaryElement<1,2>*> boundary_elems;
        for (TetrahedralMesh<2,2>::BoundaryElementIterator iter
              = mesh.GetBoundaryElementIteratorBegin();
            iter != mesh.GetBoundaryElementIteratorEnd();
            ++iter)
        {
            // get all boundary elems except those on X=0
            if (fabs((*iter)->CalculateCentroid()[0])>1e-6)
            {
                BoundaryElement<1,2>* p_element = *iter;
                boundary_elems.push_back(p_element);
            }
        }
        assert(boundary_elems.size()==3*num_elem);

        NonlinearElasticitySolver<2> solver(&mesh,
                                            &law,
                                            body_force,
                                            1.0,
                                            "nonlin_elas_functional_data",
                                            fixed_nodes);

        solver.SetFunctionalBodyForce(MyBodyForce);
        solver.SetFunctionalTractionBoundaryCondition(boundary_elems, MyTraction);

        solver.Solve();
        TS_ASSERT_EQUALS(solver.GetNumNewtonIterations(), 3u); // 'hardcoded' answer, protects against jacobian getting messed up

        // matrix might have (small) errors introduced if this fails
        TS_ASSERT_EQUALS(solver.GetNumNewtonIterations(), 3u);

        std::vector<c_vector<double,2> >& r_solution = solver.rGetDeformedPosition();

        for (unsigned i=0; i<mesh.GetNumNodes(); i++)
        {
            double X = mesh.GetNode(i)->rGetLocation()[0];
            double Y = mesh.GetNode(i)->rGetLocation()[1];

            double exact_x = X + 0.5*ALPHA*X*X;
            double exact_y = Y/(1+ALPHA*X);

            TS_ASSERT_DELTA(r_solution[i](0), exact_x, 1e-4);
            TS_ASSERT_DELTA(r_solution[i](1), exact_y, 1e-4);
        }

        for (unsigned i=0; i<solver.rGetPressures().size(); i++)
        {
            TS_ASSERT_DELTA( solver.rGetPressures()[i]/(2*MATERIAL_PARAM), 1.0, 1e-3);
        }

        MechanicsEventHandler::Headings();
        MechanicsEventHandler::Report();

		solver.rGetCurrentSolution().clear();
		solver.rGetCurrentSolution().resize(solver.mNumDofs, 0.0);
        solver.SetKspAbsoluteTolerance(1); // way too high
        TS_ASSERT_THROWS_CONTAINS(solver.Solve(), "KSP Absolute tolerance was too high");
    }
};

#endif /*TESTNONLINEARELASTICITYSOLVER_HPP_*/
