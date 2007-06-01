
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>


#include "TissueSimulation.hpp"
#include "Exception.hpp"
#include "CancerParameters.hpp"
#include <cmath>
#include <ctime>
#include <iostream>
#include <fstream>
#include <set>
#include "TrianglesMeshWriter.cpp"
#include "TrianglesMeshReader.hpp"
#include "SimulationTime.hpp"
#include "ColumnDataWriter.hpp"
#include "WntCellCycleModel.hpp"
#include "WntGradient.hpp"
#include "OutputFileHandler.hpp"

/** 
 *  Constructor
 *  @param cells is defaulted to the empty vector, in which case SetIncludeRandomBirth()
 *  should be called for any birth to happen.
 */
template<unsigned DIM> 
TissueSimulation<DIM>::TissueSimulation(ConformingTetrahedralMesh<DIM,DIM> &rMesh,
                                        std::vector<MeinekeCryptCell> cells)
        : mrMesh(rMesh),
          mCells(cells),
          mCrypt(rMesh, mCells)
{ 
    mpParams = CancerParameters::Instance();
    
    mDt = 1.0/120.0;
    mEndTime = 0.0; // hours - this is set later on.
    
    srandom(0);
    mFixedBoundaries = false;
    mOutputDirectory = "";
    
    // Set up the ghost nodes bool list
    mIsGhostNode.resize(mrMesh.GetNumAllNodes());
    for (unsigned i=0; i<mIsGhostNode.size(); i++)
    {
        mIsGhostNode[i] = false;
    }
    mCrypt.SetGhostNodes(mIsGhostNode);
    
    // defaults
    mReMesh = true;
    mNoBirth = false;
    mMaxCells = 10*mrMesh.GetNumNodes();
    mMaxElements = 10*mrMesh.GetNumElements();
    mWntIncluded = false;
    mNumBirths = 0;
    mNumDeaths = 0;
    mIncludeSloughing = true;
    
    SimulationTime* p_simulation_time = SimulationTime::Instance();
    if (!p_simulation_time->IsStartTimeSetUp())
    {
        #define COVERAGE_IGNORE
        EXCEPTION("Start time not set in simulation time singleton object");
        #undef COVERAGE_IGNORE
    }
    mCrypt.SetMaxCells(mMaxCells);
    mCrypt.SetMaxElements(mMaxElements);
    
}

/**
 * Free any memory allocated by the constructor
 */
template<unsigned DIM> 
TissueSimulation<DIM>::~TissueSimulation()
{
    SimulationTime::Destroy();
}

template<unsigned DIM> 
void TissueSimulation<DIM>::WriteVisualizerSetupFile(std::ofstream& rSetupFile)
{
    assert(DIM==2); // this is 2d specific
    rSetupFile << "MeshWidth\t" << mrMesh.GetWidth(0u);// get furthest distance between nodes in the x-direction
    rSetupFile.close();
}



/**
 * During a simulation time step, process any cell divisions that need to occur.
 * If the simulation includes cell birth, causes (almost) all cells that are ready to divide
 * to produce daughter cells.
 *
 * @return the number of births that occurred.
 */
template<unsigned DIM>  
unsigned TissueSimulation<DIM>::DoCellBirth()
{
    if (mNoBirth || mCells.empty())
    {
        return 0;
    }

    unsigned num_births_this_step = 0;

    // Iterate over all cells, seeing if each one can be divided
    for (typename Crypt<DIM>::Iterator cell_iter = mCrypt.Begin();
         cell_iter != mCrypt.End();
         ++cell_iter)
    {
        MeinekeCryptCell& cell = *cell_iter;
        Node<DIM>* p_our_node = cell_iter.GetNode();

        assert(cell.GetNodeIndex() == p_our_node->GetIndex());
        
        // Check for this cell dividing
        // Construct any influences for the cell cycle...

        std::vector<double> cell_cycle_influences;
        if (mWntIncluded)
        {
            assert(DIM==2);
            double y = p_our_node->rGetLocation()[1];
            double wnt_stimulus = mWntGradient.GetWntLevel(y);
            cell_cycle_influences.push_back(wnt_stimulus);
        }
        
        // CHECK if this cell is ready to divide - if so create a new cell etc.
        if (cell.ReadyToDivide(cell_cycle_influences))
        {
            // Create new cell
            MeinekeCryptCell new_cell = cell.Divide();
            std::cout << "Cell division at node " << cell.GetNodeIndex() << "\n";
        
            // Add a new node to the mesh
            c_vector<double, DIM> new_location = CalculateDividingCellCentreLocations(cell_iter);
            
            mCrypt.AddCell(new_cell, new_location);
            
            
            num_births_this_step++;
        } // if (ready to divide)
    } // cell iteration loop
   
    return num_births_this_step;
}


/**
 * During a simulation time step, process any cell sloughing or death
 *
 * At the moment we just slough cells by turning them into ghost nodes
 * 
 * @return the number of deaths that occurred.
 */ 
template<unsigned DIM> 
unsigned TissueSimulation<DIM>::DoCellRemoval()
{
    if(mCells.empty())
    {
        return 0;
    }
    
    unsigned num_deaths_this_step=0;
    // Sloughing by turning boundary nodes into ghost nodes.
    if (DIM==2 && mIncludeSloughing) // sloughing only happens in 2d
    {
        double crypt_length=mpParams->GetCryptLength();
        double crypt_width=mpParams->GetCryptWidth();

        for (typename Crypt<DIM>::Iterator cell_iter = mCrypt.Begin();
             cell_iter != mCrypt.End();
             ++cell_iter)
        {
            double x = cell_iter.rGetLocation()[0];
            double y = cell_iter.rGetLocation()[1];

            if ((x>crypt_width) || (x<0.0) || (y>crypt_length))
            { 
                mIsGhostNode[cell_iter.GetNode()->GetIndex()] = true;
                num_deaths_this_step++;
            }
        }
    }
    
    for(unsigned killer_index = 0; killer_index<mCellKillers.size(); killer_index++)
    {
        mCellKillers[killer_index]->TestAndLabelCellsForApoptosis();
        num_deaths_this_step += mCellKillers[killer_index]->RemoveDeadCells();
    }
    
    return num_deaths_this_step;
}



/**
 * Calculates the new locations of a dividing cell's cell centres.
 * Moves the dividing node a bit and returns co-ordinates for the new node.
 * It does this by picking a random direction (0->2PI) and placing the parent 
 * and daughter in opposing directions on this axis.
 * 
 * @param node_index The parent node index
 * 
 * @return daughter_coords The coordinates for the daughter cell.
 * 
 */
template<unsigned DIM> 
c_vector<double, DIM> TissueSimulation<DIM>::CalculateDividingCellCentreLocations(typename Crypt<DIM>::Iterator parentCell)
{
    double separation = 0.1;
    c_vector<double, DIM> parent_coords = parentCell.rGetLocation();
    c_vector<double, DIM> daughter_coords;
    
    // Make a random direction vector of the required length
    c_vector<double, DIM> random_vector;
    
    if(DIM==1)
    {
        random_vector(0) = 0.5*separation;
    }   
    else if(DIM==2)
    {
        double random_angle = RandomNumberGenerator::Instance()->ranf();
        random_angle *= 2.0*M_PI;
        random_vector(0) = 0.5*separation*cos(random_angle);
        random_vector(1) = 0.5*separation*sin(random_angle);
    }
    else if(DIM==3)
    {
        double random_zenith_angle = RandomNumberGenerator::Instance()->ranf();// phi 
        random_zenith_angle *= M_PI;
        double random_azimuth_angle = RandomNumberGenerator::Instance()->ranf();// theta
        random_azimuth_angle *= 2*M_PI;
        
        random_vector(0) = 0.5*separation*cos(random_azimuth_angle)*sin(random_zenith_angle);
        random_vector(1) = 0.5*separation*sin(random_azimuth_angle)*sin(random_zenith_angle);
        random_vector(2) = 0.5*separation*cos(random_zenith_angle);
    }
    
    if(DIM==2)
    {
        if  (  (parent_coords(1)-random_vector(1) > 0.0)
            && (parent_coords(1)+random_vector(1) > 0.0))
        {   // We are not too close to the bottom of the crypt
            // add random vector to the daughter and take it from the parent location
            daughter_coords = parent_coords+random_vector;
            parent_coords = parent_coords-random_vector;
        }
        else
        {   // Leave the parent where it is and move daughter in a positive direction
            // to ensure new cells are not born below y=0
            if (random_vector(1)>0.0)
            {
                daughter_coords = parent_coords+random_vector;
            }
            else
            {
                daughter_coords = parent_coords-random_vector;
            }
        }
        assert(daughter_coords(1)>=0.0);// to make sure dividing cells stay in the crypt
        assert(parent_coords(1)>=0.0);// to make sure dividing cells stay in the crypt
    }
    else
    {
        daughter_coords = parent_coords+random_vector;
        parent_coords = parent_coords-random_vector;
    } 
    
    // set the parent to use this location
    Point<DIM> parent_coords_point(parent_coords);
    mCrypt.MoveCell(parentCell, parent_coords_point);
    return daughter_coords;
}





/**
 * Calculates the forces on each node
 *
 * @return drdt the x and y force components on each node
 */
template<unsigned DIM>  
std::vector<c_vector<double, DIM> > TissueSimulation<DIM>::CalculateVelocitiesOfEachNode()
{
    std::vector<c_vector<double, DIM> > drdt(mrMesh.GetNumAllNodes());
    for (unsigned i=0; i<drdt.size(); i++)
    {
        drdt[i]=zero_vector<double>(DIM);
    }

    std::set<std::set<unsigned> > node_pairs_checked;

    ////////////////////////////////////////////////////////////////////
    // loop over element and for each one loop over its three edges
    ////////////////////////////////////////////////////////////////////
    for (unsigned elem_index = 0; elem_index<mrMesh.GetNumAllElements(); elem_index++)
    {
        Element<DIM,DIM>* p_element = mrMesh.GetElement(elem_index);
        if (!p_element->IsDeleted())
        {
            for (unsigned k=0; k<DIM+1; k++)
            {
                unsigned nodeA = k;
                
                for(unsigned l=k+1; l<k+DIM+1; l++)
                {
                    unsigned nodeB = l%(DIM+1);
    
                    assert(!p_element->GetNode(nodeA)->IsDeleted());
                    assert(!p_element->GetNode(nodeB)->IsDeleted());
                                    
                    unsigned nodeA_global_index = p_element->GetNode(nodeA)->GetIndex();
                    unsigned nodeB_global_index = p_element->GetNode(nodeB)->GetIndex();
                    
                    // check whether we have already worked out the force between these two...
                    bool is_force_already_calculated = false;
                    
                    std::set<unsigned> current_node_pair;
                    current_node_pair.insert(nodeA_global_index);
                    current_node_pair.insert(nodeB_global_index);
                    
                    // see if the node pair is in the set of node pairs done
                    std::set<std::set<unsigned> >::iterator set_iter = node_pairs_checked.find(current_node_pair);                    
                    if(set_iter!=node_pairs_checked.end())
                    {
                        // node pair found
                        is_force_already_calculated = true;
                    }
                    else
                    {
                        is_force_already_calculated = false;
                        // add the node pair to the list of node pairs
                        node_pairs_checked.insert(current_node_pair);
                    }
                    
                    if(!is_force_already_calculated)
                    {
                        c_vector<double, DIM> force = CalculateForceInThisSpring(p_element,nodeA,nodeB);
                         
                        double damping_constantA = mpParams->GetDampingConstantNormal();
                        double damping_constantB = mpParams->GetDampingConstantNormal();
                        
                        if(!mCells.empty())
                        {
                            //note: at the moment the index into the mCells vector is the same
                            //as the node index. later this may not be the case, in which case
                            //the following assertion will trip. to deal with this, a map from 
                            //node index to cell will be needed
                            assert( mCells[nodeA_global_index].GetNodeIndex()==nodeA_global_index);
                            assert( mCells[nodeB_global_index].GetNodeIndex()==nodeB_global_index);
                            
                            if(   (mCells[nodeA_global_index].GetMutationState()==HEALTHY)
                               || (mCells[nodeA_global_index].GetMutationState()==APC_ONE_HIT))
                            {
                                damping_constantA = mpParams->GetDampingConstantNormal();
                            }
                            else
                            {
                                damping_constantA = mpParams->GetDampingConstantMutant();
                            }
                            
                            if(   (mCells[nodeB_global_index].GetMutationState()==HEALTHY)
                               || (mCells[nodeB_global_index].GetMutationState()==APC_ONE_HIT))
                            {
                                damping_constantB = mpParams->GetDampingConstantNormal();
                            }
                            else
                            {
                                damping_constantB = mpParams->GetDampingConstantMutant();
                            }
                        }
                        
                        if (!mIsGhostNode[nodeA_global_index])
                        {
                            drdt[ p_element->GetNode(nodeB)->GetIndex()] -= force / damping_constantB;
                            
                            if (!mIsGhostNode[nodeB_global_index])
                            {
                                drdt[ p_element->GetNode(nodeA)->GetIndex()] += force / damping_constantA;
                            }
                        }
                        else
                        {
                            drdt[ p_element->GetNode(nodeA)->GetIndex()] += force / damping_constantA;
                                
                            if (mIsGhostNode[nodeB_global_index])
                            {
                                drdt[ p_element->GetNode(nodeB)->GetIndex()] -= force / damping_constantB;
                            }
                        }
                    }
                }
            }
        }
    }
  
    return drdt;
}



/**
 * @return the x and y forces in this spring
 */
template<unsigned DIM> 
c_vector<double, DIM> TissueSimulation<DIM>::CalculateForceInThisSpring(Element<DIM,DIM>*& rPElement,const unsigned& rNodeA,const unsigned& rNodeB)
{
    assert(rNodeA!=rNodeB);
    unsigned node_a_global_index = rPElement->GetNodeGlobalIndex(rNodeA);
    unsigned node_b_global_index = rPElement->GetNodeGlobalIndex(rNodeB);
    return CalculateForceBetweenNodes(node_a_global_index, node_b_global_index);
}

/**
 * Calculates the force between two nodes.
 * 
 * Note that this assumes they are connected by a spring and should therefore
 * only be called by CalculateForceInThisSpring or CalculateForceInThisBoundarySpring
 * 
 * @param NodeAGlobalIndex
 * @param NodeBGlobalIndex
 * 
 * @return The force exerted on Node A by Node B.
 */
template<unsigned DIM> 
c_vector<double, DIM> TissueSimulation<DIM>::CalculateForceBetweenNodes(const unsigned& rNodeAGlobalIndex, const unsigned& rNodeBGlobalIndex)
{
    assert(rNodeAGlobalIndex!=rNodeBGlobalIndex);
    c_vector<double, DIM> unit_difference;
    c_vector<double, DIM> node_a_location = mrMesh.GetNode(rNodeAGlobalIndex)->rGetLocation();
    c_vector<double, DIM> node_b_location = mrMesh.GetNode(rNodeBGlobalIndex)->rGetLocation();
    
    // there is reason not to substract one position from the other (cyclidrical meshes). clever gary
    unit_difference = mrMesh.GetVectorFromAtoB(node_a_location, node_b_location);   
    
    double distance_between_nodes = norm_2(unit_difference);
    
    unit_difference /= distance_between_nodes;
    
    double rest_length = 1.0;
    
    if ( (mCells.size()>0) &&  (!mIsGhostNode[rNodeAGlobalIndex])
                           &&  (!mIsGhostNode[rNodeBGlobalIndex]) )
    {
        double ageA = mCells[rNodeAGlobalIndex].GetAge();
        double ageB = mCells[rNodeBGlobalIndex].GetAge();
        if (ageA<1.0 && ageB<1.0 && fabs(ageA-ageB)<1e-6)
        {
            // Spring Rest Length Increases to normal rest length from 0.1 to normal rest length, 1.0, over 1 hour
            #define COVERAGE_IGNORE
            rest_length=(0.1+0.9*ageA);
            assert(rest_length<=1.0);
            #undef COVERAGE_IGNORE
        }
    }
    return mpParams->GetSpringStiffness() * unit_difference * (distance_between_nodes - rest_length);
}


/**
 * Moves each node to a new position for this timestep
 *
 * @param rDrDt the x and y force components on each node.
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::UpdateNodePositions(const std::vector< c_vector<double, DIM> >& rDrDt)
{

    // Iterate over all cells, seeing if each one can be divided
    for (typename Crypt<DIM>::Iterator cell_iter = mCrypt.Begin();
         cell_iter != mCrypt.End();
         ++cell_iter)
    {
        MeinekeCryptCell& cell = *cell_iter;
        unsigned index = cell.GetNodeIndex();
        
        Point<DIM> new_point(mrMesh.GetNode(index)->rGetLocation() + mDt*rDrDt[index]);
        
        if(DIM==2)
        {
            // TODO: simplify/remove these 2d cases
            if (mFixedBoundaries)
            {
                assert(DIM==2);
                c_vector<double, 2> node_position = mrMesh.GetNode(index)->rGetLocation();
                // All Boundaries x=0, x=crypt_width, y=0, y=crypt_length.
                if (   node_position[1]>0
                    && node_position[1]<mpParams->GetCryptLength()
                    && node_position[0]>0
                    && node_position[0]<mpParams->GetCryptWidth() )
                {
                    mCrypt.MoveCell(cell_iter, new_point);
                }
            }
            else if (mCells.size()>0)
            {
                if (mWntIncluded)
                {   
                    // A new Wnt feature - even stem cells can move as long as they don't go below zero.
                    if ( (new_point.rGetLocation()[1] < 0.0) && !mIsGhostNode[index])
                    {
                        new_point.rGetLocation()[1] = 0.0;
                    }
                    mCrypt.MoveCell(cell_iter, new_point);
                }
                else
                {
                    // THE 'USUAL' SCENARIO move any node as long as it is not a real stem cell.
                    if (mCells[index].GetCellType()!=STEM || mIsGhostNode[index])
                    {   
                        // if a cell wants to move below y<0 (most likely because it was
                        // just born from a stem cell), stop it doing so
                        if ( (new_point.rGetLocation()[1] < 0.0) && (!mIsGhostNode[index]))
                        {
                            // Here we give the cell a push upwards so that it doesn't get stuck on y=0 for ever.
                            // it is a bit of a hack to make it work nicely!
                            new_point.rGetLocation()[1] = 0.01;
                        }
                        mCrypt.MoveCell(cell_iter, new_point);
                    }
                }
            }
            else
            {
                // NOTE: once springs in their own tested class all crypt simulations should have cells, 
                // so delete this:
                // no cells, just fix any node on line y=0
                if (mrMesh.GetNode(index)->rGetLocation()[1]>0)
                {
                    mCrypt.MoveCell(cell_iter, new_point);
                }
            }
        }
        else
        {
            // 1d or 3d
            mCrypt.MoveCell(cell_iter, new_point);
        }
    }
    mCrypt.UpdateGhostPositions(rDrDt,mDt);
}


/**
 * Change the state of cells
 *
 * At the moment this turns cells to be differentiated
 * dependent on a protein concentration when using the Wnt model.
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::UpdateCellTypes()
{
    if (!mCells.empty())
    {
        // Designate cells as proliferating (transit) or
        // quiescent (differentiated) according to protein concentrations
        for (typename Crypt<DIM>::Iterator cell_iter = mCrypt.Begin();
             cell_iter != mCrypt.End();
             ++cell_iter)
        {
            cell_iter->UpdateCellType();
        }
    }
}


/**
 * Set the timestep of the simulation
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::SetDt(double dt)
{
    assert(dt>0);
    mDt=dt;
}

/**
 * Sets the end time and resets the timestep to be endtime/100
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::SetEndTime(double endTime)
{
    assert(endTime>0);
    mEndTime=endTime;
}

template<unsigned DIM> 
void TissueSimulation<DIM>::SetOutputDirectory(std::string outputDirectory)
{
    mOutputDirectory = outputDirectory;
}

/**
 * Sets the maximum number of cells that the simulation will contain (for use by the datawriter)
 * default value is set to 10x the initial mesh value by the constructor.
 */
template<unsigned DIM>  
void TissueSimulation<DIM>::SetMaxCells(unsigned maxCells)
{
    mMaxCells = maxCells;
    if (maxCells<mrMesh.GetNumAllNodes())
    {
        #define COVERAGE_IGNORE
        EXCEPTION("mMaxCells is less than the number of cells in the mesh.");
        #undef COVERAGE_IGNORE
    }
    mCrypt.SetMaxCells(maxCells);
}

/**
 * Sets the maximum number of elements that the simulation will contain (for use by the datawriter)
 * default value is set to 10x the initial mesh value by the constructor.
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::SetMaxElements(unsigned maxElements)
{
    mMaxElements = maxElements;
    if (maxElements<mrMesh.GetNumAllElements())
    {
        #define COVERAGE_IGNORE
        EXCEPTION("mMaxElements is less than the number of elements in the mesh.");
        #undef COVERAGE_IGNORE
    }
    mCrypt.SetMaxElements(maxElements);
}

/**
 * Call this before Solve() to fix the boundary of the mesh.
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::SetFixedBoundaries()
{
    mFixedBoundaries = true;    // This is called by a nightly test.
}


template<unsigned DIM> 
Crypt<DIM>& TissueSimulation<DIM>::rGetCrypt()
{
    return mCrypt;
}


/**
 * The mesh should be surrounded by at least one layer of ghost nodes.  These are nodes which
 * do not correspond to a cell, but are necessary for remeshing (because the remesher tries to
 * create a convex hull of the set of nodes) and visualising purposes.  The mesh is passed into
 * the constructor and the class is told about the ghost nodes by using this method.
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::SetGhostNodes(std::set<unsigned> ghostNodeIndices)
{
    // First set all to not be ghost nodes
    for (unsigned i=0 ; i<mIsGhostNode.size() ; i++)
    {
        mIsGhostNode[i] = false;
    }
 
    // then update which ones are.
    std::set<unsigned>::iterator iter = ghostNodeIndices.begin();
    while(iter!=ghostNodeIndices.end())
    {
        mIsGhostNode[*iter]=true;
        iter++;
    }
    
    mCrypt.SetGhostNodes(mIsGhostNode); // I think there should be one of these here 
    // (otherwise Crypt is only ever told about ghost nodes when no nodes are ghosts in the constructor).
}


/**
 * Set whether the mesh should be remeshed at every time step.
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::SetReMeshRule(bool remesh)
{
    mReMesh = remesh;
}


/**
 * Set the simulation to run with no birth.
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::SetNoBirth(bool nobirth)
{
    mNoBirth = nobirth;
}


/**
 * This automatically sets this to be a wnt dependent simulation.
 * You should supply cells with a wnt cell cycle...
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::SetWntGradient(WntGradientType wntGradientType)
{
    mWntIncluded = true;
    mWntGradient = WntGradient(wntGradientType);
}


/**
 * Add a cell killer to be used in this simulation
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::AddCellKiller(AbstractCellKiller<DIM>* pCellKiller)
{
    mCellKillers.push_back(pCellKiller);
}

/**
 * Get the cells vector
 * N.B. Returns a copy of the cells - any operations on them will not go back into the
 * simulation.
 * \todo change this to return a const reference
 */
template<unsigned DIM> 
std::vector<MeinekeCryptCell> TissueSimulation<DIM>::GetCells()
{
    assert(mCells.size()>0);
    return mCells;
}

/**
 * Whether each node is a ghost or not.
 * \todo change this to return a const reference
 */
template<unsigned DIM> 
std::vector <bool> TissueSimulation<DIM>::GetGhostNodes()
{
    return mIsGhostNode;
}

template<unsigned DIM>
void TissueSimulation<DIM>::SetNoSloughing()
{
    mIncludeSloughing = false;
}


/**
 * Get a node's location (ONLY FOR TESTING)
 *
 * @param the node index
 * @return the co-ordinates of this node.
 */
template<unsigned DIM> 
std::vector<double> TissueSimulation<DIM>::GetNodeLocation(const unsigned& rNodeIndex)
{
    std::vector<double> location;
    for(unsigned i=0; i<DIM; i++)
    {
        location.push_back( mrMesh.GetNode(rNodeIndex)->rGetLocation()[i] );
    }
    return location;
}

/**
 * Main Solve method.
 *
 * Once CryptSimulation object has been set up, call this to run simulation
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::Solve()
{ 
    // Set up the simulation time
    SimulationTime* p_simulation_time = SimulationTime::Instance();
    double current_time = p_simulation_time->GetDimensionalisedTime();
    
    unsigned num_time_steps = (unsigned) ((mEndTime-current_time)/mDt+0.5);

    if (current_time>0)//use the reset function if necessary
    {
        p_simulation_time->ResetEndTimeAndNumberOfTimeSteps(mEndTime, num_time_steps);
    }
    else
    {
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(mEndTime, num_time_steps);
    }
    

    if (mOutputDirectory=="")
    {
        #define COVERAGE_IGNORE
        EXCEPTION("OutputDirectory not set");
        #undef COVERAGE_IGNORE
    }
    

    double time_now = p_simulation_time->GetDimensionalisedTime();
    std::ostringstream time_string;
    time_string << time_now;
    
    std::string results_directory = mOutputDirectory +"/results_from_time_" + time_string.str();
    
    
    ///////////////////////////////////////////////////////////
    // Set up Simulation
    ///////////////////////////////////////////////////////////
    
    // Data writers for tabulated results data, used in tests
    // first construction clears out the folder
    ColumnDataWriter tabulated_node_writer(results_directory+"/tab_results", "tabulated_node_results",true);
    ColumnDataWriter tabulated_element_writer(results_directory+"/tab_results", "tabulated_element_results",false);
    
    mCrypt.SetupTabulatedWriters(tabulated_node_writer, tabulated_element_writer);//, element_writer_ids);
    
    // This keeps track of when tabulated results were last output
    unsigned tabulated_output_counter = 0;
    
    // Create output files for the visualizer
    OutputFileHandler output_file_handler(results_directory+"/vis_results/",false);
    out_stream p_node_file = output_file_handler.OpenOutputFile("results.viznodes");
    out_stream p_element_file = output_file_handler.OpenOutputFile("results.vizelements");
    out_stream p_setup_file = output_file_handler.OpenOutputFile("results.vizsetup");
    
    
    /* Age the cells to the correct time (cells set up with negative birth dates
     * to give some that are almost ready to divide).
     * 
     * TODO:For some strange reason this seems to take about 3 minutes for a realistic Wnt-Crypt.
     * Not sure why - when the same code was evaluated in a test it seemed almost instant.
     */
    if (!mCells.empty())
    {
        for (typename Crypt<DIM>::Iterator cell_iter = mCrypt.Begin();
             cell_iter != mCrypt.End();
             ++cell_iter)
        {
            //std::cout << "Preparing Cell "<< i << std::endl;
            double y = cell_iter.rGetLocation()[1];
            std::vector<double> cell_cycle_influences;
            if (mWntIncluded)
            {
                double wnt_stimulus = mWntGradient.GetWntLevel(y);
                cell_cycle_influences.push_back(wnt_stimulus);
            }
            // We don't use the result; this call is just to force the cells to age to time 0,
            // running their cell cycle models to get there.
            cell_iter->ReadyToDivide(cell_cycle_influences);
        }
    }

    UpdateCellTypes();
    
    // Write initial conditions to file for the visualizer.
    if(DIM==2)
    {
        WriteVisualizerSetupFile(*p_setup_file);
    }
    
    mCrypt.WriteResultsToFiles(tabulated_node_writer, 
                               tabulated_element_writer,
                               *p_node_file, *p_element_file,
                               false,
                               true);
                               
                               
    /////////////////////////////////////////////////////////////////////
    // Main time loop
    /////////////////////////////////////////////////////////////////////
    while (p_simulation_time->GetTimeStepsElapsed() < num_time_steps)
    {        
        std::cout << "** TIME = " << p_simulation_time->GetDimensionalisedTime() << " **" << std::endl;
        
        // remove deal cells before doing birth
        mNumDeaths += DoCellRemoval();

        if(mReMesh)
        {
            mCrypt.ReMesh();
        }

        mNumBirths += DoCellBirth();

        if(mReMesh)
        {
            mCrypt.ReMesh();
        }

        //  calculate node velocities
        std::vector<c_vector<double, DIM> > drdt = CalculateVelocitiesOfEachNode();
        
        // update node positions
        UpdateNodePositions(drdt);
        
        // Change the state of some cells
        // Only active for WntCellCycleModel at the moment
        // but mutations etc. could occur in this function
        UpdateCellTypes();
                
        
        // Increment simulation time here, so results files look sensible
        p_simulation_time->IncrementTimeOneStep();
        
        // Write results to file
        mCrypt.WriteResultsToFiles(tabulated_node_writer, 
                                   tabulated_element_writer, 
                                   *p_node_file, *p_element_file,
                                   tabulated_output_counter%80==0,
                                   true);
                            
        tabulated_output_counter++;
    }
    
    // Write end state to tabulated files (not visualizer - this
    // is taken care of in the main loop).
    mCrypt.WriteResultsToFiles(tabulated_node_writer, 
                               tabulated_element_writer, 
                               *p_node_file, *p_element_file,
                               true,
                               false);
                        
    tabulated_node_writer.Close();
    tabulated_element_writer.Close();
}



/**
 * Saves the whole crypt simulation for restarting later.
 *
 * Puts it in the folder mOutputDirectory/archive/
 * and the file "2dCrypt_at_time_<SIMULATION TIME>.arch"
 *
 * First archives simulation time then the simulation itself.
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::Save()
{
    SimulationTime* p_sim_time = SimulationTime::Instance();
    assert(p_sim_time->IsStartTimeSetUp());
    
    std::string archive_directory = mOutputDirectory + "/archive/";
    
    std::ostringstream time_stamp;
    time_stamp << p_sim_time->GetDimensionalisedTime();
    
    // create an output file handler in order to get the full path of the
    // archive directory. Note the false is so the handler doesn't clean
    // the directory
    OutputFileHandler handler(archive_directory, false);
    std::string archive_filename = handler.GetTestOutputDirectory() + "2dCrypt_at_time_"+time_stamp.str()+".arch";
    std::string mesh_filename = std::string("mesh_") + time_stamp.str();
    
    if(mReMesh)
    {
        mCrypt.ReMesh();
    }

    
    // the false is so the directory isn't cleaned
    TrianglesMeshWriter<DIM,DIM> mesh_writer(archive_directory, mesh_filename, false);
    mesh_writer.WriteFilesUsingMesh(mrMesh);
    
    std::ofstream ofs(archive_filename.c_str());
    boost::archive::text_oarchive output_arch(ofs);
    
    // cast to const.
    const SimulationTime* p_simulation_time = SimulationTime::Instance();
    output_arch << *p_simulation_time;
    output_arch << static_cast<const TissueSimulation<DIM>&>(*this);
}

/**
 * Loads a saved crypt simulation to run further.
 *
 * @param rArchiveDirectory the name of the simulation to load
 * (specified originally by simulator.SetOutputDirectory("wherever"); )
 * @param rTimeStamp the time at which to load the simulation (this must
 * be one of the times at which the simulation.Save() was called)
 */
template<unsigned DIM> 
void TissueSimulation<DIM>::Load(const std::string& rArchiveDirectory, const double& rTimeStamp)
{
    std::ostringstream time_stamp;
    time_stamp << rTimeStamp;
    
    SimulationTime *p_simulation_time = SimulationTime::Instance();
    
    OutputFileHandler any_old_handler("",false);
    std::string test_output_directory = any_old_handler.GetTestOutputDirectory();
    
    std::string archive_filename = test_output_directory + rArchiveDirectory + "/archive/2dCrypt_at_time_"+time_stamp.str() +".arch";
    std::string mesh_filename = test_output_directory + rArchiveDirectory + "/archive/mesh_" + time_stamp.str();
    
    mrMesh.Clear();
    TrianglesMeshReader<DIM,DIM> mesh_reader(mesh_filename);
    mrMesh.ConstructFromMeshReader(mesh_reader);
    
    // Create an input archive
    std::ifstream ifs(archive_filename.c_str(), std::ios::binary);
    boost::archive::text_iarchive input_arch(ifs);
    
    // read the archive
    assert(p_simulation_time->IsStartTimeSetUp());
    input_arch >> *p_simulation_time;
    input_arch >> *this;
    
    double time_now = p_simulation_time->GetDimensionalisedTime();
    std::ostringstream time_string;
    time_string << time_now;
    
    mOutputDirectory = rArchiveDirectory;
    
    if (mrMesh.GetNumNodes()!=mCells.size())
    {
        EXCEPTION(" Error in Load: number of nodes is not equal to number of cells.");
    }
}
