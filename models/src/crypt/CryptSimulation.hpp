#ifndef CRYPTSIMULATION_HPP_
#define CRYPTSIMULATION_HPP_

#include "ConformingTetrahedralMesh.cpp"
#include "MeinekeCryptCell.hpp"
#include "CancerParameters.hpp"
#include "SimulationTime.hpp"
#include "StochasticCellCycleModel.hpp"
#include "Exception.hpp"
#include <cmath>
#include <ctime>

/**
 * Solve a crypt simulation based on the Meineke paper.
 *
 * The spring lengths are governed by the equations
 * dr/dt = stem_cycle_time*(mu/eta) sum_j r_hat_i,j*(|r_i,j|-s0)
 *       = alpha sum_j r_hat_i,j*(|r_i,j|-s0)
 *
 * where alpha = stem_cycle_time*(mu/eta) = stem_cycle_time*meineke_lambda.
 *       s0    = natural length of the spring

 * Length is scaled by natural length
 * Time is scaled by a stem cell cycle time

 * meineke_lambda = mu (spring constant) / eta (damping) = 0.01 (from Meineke - note
 * that the value we use for Meineke lambda is completely different because we have
 * nondimensionalised)
 */

class CryptSimulation
{
private:
    double mDt;
    double mEndTime;
    ConformingTetrahedralMesh<1,1> &mrMesh;
    
    bool mIncludeRandomBirth;
    bool mIncludeVariableRestLength;
    
    std::string mOutputDirectory;
    
    std::vector<MeinekeCryptCell> mCells;
    
    CancerParameters *mpParams;
    RandomNumberGenerators *mpGen;
    
public:

    /** Constructor
     *  @param cells is defaulted to the empty vector, in which case SetIncludeRandomBirth()
     *  should be called for any birth to happen.
     */
    CryptSimulation(ConformingTetrahedralMesh<1,1> &rMesh,
                    std::vector<MeinekeCryptCell> cells = std::vector<MeinekeCryptCell>(), 
                    RandomNumberGenerators *pGen = NULL)
            : mrMesh(rMesh),
            mCells(cells)
    {
        if (pGen!=NULL)
        {
        	mpGen=pGen;
        }
        else
        {
        	mpGen= new RandomNumberGenerators;
         }
        mpParams = CancerParameters::Instance();
        mDt = 1.0/(mpParams->GetStemCellCycleTime()*120); // NOTE: hardcoded 120?
        mEndTime = 5.0;
       
        mpParams->SetMeinekeLambda(15.0);
        
        mIncludeRandomBirth = false;
        mIncludeVariableRestLength = false;
        mOutputDirectory = "";
    }
    
    void SetDt(double dt)
    {
        assert(dt>0);
        mDt=dt;
    }
    
    void SetEndTime(double endTime)
    {
        assert(endTime>0);
        mEndTime=endTime;
    }
    
    void SetCryptLength(double cryptLength)
    {
        mpParams->SetCryptLength(cryptLength);
    }
    
    /**
     *  Call this before Solve() if no cells have been specified. Randomly adds a new 
     *  node every 1 time unit, starting 0.1
     */
    void SetIncludeRandomBirth()
    {
        mIncludeRandomBirth = true;
    }
    
    void SetOutputDirectory(std::string outputDirectory)
    {
        mOutputDirectory = outputDirectory;
    }
    
    /**
     *  Call this before Solve() to simulate cell growth after cell division.
     *  (will eventually become SetIncludeCellBirth() and then become the default)
     */
    void SetIncludeVariableRestLength()
    {
        mIncludeVariableRestLength = true;
    }
    
    /**
     * Main Solve method.
     * 
     * Once CryptSimulation object has been set up, call this to run simulation
     */
    void Solve()
    {
        if (mOutputDirectory=="")
        {
            EXCEPTION("OutputDirectory not set");
        }
        
        int num_time_steps = (int)(mEndTime/mDt+0.5);
        
        SimulationTime *p_simulation_time =
        	SimulationTime::Instance(mEndTime*mpParams->GetStemCellCycleTime(),
        							 num_time_steps);
        							 
        double time = 0.0;
        double time_since_last_birth = 0.9;
        
        int num_births = 0;
        int num_deaths = 0;
        
        std::vector<double> new_point_position(mrMesh.GetNumAllNodes());
        
        OutputFileHandler output_file_handler(mOutputDirectory);
        out_stream p_results_file = output_file_handler.OpenOutputFile("results");
        
        while (p_simulation_time->GetTimeStepsElapsed() < num_time_steps)
        {
            // Cell birth
            if (mIncludeRandomBirth && time_since_last_birth > 1)
            {
                unsigned new_node_index = AddRandomNode(time);
                time_since_last_birth = 0 ;
                // Create new cell note all are Stem Cells and have generation 0 for random birth
                RandomNumberGenerators *pGen=new RandomNumberGenerators;
                CryptCellType cell_type=STEM ;
                unsigned generation=0;
                MeinekeCryptCell new_cell(cell_type, time, generation, new StochasticCellCycleModel(pGen));
                // Update cells vector
               
                new_cell.SetNodeIndex(new_node_index);
                if (new_node_index == mCells.size())
                {
                    mCells.push_back(new_cell);
                }
                else
                {
                    mCells[new_node_index] = new_cell;
                }
                //num_births++;
                
            }
            else if (!mCells.empty())
            {
                for (unsigned i=0; i<mCells.size(); i++)
                {
                    if (mrMesh.GetNodeAt(i)->IsDeleted()) continue; // Skip deleted cells
                    // Check for this cell dividing
                    if (mCells[i].ReadyToDivide(time))//*mpParams->GetStemCellCycleTime()))
                    {
                        // Create new cell
                        MeinekeCryptCell new_cell = mCells[i].Divide(time);
                        
                        // Add new node to mesh
                        Node<1> *p_our_node = mrMesh.GetNodeAt(i);
                        
                        //Note: May need to check which side element is put esp. at the ends
                        Element<1,1> *p_element = mrMesh.GetElement(p_our_node->GetNextContainingElementIndex());
                        
                        unsigned new_node_index = AddNodeToElement(p_element,time);
                        // Update cells vector
                        new_cell.SetNodeIndex(new_node_index);
                        if (new_node_index == mCells.size())
                        {
                            mCells.push_back(new_cell);
                        }
                        else
                        {
                            mCells[new_node_index] = new_cell;
                        }
                        num_births++;
                        //std::cout<< "num_births=" << num_births <<std::endl<< std::flush;
                    }
                }
            }
            
            // calculate node velocities
            std::vector<double> drdt(mrMesh.GetNumAllNodes());
            
            if (mIncludeVariableRestLength && !mCells.empty())
            {
                for (int elem_index = 0; elem_index<mrMesh.GetNumAllElements(); elem_index++)
                {
                    Element<1,1>* element = mrMesh.GetElement(elem_index);
                    if (!element->IsDeleted())
                    {
                        c_vector<double, 2> drdt_contributions;
                        
                        double distance_between_nodes = fabs(element->GetNodeLocation(1,0) - element->GetNodeLocation(0,0));
                        double unit_vector_backward = -1;
                        double unit_vector_forward = 1;
                        
                        double time_scale = mpParams->GetStemCellCycleTime();
                        double age1 = mCells[element->GetNode(0)->GetIndex()].GetAge(time*time_scale);
                        double age2 = mCells[element->GetNode(1)->GetIndex()].GetAge(time*time_scale);
                        double rest_length=mpParams->GetNaturalSpringLength();
                        
                        if (age1<1.0/time_scale && age2<1.0/time_scale && fabs(age1-age2)<1e-6)
                        {
                        	/* Spring Rest Length Increases to 1 from 0.1 over 1 hour
                             * This doesnt happen at present as when the full line is included the tests fail
                             */
                            rest_length=0.1*rest_length+0.9*age1*time_scale;
                            assert(rest_length<=mpParams->GetNaturalSpringLength());
                        }
                        
                        drdt_contributions(0) = mpParams->GetAlpha() *(  unit_vector_forward  * (distance_between_nodes - rest_length) );
                        drdt_contributions(1) = mpParams->GetAlpha() *(  unit_vector_backward * (distance_between_nodes - rest_length) );
                        
                        drdt[ element->GetNode(0)->GetIndex() ] += drdt_contributions(0);
						drdt[ element->GetNode(1)->GetIndex() ] += drdt_contributions(1);
                    }
                }
            }
            else
            {
                for (int elem_index = 0; elem_index<mrMesh.GetNumAllElements(); elem_index++)
                {
                    Element<1,1>* element = mrMesh.GetElement(elem_index);
                    if (!element->IsDeleted())
                    {
                        c_vector<double, 2> drdt_contributions;
                        
                        double distance_between_nodes = fabs(element->GetNodeLocation(1,0) - element->GetNodeLocation(0,0));
                        double unit_vector_backward = -1;
                        double unit_vector_forward = 1;
                        
                        drdt_contributions(0) = mpParams->GetAlpha() *(  unit_vector_forward  * (distance_between_nodes - mpParams->GetNaturalSpringLength()) );
                        drdt_contributions(1) = mpParams->GetAlpha() *(  unit_vector_backward * (distance_between_nodes - mpParams->GetNaturalSpringLength()) );
                        
                        drdt[ element->GetNode(0)->GetIndex() ] += drdt_contributions(0);
                        drdt[ element->GetNode(1)->GetIndex() ] += drdt_contributions(1);
                    }
                }
            }
            
            // update node positions
            for (int index = 1; index<mrMesh.GetNumAllNodes(); index++)
            {
                // assume stem cells are fixed, or if there are no cells, fix node 0
                //if(
                if (!mrMesh.GetNodeAt(index)->IsDeleted())
                {
                    Point<1> old_point = mrMesh.GetNodeAt(index)->rGetPoint();
                    Point<1> new_point;
                    new_point.rGetLocation()[0] = old_point[0] + mDt*drdt[index]; // new_point_position[index];
                    mrMesh.SetNode(index, new_point, false);
                }
            }
            
            // Remove nodes that are beyond the crypt
            while (true)
            {
                bool sloughed_node = false;
                ConformingTetrahedralMesh<1,1>::BoundaryNodeIterator it = mrMesh.GetBoundaryNodeIteratorEnd();
                while (it != mrMesh.GetBoundaryNodeIteratorBegin())
                {
                    it--;
                    const Node<1> *p_node = *it;
                    if (p_node->rGetPoint()[0] > mpParams->GetCryptLength())
                    {
                        // It's fallen off
                        mrMesh.DeleteBoundaryNodeAt(p_node->GetIndex());
                        num_deaths++;
                        //std::cout<< "num_deaths=" << num_deaths <<std::endl<< std::flush;
                        sloughed_node = true;
                        break;
                    }
                }
                if (!sloughed_node) break;
            }
            // Check nodes havent crossed
            for (int elem_index = 0; elem_index<mrMesh.GetNumAllElements(); elem_index++)
            {
                Element<1,1>* element = mrMesh.GetElement(elem_index);
                if (!element->IsDeleted())
                {
                    assert(element->GetNodeLocation(1,0) - element->GetNodeLocation(0,0)>0);
                }
            }
            
            // write results to file
            (*p_results_file) << time << "\t";
            for (int index = 0; index<mrMesh.GetNumAllNodes(); index++)
            {
                if (!mrMesh.GetNodeAt(index)->IsDeleted())
                {
                    Point<1> point = mrMesh.GetNodeAt(index)->rGetPoint();
                    (*p_results_file) << point.rGetLocation()[0] << " ";
                }
            }
            (*p_results_file) << "\n";
            
            p_simulation_time->IncrementTimeOneStep();
            time = p_simulation_time->GetDimensionalisedTime()/mpParams->GetStemCellCycleTime();
            
            time_since_last_birth += mDt;
        }
        SimulationTime::Destroy();
    }
    
private:
    unsigned AddRandomNode(double time)
    {
    	    	
        //Pick an element
        int random_element_number = mpGen->randMod(mrMesh.GetNumAllElements());
        Element<1,1>* p_random_element = mrMesh.GetElement(random_element_number);
        double element_length = fabs(p_random_element->GetNodeLocation(1,0) - p_random_element->GetNodeLocation(0,0));
        //std::cout << "length " <<element_length << "\n";
        
        // keep picking until find an element which is big enough and not deleted
        while (element_length <0.4 || p_random_element->IsDeleted())
        {
            // the following is ignored in coverage as there is a random 
            // chance of it not happening
            #define COVERAGE_IGNORE
            random_element_number = mpGen->randMod(mrMesh.GetNumAllElements());
            p_random_element = mrMesh.GetElement(random_element_number);
            element_length = fabs(p_random_element->GetNodeLocation(1,0) - p_random_element->GetNodeLocation(0,0));
            #undef COVERAGE_IGNORE
            //std::cout << "..too small, trying: length " <<element_length << "\n";
            //double random_displacement = 0.2+mpGen->randf()*0.6;
        }
        // Reset age of left node to zero
        mCells[p_random_element->GetNode(0)->GetIndex()].SetBirthTime(time);
        return AddNodeToElement(p_random_element,time);
    }
    
    
    int AddNodeToElement(Element<1,1>* pElement, double time)
    {

        double displacement;
        double left_position= pElement->GetNodeLocation(0,0);
        if(mIncludeVariableRestLength)
        {
        	//double time_scale = mpParams->GetStemCellCycleTime();
        	//double time_scale = mpParams->GetStemCellCycleTime();
            double age0 = mCells[pElement->GetNode(0)->GetIndex()].GetAge(time);
            double age1 = mCells[pElement->GetNode(1)->GetIndex()].GetAge(time);
            
                       
            if (fabs(age0)<1e-6)
            {
                // place the new node to 0.1 to the right of the left-hand node
                displacement = 0.1;
            }
            else if (fabs(age1)<1e-6)
            {
            	// place the new node to 0.1 to the left of the right-hand node
            	double element_length = fabs(pElement->GetNodeLocation(1,0) - pElement->GetNodeLocation(0,0));
                displacement = element_length - 0.1;
            }
            else
            {
            	EXCEPTION("No cell has divided in this element");
            }
        }
        else
        {
        	double element_length = fabs(pElement->GetNodeLocation(1,0) - pElement->GetNodeLocation(0,0));
            // pick a random position in the central 60% of the element
            displacement = 0.2 + (mpGen->ranf())*(element_length-0.4);
         
        }
        Point<1> new_point(left_position + displacement);
        
        return mrMesh.RefineElement(pElement, new_point);
    }
    
};

#endif /*CRYPTSIMULATION_HPP_*/
