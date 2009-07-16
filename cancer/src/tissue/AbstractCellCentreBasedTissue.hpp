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
#ifndef ABSTRACTCELLCENTREBASEDTISSUE_HPP_
#define ABSTRACTCELLCENTREBASEDTISSUE_HPP_

#include "AbstractTissue.hpp"

/**
 * An abstract facade class encapsulating a cell-centre based tissue, in which
 * each cell corresponds to a Node.
 */
template<unsigned DIM>
class AbstractCellCentreBasedTissue : public AbstractTissue<DIM>
{
private:

    /** Needed for serialization. */
    friend class boost::serialization::access;
    /**
     * Serialize the facade.
     *
     * @param archive the archive
     * @param version the current version of this class
     */
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
        // If Archive is an output archive, then '&' resolves to '<<'
        // If Archive is an input archive, then '&' resolves to '>>'
        archive & boost::serialization::base_object<AbstractTissue<DIM> >(*this);
    }

public:

    /**
     * Default constructor.
     *
     * @param rCells a vector of cells
     * @param locationIndices an optional vector of location indices that correspond to real cells
     */
    AbstractCellCentreBasedTissue(const std::vector<TissueCell>& rCells,
                                  const std::vector<unsigned> locationIndices=std::vector<unsigned>());

    /**
     * Constructor for use by archiving - doesn't take in cells, since these are dealt
     * with by the serialize method.
     */
    AbstractCellCentreBasedTissue();

    /**
     * Overridden GetLocationOfCellCentre() method.
     * Find where a given cell is in space.
     *
     * @param pCell pointer to the cell
     *
     * @return the location of the node corresponding to this cell.
     */
    c_vector<double, DIM> GetLocationOfCellCentre(TissueCell* pCell);

    /**
     * Get a pointer to the node corresponding to a given cell.
     *
     * @param pCell pointer to the cell
     *
     * @return address of the node
     */
    Node<DIM>* GetNodeCorrespondingToCell(TissueCell* pCell);

    /**
     * Add a new cell to the tissue.
     *
     * @param rNewCell  the cell to add
     * @param newLocation  the position in space at which to put it
     * @param pParentCell pointer to a parent cell (if required)
     *
     * @returns address of cell as it appears in the cell list
     */
    TissueCell* AddCell(TissueCell& rNewCell, c_vector<double,DIM> newLocation, TissueCell* pParentCell=NULL);

    /**
     * Overridden IsCellAssociatedWithADeletedNode() method.
     *
     * @param rCell the cell
     *
     * @return whether a given cell is associated with a deleted node.
     */
    bool IsCellAssociatedWithADeletedNode(TissueCell& rCell);

    /**
     * Overridden UpdateNodeLocations() method.
     *
     * @param rNodeForces a vector containing the force on each node in the tissue
     * @param dt the time step
     */
    virtual void UpdateNodeLocations(const std::vector< c_vector<double, DIM> >& rNodeForces, double dt);

    /**
     * Overridden GetDampingConstant() method.
     *
     * Get the damping constant for the cell associated with this node,
     * i.e. d in drdt = F/d. This depends on whether using area-based
     * viscosity has been switched on, and on whether the cell is a mutant
     * or not.
     *
     * @param nodeIndex the global index of this node
     * @return the damping constant at the TissueCell associated with this node
     */
    virtual double GetDampingConstant(unsigned nodeIndex);

    /**
     * Write results from the current tissue state to output files.
     */
    virtual void WriteResultsToFiles();

};

#endif /*ABSTRACTCELLCENTREBASEDTISSUE_HPP_*/
