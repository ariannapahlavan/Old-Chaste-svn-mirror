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

#ifndef MIXEDDIMENSIONMESH_HPP_
#define MIXEDDIMENSIONMESH_HPP_

#include "DistributedTetrahedralMesh.hpp"
#include "AbstractMeshReader.hpp"

#include "ChasteSerialization.hpp"
#include <boost/serialization/base_object.hpp>


/**
 * A tetrahedral mesh that also supports embedded 1D cable elements.
 *
 * Could be used for Purkinje or blood vessels, etc.
 */
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
class MixedDimensionMesh : public DistributedTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>
{
public:

    /**
     * Constructor.
     *
     * @param partitioningMethod  defaults to METIS_LIBRARY, but in 1-D is always overridden in this constructor to be the DUMB partition
     */
    MixedDimensionMesh(DistributedTetrahedralMeshPartitionType::type partitioningMethod=DistributedTetrahedralMeshPartitionType::METIS_LIBRARY);

    /**
     * Destructor - cleans up the cables
     *
     */
    ~MixedDimensionMesh();

    /**
     * Construct the mesh using a MeshReader.
     *
     * @param rMeshReader the mesh reader
     */
    void ConstructFromMeshReader(AbstractMeshReader<ELEMENT_DIM,SPACE_DIM>& rMeshReader);

   /**
     * Add the most recently constructed cable element to the global->local cable element mapping
     *
     * @param index is the global index of cable element to be registered
     */
    void RegisterCableElement(unsigned index);

    /**
     * Get the number of cable elements.
     */
    unsigned GetNumCableElements() const;

    /**
     * Get the number of cable elements on this process.
     */
    unsigned GetNumLocalCableElements() const;

    /**
     * Get the cable element with a given index in the mesh.
     *
     * @param globalElementIndex the global index of the cable element
     * @return a pointer to the cable element.
     */
    Element<1u, SPACE_DIM>* GetCableElement(unsigned globalElementIndex) const;

    /**
     * Determine whether or not the current process owns node 0 of this cable element (tie breaker to determine which process writes
     * to file for when two or more share ownership of a cable element).
     *
     * @param globalElementIndex is the global index of the cable element
     */
     bool CalculateDesignatedOwnershipOfCableElement( unsigned globalElementIndex );


private:
    /** The elements making up the 1D cables */
    std::vector<Element<1u, SPACE_DIM>*> mCableElements;
    /** The global number of cables over all processes*/
    unsigned mNumCableElements;
    /** A map from global cable index to local index used by this process. */
    std::map<unsigned, unsigned> mCableElementsMapping;

    /** Needed for serialization.*/
    friend class boost::serialization::access;
    /**
     * Serialize the mesh.
     *
     * @param archive the archive
     * @param version the current version of this class
     */
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
        archive & boost::serialization::base_object<DistributedTetrahedralMesh<ELEMENT_DIM, SPACE_DIM> >(*this);
    }

public:

    //////////////////////////////////////////////////////////////////////
    //                            Iterators                             //
    //////////////////////////////////////////////////////////////////////

    /** Definition of cable element Iterator type. */
    typedef typename std::vector<Element<1, SPACE_DIM> *>::const_iterator CableElementIterator;

        /**
     * Return a pointer to the first boundary element in the mesh.
     */
    CableElementIterator GetCableElementIteratorBegin() const;

    /**
     * Return a pointer to *one past* the last boundary element in the mesh
     * (for consistency with STL iterators).
     */
    CableElementIterator GetCableElementIteratorEnd() const;


};


#include "SerializationExportWrapper.hpp"
EXPORT_TEMPLATE_CLASS_ALL_DIMS(MixedDimensionMesh)

namespace boost
{
namespace serialization
{
/**
 * Record number of processors when saving...
 */
template<class Archive, unsigned ELEMENT_DIM, unsigned SPACE_DIM>
inline void save_construct_data(
    Archive & ar, const MixedDimensionMesh<ELEMENT_DIM, SPACE_DIM> * t, const BOOST_PFTO unsigned int file_version)
{
    unsigned num_procs = PetscTools::GetNumProcs();
    const DistributedTetrahedralMeshPartitionType::type partition_type = t->GetPartitionType();
    ar << num_procs;
    ar << partition_type;
}

/**
 * De-serialize constructor parameters and initialise a MixedDimensionMesh,
 * checking the number of processors is the same.
 */
template<class Archive, unsigned ELEMENT_DIM, unsigned SPACE_DIM>
inline void load_construct_data(
    Archive & ar, MixedDimensionMesh<ELEMENT_DIM, SPACE_DIM> * t, const unsigned int file_version)
{
    unsigned num_procs;
    DistributedTetrahedralMeshPartitionType::type partition_type;

    ar >> num_procs;
    ar >> partition_type;

    // Invoke inplace constructor to initialise instance
    /// \todo #1199  Lots of stuff can't cope if we re-partition
    //::new(t)MixedDimensionMesh<ELEMENT_DIM, SPACE_DIM>(partition_type);
    ::new(t)MixedDimensionMesh<ELEMENT_DIM, SPACE_DIM>(DistributedTetrahedralMeshPartitionType::DUMB);

    /*
     * The exception needs to be thrown after the call to ::new(t), or Boost will try
     * to free non-allocated memory when the exception is thrown.
     */
    if (DistributedVectorFactory::CheckNumberOfProcessesOnLoad() &&
        num_procs != PetscTools::GetNumProcs())
    {
        ///\todo #1898
        NEVER_REACHED;
//        EXCEPTION("This archive was written for a different number of processors");
    }

}
}
} // namespace ...

#endif /*MIXEDDIMENSIONMESH_HPP_*/
