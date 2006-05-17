#ifndef TESTBIDOMAINPDE_HPP_
#define TESTBIDOMAINPDE_HPP_


//#include <iostream>
#include <vector>

#include "InitialStimulus.hpp"
#include "EulerIvpOdeSolver.hpp"
#include "LuoRudyIModel1991OdeSystem.hpp"
#include "MonodomainPde.hpp"
#include "BidomainPde.hpp"
#include "OdeSolution.hpp"
#include "PetscSetupAndFinalize.hpp"
#include "AbstractCardiacCellFactory.hpp"
#include <petsc.h>
#include <cxxtest/TestSuite.h>


// cell factory for creating 2 cells, one with initial (intracellular) stimulus,
// second node without any stimulus
class MyCardiacCellFactory : public AbstractCardiacCellFactory<1>
{
private:
   InitialStimulus* mpStimulus;

public:
    
    MyCardiacCellFactory() : AbstractCardiacCellFactory<1>(0.01)
    {
        mpStimulus = new InitialStimulus(-80.0, 0.5);
    }
    
    AbstractCardiacCell* CreateCardiacCellForNode(int node)
    {                    
        if(node==0)
        {
            return new LuoRudyIModel1991OdeSystem(mpSolver, mpStimulus, mTimeStep);
        }
        else if(node==1)
        {
            return new LuoRudyIModel1991OdeSystem(mpSolver, mpZeroStimulus, mTimeStep);
        }
        else
        {
            assert(0);
        }
    }
    
    ~MyCardiacCellFactory(void)
    {
        delete mpStimulus;
    }
    
    int GetNumberOfNodes()
    {
        return 2;
    }
};


// 
class MyBidomainCellFactory : public MyCardiacCellFactory
{
private:
    AbstractStimulusFunction* mpExtracellularStimulus1;
    AbstractStimulusFunction* mpExtracellularStimulus2;
    
public:
    ~MyBidomainCellFactory()
    {
        delete mpExtracellularStimulus1;
        delete mpExtracellularStimulus2;
    }

    void FinaliseCellCreation(std::vector< AbstractCardiacCell* >* pCellsDistributed, int lo, int hi)
    {
        mpExtracellularStimulus1 = new InitialStimulus(-150,0.5);
        mpExtracellularStimulus2 = new InitialStimulus(-250,0.5);

        int global_index = 0;
        if((global_index>=lo) && (global_index<hi))
        {
            int local_index = global_index - lo;        
            (*pCellsDistributed)[local_index]->SetExtracellularStimulusFunction( mpExtracellularStimulus1 );
        }

        global_index = 1;
        if((global_index>=lo) && (global_index<hi))
        {
            int local_index = global_index - lo;        
            (*pCellsDistributed)[local_index]->SetExtracellularStimulusFunction( mpExtracellularStimulus2 );
        }
    }    
};



class TestBidomainPde : public CxxTest::TestSuite
{
    public:    
    
    void testBidomainPdeGetSet( void )
    {
        double start_time = 0;  
        double big_time_step = 0.5;        
        MyBidomainCellFactory bidomain_cell_factory; // same as cell factory but with extracell stimuli

        BidomainPde<1>   bidomain_pde( &bidomain_cell_factory, start_time, big_time_step );   
        
        bidomain_pde.SetSurfaceAreaToVolumeRatio(3.14);
        TS_ASSERT_DELTA( bidomain_pde.GetSurfaceAreaToVolumeRatio(), 3.14, 1e-10);
        
        bidomain_pde.SetCapacitance(2.718);
        TS_ASSERT_DELTA( bidomain_pde.GetCapacitance(), 2.718, 1e-10);

        c_matrix<double, 1,1> sigma_i;
        c_matrix<double, 1,1> sigma_e;

//        MatrixDouble sigma_i(1,1);
        sigma_i(0,0) = 314;
        bidomain_pde.SetIntracellularConductivityTensor(sigma_i);

//        MatrixDouble sigma_e(1,1);
        sigma_e(0,0) = 218;
        bidomain_pde.SetExtracellularConductivityTensor(sigma_e);
        
//        MatrixDouble*
        c_matrix<double, 1,1> sigma = bidomain_pde.GetIntracellularConductivityTensor();
        TS_ASSERT_DELTA( sigma(0,0), 314, 1e-10);

        sigma = bidomain_pde.GetExtracellularConductivityTensor();
        TS_ASSERT_DELTA( sigma(0,0), 218, 1e-10);
    }
    
    void testBidomainPde_PrepareForAssembleSolution( void )
    {   
        double start_time = 0;  
        double big_time_step = 0.5;        
        MyCardiacCellFactory cell_factory;
        MyBidomainCellFactory bidomain_cell_factory; // same as cell factory but with extracell stimuli

        MonodomainPde<1> monodomain_pde( &cell_factory, start_time, big_time_step );        
        BidomainPde<1>   bidomain_pde( &bidomain_cell_factory, start_time, big_time_step );   
        
        // voltage that gets passed in solving ode
        double initial_voltage = -83.853;
 
        int num_nodes = 2;
        // initial condition;   
        Vec monodomain_voltage, bidomain_voltage;
        VecCreate(PETSC_COMM_WORLD, &monodomain_voltage);
        VecSetSizes(monodomain_voltage, PETSC_DECIDE, num_nodes);
        VecSetFromOptions(monodomain_voltage);
        
        int lo, hi;
        VecGetOwnershipRange(monodomain_voltage,&lo,&hi);

        VecCreateMPI(PETSC_COMM_WORLD, 2*(hi-lo), 2*num_nodes, &bidomain_voltage);
        
        double *p_monodomain_voltage, *p_bidomain_voltage;
        VecGetArray(monodomain_voltage, &p_monodomain_voltage);
        VecGetArray(bidomain_voltage, &p_bidomain_voltage);
        for (int global_index = 0; global_index < num_nodes; global_index++ )
        {
            int local_index = global_index - lo;
            // initial voltage condition of a constant everywhere on the mesh
            if (lo<=global_index && global_index<hi)
            {
                p_monodomain_voltage[local_index] = initial_voltage;
                p_bidomain_voltage[2*local_index] = initial_voltage;
            }
        }
        VecRestoreArray(monodomain_voltage, &p_monodomain_voltage);
        VecRestoreArray(bidomain_voltage, &p_bidomain_voltage);
        VecAssemblyBegin(monodomain_voltage);
        VecAssemblyEnd(monodomain_voltage);
        VecAssemblyBegin(bidomain_voltage);
        VecAssemblyEnd(bidomain_voltage);
        
        //VecView(monodomain_voltage, PETSC_VIEWER_STDOUT_WORLD);
        //VecView(bidomain_voltage, PETSC_VIEWER_STDOUT_WORLD);
        
        monodomain_pde.PrepareForAssembleSystem(monodomain_voltage);
        bidomain_pde.PrepareForAssembleSystem(bidomain_voltage);         

        // Check that both the monodomain and bidomain PDE have the same ionic cache
        for (int global_index=lo; global_index < hi; global_index++)
        {
            TS_ASSERT_EQUALS(monodomain_pde.GetIionicCacheReplicated()[global_index], bidomain_pde.GetIionicCacheReplicated()[global_index]);
        }

        // Check that the bidomain PDE has the right intracellular stimulus at node 0 and 1
        TS_ASSERT_EQUALS(bidomain_pde.GetIntracellularStimulusCacheReplicated()[0], -80);
        TS_ASSERT_EQUALS(bidomain_pde.GetIntracellularStimulusCacheReplicated()[1], 0);

        // Check that the bidomain PDE has the right extracellular stimulus at node 0 and 1
        TS_ASSERT_EQUALS(bidomain_pde.GetExtracellularStimulusCacheReplicated()[0], -150);
        TS_ASSERT_EQUALS(bidomain_pde.GetExtracellularStimulusCacheReplicated()[1], -250);
        
        VecDestroy(monodomain_voltage);
        VecDestroy(bidomain_voltage);
    }
};        

#endif /*TESTBIDOMAINPDE_HPP_*/
