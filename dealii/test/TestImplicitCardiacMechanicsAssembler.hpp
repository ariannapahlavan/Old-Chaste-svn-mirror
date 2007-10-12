#ifndef TESTIMPLICITCARDIACMECHANICSASSEMBLER_HPP_
#define TESTIMPLICITCARDIACMECHANICSASSEMBLER_HPP_

#include <cxxtest/TestSuite.h>
#include "CardiacMechanicsAssembler.cpp"
#include "ImplicitCardiacMechanicsAssembler.hpp"
#include "TriangulationVertexIterator.hpp"
#include "DofVertexIterator.hpp"
#include "FiniteElasticityTools.hpp"


class TestImplicitCardiacMechanicsAssembler : public CxxTest::TestSuite
{
public:
    void TestCompareJacobians() throw(Exception)
    {
        Triangulation<2> mesh;
        GridGenerator::hyper_cube(mesh, 0.0, 1.0);
        
        Point<2> zero;
        FiniteElasticityTools<2>::FixFacesContainingPoint(mesh, zero);
        
        // specify this material law so the test continues to pass when the default
        // material law is changed.
        MooneyRivlinMaterialLaw<2> material_law(0.02);

        ImplicitCardiacMechanicsAssembler<2> implicit_assembler(&mesh,"",&material_law);

        std::vector<double> calcium_conc(implicit_assembler.GetTotalNumQuadPoints(), 0.0);
        
        for(unsigned i=0; i<calcium_conc.size(); i++)
        {
            calcium_conc[i] = 0.05;
        }

        implicit_assembler.SetForcingQuantity(calcium_conc);

        // NOTE: calling CompareJacobians below bypasses calling Solve(t0,t1,dt), hence the
        // time info will not be set. We therefore must explicitly set them here.
        implicit_assembler.mCurrentTime = 0.0;
        implicit_assembler.mNextTime = 0.01;
        implicit_assembler.mDt = 0.01;

        // higher tolerance than CardiacMechAssembler. difficult to say if the dTa/dU_I bits 
        // in the jacobian are quite correct or if there is some numerical error in the 
        // numerical jacobian.
        TS_ASSERT_THROWS_NOTHING( implicit_assembler.CompareJacobians(3e-5) );
    }
    
    // it is important to compare the jacs when the fibre direction is not the C00 direction.
    void TestCompareJacobiansDifferentFibreDirection()
    {
        Triangulation<2> mesh;
        GridGenerator::hyper_cube(mesh, 0.0, 1.0);
        
        Point<2> zero;
        FiniteElasticityTools<2>::FixFacesContainingPoint(mesh, zero);
        
        MooneyRivlinMaterialLaw<2> material_law(0.02);
        ImplicitCardiacMechanicsAssembler<2> implicit_assembler(&mesh,"",&material_law);
        
        Tensor<2,2> P;
        P[0][0] =  0;
        P[0][1] =  1;
        P[1][0] = -1;
        P[1][1] =  0;

        // currently, the implicit assembler doesn't do different fibre directions,
        // so make sure we get an error if we try
        TS_ASSERT_THROWS_ANYTHING(implicit_assembler.SetFibreSheetMatrix(P));
    }
    
    
    void TestCompareWithExplicitSolver()
    {
        // solve an implicit deformation, with some [Ca] forcing term. Then
        // get the active tension, pass that into an explicit solver, and 
        // check the result is the same.
        Triangulation<2> mesh;
        GridGenerator::hyper_cube(mesh, 0.0, 1.0);
        mesh.refine_global(3);
        
        Point<2> zero;
        FiniteElasticityTools<2>::FixFacesContainingPoint(mesh, zero);
        
        // specify this material law so the test continues to pass when the default
        // material law is changed.
        MooneyRivlinMaterialLaw<2> material_law(0.02);

        ImplicitCardiacMechanicsAssembler<2> implicit_assembler(&mesh,
                                                                "ImplicitCardiacMech",
                                                                &material_law);

        std::vector<double> calcium_conc(implicit_assembler.GetTotalNumQuadPoints(), 1);
        implicit_assembler.SetForcingQuantity(calcium_conc);

        implicit_assembler.Solve(0,0.01,0.01);

        // get the active tensions
        std::vector<double> active_tension(implicit_assembler.GetTotalNumQuadPoints());
        for(unsigned i=0; i<active_tension.size(); i++)
        {
            active_tension[i] = implicit_assembler.mCellMechSystems[i].GetActiveTension();
        }

        CardiacMechanicsAssembler<2> explicit_assembler(&mesh,"",&material_law);
        explicit_assembler.SetForcingQuantity(active_tension);

        // overwrite the current solution with what should be the true solution 
        // from the implicit solve
        explicit_assembler.mCurrentSolution = implicit_assembler.mCurrentSolution;
        // need to call this otherwise the assembler thinks it is the first time 
        // so it guesses the ZeroDeformation solution for the pressure
        explicit_assembler.mADeformedHasBeenSolved=true;

        explicit_assembler.Solve(0,0.01,0.01);

        // check there were no newton iterations needed, ie the solution of the implicit 
        // method with these active tensions is the same as the solution of the explicit
        TS_ASSERT_EQUALS(explicit_assembler.GetNumNewtonIterations(),0u);

        //// NOTES:
        // if we don't provide the implicit solution to the explicit assembler, and let the
        // explicit assembler find it's own solution, the explicit solution can be compared 
        // to the implicit solution with the below code. With 
        // FiniteElasticityAssembler::NEWTON_ABS_TOL = 1e-13 and AbstractDealiiAssembler->gmres
        // tol = 1e-6 the results are that close visually. 2e-15 and 1e-8 is much better.
        //
        // std::vector<Vector<double> >& r_impl_solution = implicit_assembler.rGetDeformedPosition();
        // std::vector<Vector<double> >& r_expl_solution = explicit_assembler.rGetDeformedPosition();
        // for(unsigned i=0; i<r_impl_solution[0].size(); i++)
        // {
        //     for(unsigned j=0; j<2; j++)
        //     {
        //         double tol = fabs(r_impl_solution[j](i)/100);
        //         TS_ASSERT_DELTA(r_impl_solution[j](i),r_expl_solution[j](i),tol);
        //     }
        // }
    }
};

#endif /*TESTIMPLICITCARDIACMECHANICSASSEMBLER_HPP_*/
