#ifndef TISSUESIMULATION_HPP_
#define TISSUESIMULATION_HPP_

#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp> // for archiving std::vector
#include <boost/serialization/set.hpp> // for archiving std::set
#include <boost/serialization/string.hpp>

#include "ColumnDataWriter.hpp"
#include "TissueCell.hpp"
#include "ConformingTetrahedralMesh.cpp"
#include "CancerParameters.hpp"
#include "RandomCellKiller.hpp"
#include "TrianglesMeshReader.cpp"
#include "Tissue.cpp"
#include "CryptVoronoiDataWriter.hpp"
#include "WntGradient.hpp"
#include <vector>


/**
 * Run a 2D or 3D tissue simulation, currently based on the Meineke Paper 
 * (doi:10.1046/j.0960-7722.2001.00216.x)
 * 
 * Cells are represented by their centres in space, they are connected by
 * springs defined by the cells' Delaunay/Voronoi tesselation.
 * 
 * The spring lengths are governed by the equations
 * dr/dt = stem_cycle_time*(mu/eta) sum_j r_hat_i,j*(|r_i,j|-s0)
 *       = alpha sum_j r_hat_i,j*(|r_i,j|-s0)
 *
 * where alpha = stem_cycle_time*(mu/eta) = stem_cycle_time*meineke_lambda.
 *       s0    = natural length of the spring.

 * Length is scaled by natural length.
 * Time is in hours.
 *
 * meineke_lambda = mu (spring constant) / eta (damping) = 0.01 (from Meineke - note
 * that the value we use for Meineke lambda is completely different because we have
 * nondimensionalised)
 *
 * The TissueSimulation currently only accepts a tissue (facade class) which is 
 * formed from a mesh, whose nodes are associated with TissueCells 
 * or are ghost nodes. The TissueSimulation then accesses only the 
 * TissueCells via an iterator in the tissue facade class.
 * 
 * The mesh should be surrounded by at least one layer of ghost nodes.  These are 
 * nodes which do not correspond to a cell, but are necessary for remeshing (because 
 * the remesher tries to create a convex hull of the set of nodes) and visualising 
 * purposes. The tissue class deals with ghost nodes. SetGhostNodes() should have been called
 * on it.
 * 
 * Cells can divide (at a time governed by their cell cycle models)
 * 
 * Cells can die - at a time/position specified by cell killers which can be 
 * added to the simulation.
 * 
 * \todo Move the spring calculations into a separate class?
 */
template<unsigned DIM>  
class TissueSimulation
{
    // Allow tests to access private members, in order to test computation of
    // private functions eg. DoCellBirth
    friend class TestCryptSimulation2d;
    friend class TestSprings3d;
private:

    std::set<std::set <TissueCell *> > mDivisionPairs;

protected:
    /** TimeStep */
    double mDt;
    
    /** Time to run the Solve() method up to */
    double mEndTime;

    /** Facade encapsulating cells in the tissue being simulated */
    Tissue<DIM>& mrTissue;
    /** Whether to delete the facade in our destructor */
    bool mDeleteTissue;
    
    /** Whether to run the simulation with no birth (defaults to false). */
    bool mNoBirth;
    
    /** Whether to remesh at each timestep or not (defaults to true).*/
    bool mReMesh;
    
    /** Whether to count the number of each cell type and output to file*/
    bool mOutputCellTypes;
    
    /** The maximum number of cells that this simulation will include (for use by datawriter). */
    unsigned mMaxCells;
    /** The maximum number of elements that this simulation will include (for use by datawriter). */
    unsigned mMaxElements;
    
    /** Output directory (a subfolder of tmp/<USERNAME>/testoutput) */
    std::string mOutputDirectory;
    
    /** The Meineke and cancer parameters */
    CancerParameters *mpParams;
    
    /** The singleton RandomNumberGenerator */
    RandomNumberGenerator *mpRandomGenerator;
    
    /** Whether to have zero force if the cells are far enough apart */
    bool mUseCutoffPoint;
    /** Have zero force if the cells are this distance apart (and mUseCutoffPoint==true) */
    double mCutoffPoint;

    /** Counts the number of births during the simulation */
    unsigned mNumBirths;
    
    /** Counts the number of deaths during the simulation */
    unsigned mNumDeaths;
    
    /** List of cell killers */
    std::vector<AbstractCellKiller<DIM>*> mCellKillers;
    
    /** Whether to use spring constant proportional to cell-cell contact length/area (defaults to false) */
    bool mUseEdgeBasedSpringConstant;
    
    /** Whether to do a voronoi tessellation every time step */
    bool mCreateVoronoiTessellation;

    /** Whether to print out cell area and perimeter info */
    bool mWriteVoronoiData;
    
    /** Whether to follow only the logged cell if writing voronoi data */
    bool mFollowLoggedCell;
    
#define COVERAGE_IGNORE
    /**
     * A helpful method for debugging.
     */
    void CheckDivisionPairPointers()
    {
        bool res = true;
        for (std::set<std::set<TissueCell*> >::iterator it1 = mDivisionPairs.begin();
             it1 != mDivisionPairs.end();
             ++it1)
        {
            const std::set<TissueCell*>& r_pair = *it1;
            assert(r_pair.size() == 2);
            for (std::set<TissueCell*>::iterator it2 = r_pair.begin();
                 it2 != r_pair.end();
                 ++it2)
            {
                TissueCell* p_cell = *it2;
                assert(p_cell);
                AbstractCellCycleModel *p_model = p_cell->GetCellCycleModel();
                assert(p_model);
                // Check cell exists in tissue
                unsigned node_index = p_cell->GetNodeIndex();
                std::cout << "Cell at node " << node_index << " addr " << p_cell << std::endl << std::flush;
                TissueCell& r_cell = mrTissue.rGetCellAtNodeIndex(node_index);
                if (&r_cell != p_cell)
                {
                    std::cout << "  Mismatch with tissue" << std::endl << std::flush;
                    res = false;
                }
                // Check model links back to cell
                if (p_model->GetCell() != p_cell)
                {
                    std::cout << "  Mismatch with cycle model" << std::endl << std::flush;
                    res = false;
                }
            }
        }
        assert(res);
    }
#undef COVERAGE_IGNORE
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
        mpParams = CancerParameters::Instance();
        archive & *mpParams;
        archive & mpParams;
        
        mpRandomGenerator = RandomNumberGenerator::Instance();
        archive & *mpRandomGenerator;
        archive & mpRandomGenerator;
                
        // If Archive is an output archive, then & resolves to <<
        // If Archive is an input archive, then & resolves to >>
        archive & mDt;
        archive & mEndTime;
        archive & mNoBirth;
        archive & mReMesh;
        archive & mMaxCells;
        archive & mMaxElements;
        archive & mOutputDirectory;
        archive & mNumBirths;
        archive & mNumDeaths;
        archive & mCellKillers;
        archive & mUseCutoffPoint;
        archive & mCutoffPoint;
        archive & mOutputCellTypes;
        archive & mDivisionPairs;
        //CheckDivisionPairPointers();
        archive & mUseEdgeBasedSpringConstant;
        archive & mCreateVoronoiTessellation;
        archive & mWriteVoronoiData;
        archive & mFollowLoggedCell;
    }
    
    /**
     * Writes out special information about the mesh to the visualizer.
     */
    void WriteVisualizerSetupFile(std::ofstream& rSetupFile);

    /**
     * During a simulation time step, process any cell divisions that need to occur.
     * If the simulation includes cell birth, causes (almost) all cells that are ready to divide
     * to produce daughter cells.
     *
     * @return the number of births that occurred.
     */
    unsigned DoCellBirth();
    
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
    virtual c_vector<double, DIM> CalculateDividingCellCentreLocations(typename Tissue<DIM>::Iterator parentCell);
    
    /**
     * During a simulation time step, process any cell sloughing or death
     *
     * This uses the cell killers to remove cells and associated nodes from the
     * facade class.
     * 
     * @return the number of deaths that occurred.
     */ 
    unsigned DoCellRemoval();
   
    /**
     * Calculates the forces on each node
     *
     * @return drdt the force components on each node
     */
    virtual std::vector<c_vector<double, DIM> > CalculateVelocitiesOfEachNode();
    
    /**
     * Calculates the force between two nodes.
     * 
     * Note that this assumes they are connected and is called by CalculateVelocitiesOfEachNode()
     * 
     * @param NodeAGlobalIndex
     * @param NodeBGlobalIndex
     * 
     * @return The force exerted on Node A by Node B.
     */
    virtual c_vector<double, DIM> CalculateForceBetweenNodes(unsigned nodeAGlobalIndex,unsigned nodeBGlobalIndex);
    
    /**
     * Moves each node to a new position for this timestep
     *
     * @param rDrDt the x and y force components on each node.
     */
    virtual void UpdateNodePositions(const std::vector< c_vector<double, DIM> >& rDrDt);
    
    /** 
     *  A method for subclasses to do something at the end of each timestep
     */
    virtual void PostSolve()
    {
    }
    
    
public:

    /** 
     *  Constructor
     * 
     *  @param rTissue A tissue facade class (contains a mesh and cells)
     *  @param deleteTissue whether to delete the tissue on destruction to free up memory.
     */
    TissueSimulation(Tissue<DIM>& rTissue, bool deleteTissue=false);
    
    /**
     * Free any memory allocated by the constructor
     */                         
    virtual ~TissueSimulation();
    
    void SetDt(double dt);
    double GetDt();
    void SetEndTime(double endTime);
    void SetOutputDirectory(std::string outputDirectory);
    void SetMaxCells(unsigned maxCells);
    void SetMaxElements(unsigned maxElements);
    void SetReMeshRule(bool remesh);
    void SetNoBirth(bool nobirth);
    void SetOutputCellTypes(bool outputCellTypes);
    void SetEdgeBasedSpringConstant(bool useEdgeBasedSpringConstant);
    void SetWriteVoronoiData(bool writeVoronoiData, bool followLoggedCell);
    void AddCellKiller(AbstractCellKiller<DIM>* pCellKiller);
    std::vector<double> GetNodeLocation(const unsigned& rNodeIndex);    
    void UseCutoffPoint(double cutoffPoint);
    c_vector<unsigned,5> GetCellTypeCount();

    void Solve();
    
    void Save();
    static TissueSimulation<DIM>* Load(const std::string& rArchiveDirectory, const double& rTimeStamp);

    Tissue<DIM>& rGetTissue();
    const Tissue<DIM>& rGetTissue() const;
    
};


namespace boost
{
namespace serialization
{
/**
 * Serialize information required to construct a TissueSimulation.
 */
template<class Archive, unsigned DIM>
inline void save_construct_data(
    Archive & ar, const TissueSimulation<DIM> * t, const BOOST_PFTO unsigned int file_version)
{
    // save data required to construct instance
    const Tissue<DIM> * p_tissue = &(t->rGetTissue());
    ar & p_tissue;
    
    bool archive_wnt;
    archive_wnt=WntGradient::Instance()->IsGradientSetUp();
    ar & archive_wnt;
    if (archive_wnt)
    {
        WntGradient* p_wnt_gradient = WntGradient::Instance();
        ar & *p_wnt_gradient;
        ar & p_wnt_gradient;
    }
}

/**
 * De-serialize constructor parameters and initialise Tissue.
 */
template<class Archive, unsigned DIM>
inline void load_construct_data(
    Archive & ar, TissueSimulation<DIM> * t, const unsigned int file_version)
{
    // retrieve data from archive required to construct new instance
    Tissue<DIM>* p_tissue;

    ar >> p_tissue;
    bool archive_wnt;
    ar & archive_wnt;
    if (archive_wnt)
    {
        WntGradient* p_wnt_gradient = WntGradient::Instance();
        ar & *p_wnt_gradient;
        ar & p_wnt_gradient;
    }
    // invoke inplace constructor to initialize instance
    ::new(t)TissueSimulation<DIM>(*p_tissue, true);
}
}
} // namespace ...


#endif /*TISSUESIMULATION_HPP_*/
