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
#include <cmath>
#include <ctime>
#include <iostream>
#include <fstream>
#include <set>

#include "TissueSimulation.hpp"
#include "AbstractCellCentreBasedTissue.hpp"
#include "CancerEventHandler.hpp"
#include "LogFile.hpp"

template<unsigned DIM>
TissueSimulation<DIM>::TissueSimulation(AbstractTissue<DIM>& rTissue,
                                        std::vector<AbstractForce<DIM>*> forceCollection,
                                        bool deleteTissueAndForceCollection,
                                        bool initialiseCells)
    : mDt(1.0/120.0), // Timestep of 30 seconds (as per Meineke)
      mEndTime(0.0),  // hours - this is set later on
      mrTissue(rTissue),
      mDeleteTissue(deleteTissueAndForceCollection),
      mAllocatedMemoryForForceCollection(deleteTissueAndForceCollection),
      mInitialiseCells(initialiseCells),
      mNoBirth(false),
      mUpdateTissue(true),
      mOutputDirectory(""),
      mSimulationOutputDirectory(mOutputDirectory),
      mNumBirths(0),
      mNumDeaths(0),
      mSamplingTimestepMultiple(1),
      mForceCollection(forceCollection)
{
    mpConfig = TissueConfig::Instance();

    // This line sets a random seed of 0 if it wasn't specified earlier.
    mpRandomGenerator = RandomNumberGenerator::Instance();

    if (mInitialiseCells)
    {
        mrTissue.InitialiseCells();
    }
}


template<unsigned DIM>
TissueSimulation<DIM>::~TissueSimulation()
{
    if (mAllocatedMemoryForForceCollection)
    {
        for (typename std::vector<AbstractForce<DIM>*>::iterator force_iter = mForceCollection.begin();
             force_iter != mForceCollection.end();
             ++force_iter)
        {
            delete *force_iter;
        }
    }

    if (mDeleteTissue)
    {
        for (typename std::vector<AbstractCellKiller<DIM>*>::iterator it=mCellKillers.begin();
             it != mCellKillers.end();
             ++it)
        {
            delete *it;
        }
        delete &mrTissue;
    }
}


template<unsigned DIM>
unsigned TissueSimulation<DIM>::DoCellBirth()
{
    if (mNoBirth)
    {
        return 0;
    }

    unsigned num_births_this_step = 0;

    // Iterate over all cells, seeing if each one can be divided
    for (typename AbstractTissue<DIM>::Iterator cell_iter = mrTissue.Begin();
         cell_iter != mrTissue.End();
         ++cell_iter)
    {
        // Check if this cell is ready to divide - if so create a new cell etc.
        if (cell_iter->GetAge() > 0.0)
        {
            if (cell_iter->ReadyToDivide())
            {
                // Create a new cell
                TissueCell new_cell = cell_iter->Divide();

                //\todo This is specific to cell-centre based models, the location isn't used in a vertex simulation
                c_vector<double, DIM> new_location = zero_vector<double>(DIM);
                if (dynamic_cast<AbstractCellCentreBasedTissue<DIM>*>(&mrTissue))
                {
                    new_location = CalculateDividingCellCentreLocations(&(*cell_iter));
                }

                // Add new cell to the tissue
                mrTissue.AddCell(new_cell, new_location, &(*cell_iter));

                // Update counter
                num_births_this_step++;
            }
        }
    }
    return num_births_this_step;
}


template<unsigned DIM>
unsigned TissueSimulation<DIM>::DoCellRemoval()
{
    unsigned num_deaths_this_step = 0;

    // This labels cells as dead or apoptosing. It does not actually remove the cells,
    // tissue.RemoveDeadCells() needs to be called for this.
    for (typename std::vector<AbstractCellKiller<DIM>*>::iterator killer_iter = mCellKillers.begin();
         killer_iter != mCellKillers.end();
         ++killer_iter)
    {
        (*killer_iter)->TestAndLabelCellsForApoptosisOrDeath();
    }

    num_deaths_this_step += mrTissue.RemoveDeadCells();

    return num_deaths_this_step;
}


template<unsigned DIM>
const std::vector<AbstractForce<DIM>*> TissueSimulation<DIM>::rGetForceCollection() const
{
    return mForceCollection;
}


template<unsigned DIM>
c_vector<double, DIM> TissueSimulation<DIM>::CalculateDividingCellCentreLocations(TissueCell* pParentCell)
{
    // Location of parent and daughter cells
    c_vector<double, DIM> parent_coords = mrTissue.GetLocationOfCellCentre(pParentCell);
    c_vector<double, DIM> daughter_coords;

    // Get separation parameter
    double separation = TissueConfig::Instance()->GetDivisionSeparation();

    // Make a random direction vector of the required length
    c_vector<double, DIM> random_vector;

    /*
     * Pick a random direction and move the parent cell backwards by 0.5*separation 
     * in that direction and return the position of the daughter cell 0.5*separation 
     * forwards in that direction.
     */
    switch (DIM)
    {
        case 1:
        {
            double random_direction = -1.0 + 2.0*(RandomNumberGenerator::Instance()->ranf() < 0.5);

            random_vector(0) = 0.5*separation*random_direction;
            
            parent_coords = parent_coords - random_vector;
		    daughter_coords = parent_coords + random_vector;

            break;
        }
        case 2:
        {
            double random_angle = 2.0*M_PI*RandomNumberGenerator::Instance()->ranf();
    
            random_vector(0) = 0.5*separation*cos(random_angle);
            random_vector(1) = 0.5*separation*sin(random_angle);
            
            parent_coords = parent_coords - random_vector;
		    daughter_coords = parent_coords + random_vector;

            break;
        }
        case 3:
        {
            double random_zenith_angle = M_PI*RandomNumberGenerator::Instance()->ranf(); // phi
            double random_azimuth_angle = 2*M_PI*RandomNumberGenerator::Instance()->ranf(); // theta
    
            random_vector(0) = 0.5*separation*cos(random_azimuth_angle)*sin(random_zenith_angle);
            random_vector(1) = 0.5*separation*sin(random_azimuth_angle)*sin(random_zenith_angle);
            random_vector(2) = 0.5*separation*cos(random_zenith_angle);

            /// \todo Should really make this the same way round as the other cases, but this would break tests
            parent_coords = parent_coords + random_vector;
		    daughter_coords = parent_coords - random_vector;

            break;
        }
        default:
            // This can't happen
            NEVER_REACHED;
    }

    // Set the parent to use this location
    ChastePoint<DIM> parent_coords_point(parent_coords);
    unsigned node_index = mrTissue.GetLocationIndexUsingCell(pParentCell);
    mrTissue.SetNode(node_index, parent_coords_point);

    return daughter_coords;
}


template<unsigned DIM>
void TissueSimulation<DIM>::UpdateNodePositions(const std::vector< c_vector<double, DIM> >& rNodeForces)
{
    // Get the previous node positions (these may be needed
    // when applying boundary conditions, e.g. in the case
    // of immotile cells)
    std::vector<c_vector<double, DIM> > old_node_locations;
    old_node_locations.reserve(mrTissue.GetNumNodes());
    for (unsigned node_index=0; node_index<mrTissue.GetNumNodes(); node_index++)
    {
        old_node_locations[node_index] = mrTissue.GetNode(node_index)->rGetLocation();
    }

    // Update node locations
    mrTissue.UpdateNodeLocations(rNodeForces, mDt);

    // Apply any boundary conditions
    ApplyTissueBoundaryConditions(old_node_locations);
}


template<unsigned DIM>
void TissueSimulation<DIM>::SetDt(double dt)
{
    assert(dt > 0);
    mDt = dt;
}


template<unsigned DIM>
double TissueSimulation<DIM>::GetDt()
{
    return mDt;
}


template<unsigned DIM>
unsigned TissueSimulation<DIM>::GetNumBirths()
{
    return mNumBirths;
}


template<unsigned DIM>
unsigned TissueSimulation<DIM>::GetNumDeaths()
{
    return mNumDeaths;
}


template<unsigned DIM>
void TissueSimulation<DIM>::SetEndTime(double endTime)
{
    assert(endTime > 0);
    mEndTime = endTime;
}


template<unsigned DIM>
void TissueSimulation<DIM>::SetOutputDirectory(std::string outputDirectory)
{
    mOutputDirectory = outputDirectory;
    mSimulationOutputDirectory = mOutputDirectory;
}


template<unsigned DIM>
std::string TissueSimulation<DIM>::GetOutputDirectory()
{
    return mOutputDirectory;
}


template<unsigned DIM>
void TissueSimulation<DIM>::SetSamplingTimestepMultiple(unsigned samplingTimestepMultiple)
{
    assert(samplingTimestepMultiple > 0);
    mSamplingTimestepMultiple = samplingTimestepMultiple;
}


template<unsigned DIM>
AbstractTissue<DIM>& TissueSimulation<DIM>::rGetTissue()
{
    return mrTissue;
}


template<unsigned DIM>
const AbstractTissue<DIM>& TissueSimulation<DIM>::rGetTissue() const
{
    return mrTissue;
}


template<unsigned DIM>
void TissueSimulation<DIM>::SetUpdateTissueRule(bool updateTissue)
{
    mUpdateTissue = updateTissue;
}


template<unsigned DIM>
void TissueSimulation<DIM>::SetNoBirth(bool noBirth)
{
    mNoBirth = noBirth;
}


template<unsigned DIM>
void TissueSimulation<DIM>::AddCellKiller(AbstractCellKiller<DIM>* pCellKiller)
{
    mCellKillers.push_back(pCellKiller);
}


template<unsigned DIM>
std::vector<double> TissueSimulation<DIM>::GetNodeLocation(const unsigned& rNodeIndex)
{
    std::vector<double> location;
    for (unsigned i=0; i<DIM; i++)
    {
        location.push_back( mrTissue.GetNode(rNodeIndex)->rGetLocation()[i] );
    }
    return location;
}


template<unsigned DIM>
void TissueSimulation<DIM>::Solve()
{
    CancerEventHandler::BeginEvent(CancerEventHandler::EVERYTHING);
    CancerEventHandler::BeginEvent(CancerEventHandler::SETUP);

    // Set up the simulation time
    SimulationTime *p_simulation_time = SimulationTime::Instance();
    double current_time = p_simulation_time->GetTime();

    unsigned num_time_steps = (unsigned) ((mEndTime-current_time)/mDt+0.5);

    if (current_time > 0) // use the reset function if necessary
    {
        p_simulation_time->ResetEndTimeAndNumberOfTimeSteps(mEndTime, num_time_steps);
    }
    else
    {
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(mEndTime, num_time_steps);
    }

    if (mOutputDirectory=="")
    {
        EXCEPTION("OutputDirectory not set");
    }

    double time_now = p_simulation_time->GetTime();
    std::ostringstream time_string;
    time_string << time_now;

    std::string results_directory = mOutputDirectory +"/results_from_time_" + time_string.str();
    mSimulationOutputDirectory = results_directory;

    ///////////////////////////////////////////////////////////
    // Set up Simulation
    ///////////////////////////////////////////////////////////

    // Create output files for the visualizer
    OutputFileHandler output_file_handler(results_directory+"/", true);

    mrTissue.CreateOutputFiles(results_directory+"/", false);

    mpSetupFile = output_file_handler.OpenOutputFile("results.vizsetup");

    SetupSolve();

    // Age the cells to the correct time. Note that cells are created with
    // negative birth times so that some are initially almost ready to divide.
    LOG(1, "Setting up cells...");
    for (typename AbstractTissue<DIM>::Iterator cell_iter = mrTissue.Begin();
         cell_iter != mrTissue.End();
         ++cell_iter)
    {
        // We don't use the result; this call is just to force the cells to age
        // to the current time running their cell cycle models to get there
        cell_iter->ReadyToDivide();
    }
    LOG(1, "\tdone\n");

    // Write initial conditions to file for the visualizer
    if (DIM==2)
    {
        WriteVisualizerSetupFile();
    }
    *mpSetupFile << std::flush;

    mrTissue.WriteResultsToFiles();

    CancerEventHandler::EndEvent(CancerEventHandler::SETUP);

    // Initialise a vector of forces on node
    std::vector<c_vector<double, DIM> > forces(mrTissue.GetNumNodes(),zero_vector<double>(DIM));

    /////////////////////////////////////////////////////////////////////
    // Main time loop
    /////////////////////////////////////////////////////////////////////

    while ((p_simulation_time->GetTimeStepsElapsed() < num_time_steps) && !(StoppingEventHasOccurred()) )
    {
        LOG(1, "--TIME = " << p_simulation_time->GetTime() << "\n");

        /**
         * This function calls:
         * DoCellRemoval()
         * DoCellBirth()
         * Tissue::Update()
         */
        UpdateTissue();

        /////////////////////////
        // Calculate Forces
        /////////////////////////
        CancerEventHandler::BeginEvent(CancerEventHandler::FORCE);

        // First set all the forces to zero
        for (unsigned i=0; i<forces.size(); i++)
        {
             forces[i].clear();
        }

        // Then resize the std::vector if the number of cells has increased or decreased
        // (note this should be done after the above zeroing)
        if (mrTissue.GetNumNodes()!=forces.size())
        {
            forces.resize(mrTissue.GetNumNodes(), zero_vector<double>(DIM));
        }

        // Now add force contributions from each AbstractForce
        for (typename std::vector<AbstractForce<DIM>*>::iterator iter = mForceCollection.begin();
             iter !=mForceCollection.end();
             iter++)
        {
            (*iter)->AddForceContribution(forces, mrTissue);
        }
        CancerEventHandler::EndEvent(CancerEventHandler::FORCE);

        ////////////////////////////
        // Update node positions
        ////////////////////////////
        CancerEventHandler::BeginEvent(CancerEventHandler::POSITION);
        UpdateNodePositions(forces);
        CancerEventHandler::EndEvent(CancerEventHandler::POSITION);

        //////////////////////////////////////////
        // PostSolve, which may be implemented by
        // child classes (eg to solve nutrient pdes)
        //////////////////////////////////////////
        PostSolve();

        // Increment simulation time here, so results files look sensible
        p_simulation_time->IncrementTimeOneStep();

        ////////////////////////////
        // Output current results
        ////////////////////////////
        CancerEventHandler::BeginEvent(CancerEventHandler::OUTPUT);

        // Write results to file
        if (p_simulation_time->GetTimeStepsElapsed()%mSamplingTimestepMultiple==0)
        {
            mrTissue.WriteResultsToFiles();
        }

        CancerEventHandler::EndEvent(CancerEventHandler::OUTPUT);
    }

    LOG(1, "--END TIME = " << SimulationTime::Instance()->GetTime() << "\n");
    /* Carry out a final update so that tissue is coherent with new cell positions.
     * NB cell birth/death still need to be checked because they may be spatially-dependent.*/
    UpdateTissue();

    AfterSolve();

    CancerEventHandler::BeginEvent(CancerEventHandler::OUTPUT);
    mrTissue.CloseOutputFiles();

    *mpSetupFile << "Complete\n";
    mpSetupFile->close();

    CancerEventHandler::EndEvent(CancerEventHandler::OUTPUT);

    CancerEventHandler::EndEvent(CancerEventHandler::EVERYTHING);
}

template<unsigned DIM>
bool TissueSimulation<DIM>::StoppingEventHasOccurred()
{
    return false;
}


template<unsigned DIM>
c_vector<unsigned, NUM_CELL_MUTATION_STATES> TissueSimulation<DIM>::GetCellMutationStateCount()
{
    if (TissueConfig::Instance()->GetOutputCellMutationStates()==false)
    {
        EXCEPTION("Call TissueConfig::Instance()->SetOutputCellMutationStates(true) before using this function");
    }
    return mrTissue.GetCellMutationStateCount();
}


template<unsigned DIM>
c_vector<unsigned, NUM_CELL_TYPES> TissueSimulation<DIM>::GetCellTypeCount()
{
    if (TissueConfig::Instance()->GetOutputCellTypes()==false)
    {
        EXCEPTION("Call TissueConfig::Instance()->SetOutputCellTypes(true) before using this function");
    }
    return mrTissue.GetCellTypeCount();
}


template<unsigned DIM>
c_vector<unsigned, 5> TissueSimulation<DIM>::GetCellCyclePhaseCount()
{
    if (TissueConfig::Instance()->GetOutputCellCyclePhases()==false)
    {
        EXCEPTION("Call TissueConfig::Instance()->SetOutputCellCyclePhases(true) before using this function");
    }
    return mrTissue.GetCellCyclePhaseCount();
}

template<unsigned DIM>
void TissueSimulation<DIM>::UpdateTissue()
{
    /////////////////////////
    // Remove dead cells
    /////////////////////////
    CancerEventHandler::BeginEvent(CancerEventHandler::DEATH);
    unsigned deaths_this_step = DoCellRemoval();
    mNumDeaths += deaths_this_step;
    LOG(1, "\tNum deaths = " << mNumDeaths << "\n");
    CancerEventHandler::EndEvent(CancerEventHandler::DEATH);

    /////////////////////////
    // Divide cells
    /////////////////////////
    CancerEventHandler::BeginEvent(CancerEventHandler::BIRTH);
    unsigned births_this_step = DoCellBirth();
    mNumBirths += births_this_step;
    LOG(1, "\tNum births = " << mNumBirths << "\n");
    CancerEventHandler::EndEvent(CancerEventHandler::BIRTH);

    // This allows NodeBasedTissue::Update() to do the minimum amount of work
    bool births_or_death_occurred = ((births_this_step>0) || (deaths_this_step>0));

    ////////////////////////////
    // Update topology of tissue
    ////////////////////////////
    CancerEventHandler::BeginEvent(CancerEventHandler::UPDATETISSUE);
    if (mUpdateTissue)
    {
        LOG(1, "\tUpdating tissue...");
        mrTissue.Update(births_or_death_occurred);
        LOG(1, "\tdone.\n");
    }
    else if (births_or_death_occurred)
    {
        EXCEPTION("Tissue has had births or deaths but mUpdateTissue is set to false, please set it to true.");
    }
    CancerEventHandler::EndEvent(CancerEventHandler::UPDATETISSUE);
}

/////////////////////////////////////////////////////////////////////////////
// Explicit instantiation
/////////////////////////////////////////////////////////////////////////////


template class TissueSimulation<1>;
template class TissueSimulation<2>;
template class TissueSimulation<3>;
