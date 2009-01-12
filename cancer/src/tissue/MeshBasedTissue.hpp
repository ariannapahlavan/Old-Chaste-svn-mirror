/*

Copyright (C) University of Oxford, 2008

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
#ifndef MESHBASEDTISSUE_HPP_
#define MESHBASEDTISSUE_HPP_

#include "AbstractCellCentreBasedTissue.hpp"
#include "MutableMesh.hpp"
#include "VoronoiTessellation.hpp"
#include "Exception.hpp"
#include "MeshArchiveInfo.hpp"

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>


/**
 * A facade class encapsulating a mesh-based 'tissue'
 *
 * Contains a group of cells and maintains the associations between cells and
 * nodes in the mesh.
 *
 */
template<unsigned DIM>
class MeshBasedTissue : public AbstractCellCentreBasedTissue<DIM>
{
    friend class TestMeshBasedTissue;
    
protected:

    /** Reference to the mesh. */
    MutableMesh<DIM, DIM>& mrMesh;

    /** 
     * Pointer to a Voronoi tessellation object. 
     * Used to calculate cell area and perimeter information if required.
     */
    VoronoiTessellation<DIM>* mpVoronoiTessellation;

    /**
     * Whether to delete the mesh when we are destroyed.
     * Needed if this tissue has been de-serialized.
     */
    bool mDeleteMesh;

    /**
     * Special springs that we want to keep track of for some reason.
     * Currently used to track cells in the process of dividing
     * (which are represented as two cells joined by a shorter spring).
     */
    std::set<std::set<TissueCell*> > mMarkedSprings;

    /** Whether to print out cell area and perimeter information. */
    bool mWriteVoronoiData;

    /** Whether to follow only the logged cell if writing Voronoi data. */
    bool mFollowLoggedCell;

    /** Whether to print out tissue areas. */
    bool mWriteTissueAreas;

    /** Results file for elements. */
    out_stream mpElementFile;

    /** Results file for Voronoi data. */
    out_stream mpVoronoiFile;

    /** Results file for tissue area data. */
    out_stream mpTissueAreasFile;

    /** Helper method used by the spring marking routines */
    std::set<TissueCell*> CreateCellPair(TissueCell&, TissueCell&);
    
    /** Whether to use a viscosity that is linear in the cell area, rather than constant. */
    bool mUseAreaBasedDampingConstant;

    friend class boost::serialization::access;
    /**
     * Serialize the facade.
     *
     * Note that serialization of the mesh and cells is handled by load/save_construct_data.
     *
     * Note also that member data related to writers is not saved - output must
     * be set up again by the caller after a restart.
     */
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
        archive & boost::serialization::base_object<AbstractCellCentreBasedTissue<DIM> >(*this);

        // The Voronoi stuff can't be archived yet
        //archive & mpVoronoiTessellation
        delete mpVoronoiTessellation;
        mpVoronoiTessellation = NULL;

        archive & mMarkedSprings;
        archive & mWriteVoronoiData;
        archive & mFollowLoggedCell;
        archive & mWriteTissueAreas;
        archive & mUseAreaBasedDampingConstant;

        // In its present form, a call to MeshBasedTissue::Validate() here
        // would result in a seg fault in the situation where we are actually loading
        // a MeshBasedTissueWithGhostNodes. Commenting out this call breaks no tests.

        // Validate();
    }

public:

    /**
     * Create a new tissue facade from a mesh and collection of cells.
     *
     * There must be precisely 1 cell for each node of the mesh.
     *
     * @param rMesh a mutable tetrahedral mesh
     * @param rCells TissueCells corresponding to the nodes of the mesh
     * @param deleteMesh set to true if you want the tissue to free the mesh memory on destruction
     * @param validate whether to validate the tissue
     */
    MeshBasedTissue(MutableMesh<DIM, DIM>& rMesh,
                    const std::vector<TissueCell>& rCells,
                    bool deleteMesh=false,
                    bool validate=true);

    /**
     * Constructor for use by the de-serializer.
     *
     * @param rMesh a mutable tetrahedral mesh.
     */
    MeshBasedTissue(MutableMesh<DIM, DIM>& rMesh);

    /**
     * Destructor.
     */
    ~MeshBasedTissue();

    /**
     * @return reference to  mrMesh.
     */
    MutableMesh<DIM, DIM>& rGetMesh();

    /**
     * @return const reference to mrMesh (used in archiving).
     */
    const MutableMesh<DIM, DIM>& rGetMesh() const;

    /** @return mWriteVoronoiData. */
    bool GetWriteVoronoiData();

    /** @return mWriteTissueAreas. */
    bool GetWriteTissueAreas();

    /** @return mUseAreaBasedDampingConstant. */
    bool UseAreaBasedDampingConstant();

    /** Get method for mWriteVoronoiData and mFollowLoggedCell.
     * 
     * @param writeVoronoiData  whether to output cell area and perimeter information
     * @param followLoggedCell  whether to follow only the logged cell if writing Voronoi data
     */   
    void SetWriteVoronoiData(bool writeVoronoiData, bool followLoggedCell);
    
    /**
     * Overridden AddNode() method.
     * 
     * Add a new node to the tissue.
     * 
     * @param pNewNode pointer to the new node 
     * @return global index of new node in tissue
     */
    unsigned AddNode(Node<DIM> *pNewNode);

    /**
     * Overridden SetNode() method.
     * 
     * Move the node with a given index to a new point in space.
     * 
     * @param nodeIndex the index of the node to be moved
     * @param rNewLocation the new target location of the node
     */
    void SetNode(unsigned nodeIndex, ChastePoint<DIM>& rNewLocation);

    /**
     * Overridden GetDampingConstant() method that includes the 
     * case of a cell-area-based damping constant.
     * 
     * @param nodeIndex the global index of this node
     * @return the damping constant for the given TissueCell.
     */ 
    double GetDampingConstant(unsigned nodeIndex);

    /** 
     * Set method for mWriteTissueAreas. 
     * 
     * @param   whether to output tissue area data
     */
    void SetWriteTissueAreas(bool writeTissueAreas);

    /** 
     * Set method for mUseAreaBasedDampingConstant. 
     * 
     * @param   whether to use a viscosity that is linear in the cell area, rather than constant
     */
    void SetAreaBasedDampingConstant(bool useAreaBasedDampingConstant);

    /**
     * Remove all cells that are labelled as dead.
     *
     * Note that this now calls MutableMesh::DeleteNodePriorToReMesh() 
     * and therefore a ReMesh(map) must be called before any element 
     * information is used.
     *
     * Note also that after calling this method the tissue will be in an inconsistent state until 
     * Update() is called! So don't try iterating over cells or anything like that.
     * 
     * \todo weaken the data invariant in this class so it doesn't require an exact correspondance
     * between nodes and cells (see #430) - most of the work will actually be in AbstractTissue.
     *
     * @return number of cells removed.
     */
    unsigned RemoveDeadCells();

    /**
     * Overridden WriteMeshToFile() method. For use by 
     * the TissueSimulationArchiver.
     * 
     * @param rArchiveDirectory directory in which archive is stored
     * @param rMeshFileName base name for mesh files
     */
    void WriteMeshToFile(const std::string &rArchiveDirectory, const std::string &rMeshFileName);
    
    /**
     * Overridden CreateOutputFiles() method.
     * 
     * @param rDirectory  pathname of the output directory, relative to where Chaste output is stored
     * @param rCleanOutputDirectory  whether to delete the contents of the output directory prior to output file creation
     * @param outputCellMutationStates  whether to create a cell mutation state results file
     * @param outputCellTypes  whether to create a cell type results file
     * @param outputCellVariables  whether to create a cell-cycle variable results file
     * @param outputCellCyclePhases  whether to create a cell-cycle phase results file
     * @param outputCellAncestors  whether to create a cell ancestor results file
     */
    void CreateOutputFiles(const std::string &rDirectory,
                           bool rCleanOutputDirectory,
                           bool outputCellMutationStates,
                           bool outputCellTypes,
                           bool outputCellVariables,
                           bool outputCellCyclePhases,
                           bool outputCellAncestors);
    /**
     * Overridden CloseOutputFiles() method.
     * 
     * @param outputCellMutationStates  whether a cell mutation state results file is open
     * @param outputCellTypes  whether a cell type results file is open
     * @param outputCellVariables  whether a cell-cycle variable results file is open
     * @param outputCellCyclePhases  whether a cell-cycle phase results file is open
     * @param outputCellAncestors  whether a cell ancestor results file is open
     */
    void CloseOutputFiles(bool outputCellMutationStates,
                          bool outputCellTypes,
                          bool outputCellVariables,
                          bool outputCellCyclePhases,
                          bool outputCellAncestors);
                          
    /**
     * Overridden WriteResultsToFiles() method.
     * 
     * @param outputCellMutationStates  whether to output cell mutation state results
     * @param outputCellTypes  whether to output cell type results
     * @param outputCellVariables  whether to output cell-cycle variable results
     * @param outputCellCyclePhases  whether to output cell-cycle phase results
     * @param outputCellAncestors  whether to output cell ancestor results
     */
    void WriteResultsToFiles(bool outputCellMutationStates,
                             bool outputCellTypes,
                             bool outputCellVariables,
                             bool outputCellCyclePhases,
                             bool outputCellAncestors);

    /**
     * Overridden Update() method. 
     * Fixes up the mappings between cells and nodes.
     */
    virtual void Update();

    /**
     * Overridden GetNode() method.
     * 
     * @param index  global index of the specified Node
     * 
     * @return pointer to the Node with given index.
     */
    Node<DIM>* GetNode(unsigned index);
    
    /**
     * Overridden GetNumNodes() method.
     * 
     * @return the number of nodes in the tissue.
     */
    unsigned GetNumNodes();

    /**
     * Sets the Ancestor index of all the cells at the bottom in order,
     * can be used to trace clonal populations.
     */
    void SetBottomCellAncestors();

    /**
     * Check consistency of our internal data structures. Each node must
     * have a cell associated with it.
     */
    virtual void Validate();

    /**
     * Write current results to mpVoronoiFile.
     */
    void WriteVoronoiResultsToFile();

    /**
     * Write current results to mpTissueAreasFile.
     */
    void WriteTissueAreaResultsToFile();

    /** Get a reference to a Voronoi tessellation of the mesh. */
    void CreateVoronoiTessellation();

    /**
     * @return mpVoronoiTessellation.
     */
    VoronoiTessellation<DIM>& rGetVoronoiTessellation();

    /**
     * Update mIsGhostNode if required by a remesh.
     */
    virtual void UpdateGhostNodesAfterReMesh(NodeMap& rMap);

    /**
     * Iterator over edges in the mesh, which correspond to springs between cells.
     *
     * This class takes care of the logic to make sure that you consider each edge exactly once.
     */
    class SpringIterator
    {
    public:

        /**
         * Get a pointer to the node in the mesh at end A of the spring.
         */
        Node<DIM>* GetNodeA();

        /**
         * Get a pointer to the node in the mesh at end B of the spring.
         */
        Node<DIM>* GetNodeB();

        /**
         * Get a *reference* to the cell at end A of the spring.
         */
        TissueCell& rGetCellA();

        /**
         * Get a *reference* to the cell at end B of the spring.
         */
        TissueCell& rGetCellB();

        /**
         * Comparison not-equal-to.
         */
        bool operator!=(const SpringIterator& other);

        /**
         * Prefix increment operator.
         */
        SpringIterator& operator++();

        /**
         * Constructor for a new iterator.
         */
        SpringIterator(MeshBasedTissue& rTissue, typename MutableMesh<DIM,DIM>::EdgeIterator edgeIter);

    private:

        /** Keep track of what edges have been visited. */
        std::set<std::set<unsigned> > mSpringsVisited;

        /** The tissue member. */
        MeshBasedTissue& mrTissue;

        /** The edge iterator member. */
        typename MutableMesh<DIM, DIM>::EdgeIterator mEdgeIter;
    };

    /**
     * @return iterator pointing to the first spring in the tissue
     */
    SpringIterator SpringsBegin();

    /**
     * @return iterator pointing to one past the last spring in the tissue
     */
    SpringIterator SpringsEnd();

    // For debugging
    void CheckTissueCellPointers();

    /**
     * @return whether the spring between two given cells is marked.
     */
    bool IsMarkedSpring(TissueCell&, TissueCell&);

    /**
     * Mark the spring between the given cells.
     */
    void MarkSpring(TissueCell&, TissueCell&);

    /**
     * Stop marking the spring between the given cells.
     */
    void UnmarkSpring(TissueCell&, TissueCell&);

};


#include "TemplatedExport.hpp"
EXPORT_TEMPLATE_CLASS_SAME_DIMS(MeshBasedTissue)

namespace boost
{
namespace serialization
{
/**
 * Serialize information required to construct a Tissue facade.
 */
template<class Archive, unsigned DIM>
inline void save_construct_data(
    Archive & ar, const MeshBasedTissue<DIM> * t, const BOOST_PFTO unsigned int file_version)
{
    // Save data required to construct instance
    const MutableMesh<DIM,DIM>* p_mesh = &(t->rGetMesh());
    ar & p_mesh;
}

/**
 * De-serialize constructor parameters and initialise Tissue.
 * Loads the mesh from separate files.
 */
template<class Archive, unsigned DIM>
inline void load_construct_data(
    Archive & ar, MeshBasedTissue<DIM> * t, const unsigned int file_version)
{
    // Retrieve data from archive required to construct new instance
    assert(MeshArchiveInfo::meshPathname.length() > 0);
    MutableMesh<DIM,DIM>* p_mesh;
    ar >> p_mesh;

    // Re-initialise the mesh
    p_mesh->Clear();
    TrianglesMeshReader<DIM,DIM> mesh_reader(MeshArchiveInfo::meshPathname);
    p_mesh->ConstructFromMeshReader(mesh_reader);

    // Needed for cylindrical meshes at present; should be safe in any case.
    NodeMap map(p_mesh->GetNumNodes());
    p_mesh->ReMesh(map);

    // Invoke inplace constructor to initialise instance
    ::new(t)MeshBasedTissue<DIM>(*p_mesh);
}
}
} // namespace ...

#endif /*MESHBASEDTISSUE_HPP_*/
