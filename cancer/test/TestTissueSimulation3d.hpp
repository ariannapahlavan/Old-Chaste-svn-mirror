#ifndef TESTTISSUESIMULATION3D_HPP_
#define TESTTISSUESIMULATION3D_HPP_

#include <cxxtest/TestSuite.h>
#include "TissueSimulation.cpp"

#include "ConformingTetrahedralMesh.cpp"
#include "TrianglesMeshReader.cpp"
#include "TrianglesMeshWriter.cpp"
#include <cmath>
#include <ctime>
#include <vector>
#include "OutputFileHandler.hpp"
#include "CancerParameters.hpp"
#include "NodeMap.hpp"
#include "TissueCell.hpp"
#include "FixedCellCycleModel.hpp"
#include "ColumnDataReader.hpp"
#include "SimulationTime.hpp"
#include "WntGradient.hpp"

class TestTissueSimulation3d : public CxxTest::TestSuite
{
    ConformingTetrahedralMesh<3,3> Make3dMesh(unsigned width=3, unsigned height=3, unsigned depth=3)
    {
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructCuboid(width,height,depth,true);
        TrianglesMeshWriter<3,3> mesh_writer("","3dSpringMesh");
        mesh_writer.WriteFilesUsingMesh(mesh);

        return mesh;
    }   

public:
    void TestDoCellBirth() throw (Exception)
    {
        CancerParameters::Instance()->Reset();
        RandomNumberGenerator::Instance();
        
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_1626_elements");
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetStartTime(0.0);
        
        // Set up cells by iterating through the mesh nodes
        unsigned num_cells = mesh.GetNumAllNodes();
        std::vector<TissueCell> cells;
        for (unsigned i=0; i<num_cells; i++)
        {
            CellType cell_type;
            unsigned generation;
            cell_type = STEM;
            generation = 0;
            TissueCell cell(cell_type, HEALTHY, generation, new FixedCellCycleModel());
            cell.SetNodeIndex(i);
            if ( i == 50u)
            {
                cell.SetBirthTime(-50.0 );
            }
            
            cells.push_back(cell);
        }
        
        Tissue<3> tissue(mesh,cells);
        TissueSimulation<3> simulator(tissue);
        
        simulator.SetMaxCells(400);
        simulator.SetMaxElements(2400);
        
        unsigned num_births = simulator.DoCellBirth();
                                                                
        TS_ASSERT_EQUALS(num_births, 1u);
        
        SimulationTime::Destroy();
        RandomNumberGenerator::Destroy();
    }
       
    void TestBirthOccursDuringSolve() throw (Exception)
    {
        CancerParameters::Instance()->Reset();

        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/3D_Single_tetrahedron_element");
        
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetStartTime(0.0);
        
        TissueCell cell(STEM, HEALTHY, 0u, new FixedCellCycleModel());
        std::vector<TissueCell> cells;
        for(unsigned i=0; i<mesh.GetNumNodes()-1; i++)
        {
            cell.SetNodeIndex(i);
            cell.SetBirthTime(0.0);
            cells.push_back(cell);
        }

        // Setting last cell to undergo cell birth.
        cell.SetNodeIndex(mesh.GetNumNodes()-1);
        cell.SetBirthTime(-50.0);
        cells.push_back(cell);
        
        Tissue<3> tissue(mesh,cells);
        TissueSimulation<3> simulator(tissue);
       
        TrianglesMeshWriter<3,3> mesh_writer1("Test3DCellBirth","StartMesh");
        mesh_writer1.WriteFilesUsingMesh(mesh);
        
        simulator.SetMaxCells(10);
        simulator.SetMaxElements(25);
        simulator.SetOutputDirectory("Test3DCellBirth");
        
        // Set to re-mesh
        simulator.SetReMeshRule(true);
        simulator.SetEndTime(1.0);
        
        simulator.Solve();

        TS_ASSERT_EQUALS(mesh.GetNumNodes(),5u);
        TS_ASSERT_EQUALS(mesh.GetNumElements(),3u);

        TrianglesMeshWriter<3,3> mesh_writer2("Test3DCellBirth","EndMesh",false); 
        mesh_writer2.WriteFilesUsingMesh(mesh);

        SimulationTime::Destroy();
    } 
    
    void TestSolveMethodSpheroidSimulation3D() throw (Exception)
    {
        CancerParameters *p_params = CancerParameters::Instance();
        p_params->Reset();
        RandomNumberGenerator *p_random_num_gen=RandomNumberGenerator::Instance();
                
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");
        ConformingTetrahedralMesh<3,3> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);

        TrianglesMeshWriter<3,3> mesh_writer("TestSolveMethodSpheroidSimulation3DMesh","StartMesh");
        mesh_writer.WriteFilesUsingMesh(mesh);
        
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetStartTime(0.0);

        // Set up cells by iterating through the mesh nodes
        unsigned num_cells = mesh.GetNumAllNodes();
        std::vector<TissueCell> cells;
        for (unsigned i=0; i<num_cells; i++)
        {
            CellType cell_type;
            unsigned generation;
            cell_type = STEM;
            generation = 0;
            TissueCell cell(cell_type, HEALTHY, generation, new FixedCellCycleModel());
            cell.SetNodeIndex(i);    
            cell.SetBirthTime(-p_random_num_gen->ranf()*
                               (p_params->GetStemCellG1Duration() + p_params->GetSG2MDuration())  );            
            cells.push_back(cell);
        } 
        
        Tissue<3> tissue(mesh,cells);
        TissueSimulation<3> simulator(tissue);
        
        simulator.SetMaxCells(1000);
        simulator.SetMaxElements(2500);
        simulator.SetOutputDirectory("TestSolveMethodSpheroidSimulation3D");
        
        // Set to re-mesh
        simulator.SetReMeshRule(true);
        simulator.SetEndTime(0.1);
        
        TS_ASSERT_THROWS_NOTHING(simulator.Solve());
        SimulationTime::Destroy();
        RandomNumberGenerator::Destroy();
        
        TrianglesMeshWriter<3,3> mesh_writer2("TestSolveMethodSpheroidSimulation3DMesh","EndMesh",false); 
        mesh_writer2.WriteFilesUsingMesh(mesh);      
    }
    
 
    void TestGhostNodesSpheroidSimulation3DandSave() throw (Exception)
    {
        double start_time = std::clock();
        
        CancerParameters *p_params = CancerParameters::Instance();
        p_params->Reset();
        RandomNumberGenerator *p_random_num_gen=RandomNumberGenerator::Instance();
                       
        unsigned width = 3;
        unsigned height = 3;               
        unsigned depth = 3;      
                 
        ConformingTetrahedralMesh<3,3> mesh = Make3dMesh(width,height,depth);
        TrianglesMeshWriter<3,3> mesh_writer("TestGhostNodesSpheroidSimulation3D","StartMesh");
        mesh_writer.WriteFilesUsingMesh(mesh);
                       
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetStartTime(0.0);
      
        // Set up cells by iterating through the mesh nodes
        unsigned num_cells = mesh.GetNumAllNodes();
        std::vector<TissueCell> cells;
                  
        c_vector<double, 3> spheroid_centre;
        spheroid_centre[0] = 0.5*((double) width);
        spheroid_centre[1] = 0.5*((double) height);
        spheroid_centre[2] = 0.5*((double) depth);
        
        std::set<unsigned> ghost_node_indices;

        for (unsigned i=0; i<num_cells; i++)
        {
            CellType cell_type;
            unsigned generation;
            
            c_vector<double, 3> node_location = mesh.GetNode(i)->rGetLocation();
            
            unsigned min_spatial_dimension;
            if (width <= height && width <= depth)
            {
                min_spatial_dimension = width;
            }
            else
            {
                if (height <= depth)
                {
                    min_spatial_dimension = height;
                }
                else
                {
                    min_spatial_dimension = depth;
                }
            }
            if ( norm_2(node_location - spheroid_centre) > 0.5*sqrt(3)*1.01*((double) min_spatial_dimension)/3.0 )
            {
                ghost_node_indices.insert(i);
            }
                
            cell_type = STEM;
            generation = 0;
            TissueCell cell(cell_type, HEALTHY, generation, new FixedCellCycleModel());
            cell.SetNodeIndex(i);    
            cell.SetBirthTime(-p_random_num_gen->ranf()*(  p_params->GetStemCellG1Duration() +
                                                           p_params->GetSG2MDuration()  ));      
            cells.push_back(cell);
        } 
                    
        TS_ASSERT(ghost_node_indices.size() < num_cells);
        TS_ASSERT(ghost_node_indices.size() > 0)
        
        Tissue<3> tissue(mesh,cells);
        tissue.SetGhostNodes(ghost_node_indices);        

        TissueSimulation<3> simulator(tissue);

        simulator.SetMaxCells(500);
        simulator.SetMaxElements(1000);
        simulator.SetOutputDirectory("TestGhostNodesSpheroidSimulation3D");
        
        // Set to re-mesh
        simulator.SetReMeshRule(true);
        simulator.SetEndTime(0.1);
        
        simulator.Solve();
        simulator.Save();
        
        // These lines generate result to test in the following Test. 
//        unsigned num_real_cells = p_simulator->rGetTissue().GetNumRealCells();
//        std::cout << "Num real cells = " << num_real_cells << "/" << num_cells << "\n" << std::flush;
 
        TrianglesMeshWriter<3,3> mesh_writer2("TestGhostNodesSpheroidSimulation3D","EndMesh",false); 
        mesh_writer2.WriteFilesUsingMesh(mesh);
        SimulationTime::Destroy();
        RandomNumberGenerator::Destroy();
        
        double end_time = std::clock();
        double elapsed_time = (end_time - start_time)/(CLOCKS_PER_SEC);
        std::cout <<  "Time of simulation " << elapsed_time << "\n" << std::flush; 
    }
    
    void TestLoadOf3DSimulation() throw (Exception)
    {
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetStartTime(0.0);
        
        TissueSimulation<3>* p_simulator = TissueSimulation<3>::Load("TestGhostNodesSpheroidSimulation3D", 0.1);
        unsigned num_cells = p_simulator->rGetTissue().GetNumRealCells();
        
        TS_ASSERT_EQUALS(num_cells, 8u);      
        TS_ASSERT_DELTA(p_simulation_time->GetDimensionalisedTime(), 0.1, 1e-9);  
        
        SimulationTime::Destroy();
        RandomNumberGenerator::Destroy();
        
        delete p_simulator;
    }
};

#endif /*TESTTISSUESIMULATION3D_HPP_*/

