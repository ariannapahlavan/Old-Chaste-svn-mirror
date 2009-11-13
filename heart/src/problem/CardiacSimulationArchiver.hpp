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

#ifndef CARDIACSIMULATIONARCHIVER_HPP_
#define CARDIACSIMULATIONARCHIVER_HPP_

// Must be included before any other serialisation headers
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "OutputFileHandler.hpp"
#include "ArchiveOpener.hpp"
#include "ArchiveLocationInfo.hpp"

/**
 *  CardiacSimulationArchiver is a helper class for checkpointing of cardiac simulations.
 * 
 *  The class is templated over the class defining the simulation (i.e. MonodomainProblem) 
 */
template<class PROBLEM_CLASS>
class CardiacSimulationArchiver
{
public:
    /**
     *  Archives a simulation in the directory specified.
     * 
     *  @param simulationToArchive object defining the simulation to archive
     *  @param directory directory where the multiple files defining the checkpoint will be stored
     *  @param clearDirectory whether the directory needs to be cleared or not.
     */
    static void Save(PROBLEM_CLASS& simulationToArchive, std::string directory, bool clearDirectory=true);


    /**
     *  Unarchives a simulation from the directory specified
     * 
     *  @param directory directory where the multiple files defining the checkpoint are located
     */
    static PROBLEM_CLASS* Load(std::string directory);
    
     /**
     *  Archives a simulation in the directory specified.
     * 
     *  @param simulationToArchive object defining the simulation to archive
     *  @param directory directory where the multiple files defining the checkpoint will be stored
     *  @param clearDirectory whether the directory needs to be cleared or not.
     */
    static void SaveAsSequential(PROBLEM_CLASS& simulationToArchive, std::string directory, bool clearDirectory=true);
 
    /**
     * Take a parallel archive and convert it to a sequential one
     * @param inputDirectory
     * @param outputDirectory
     * @param clearDirectory whether the directory needs to be cleared or not.
     */
    static void MigrateToSequential(std::string inputDirectory, std::string outputDirectory, bool clearDirectory=true);
};


template<class PROBLEM_CLASS>
void CardiacSimulationArchiver<PROBLEM_CLASS>::Save(PROBLEM_CLASS& simulationToArchive, std::string directory, bool clearDirectory)
{
    // Clear directory if requested (and make sure it exists)
    OutputFileHandler handler(directory, clearDirectory);
    
    // Open the archive files
    ArchiveOpener<boost::archive::text_oarchive, std::ofstream> archive_opener(directory, directory + ".arch", true);
    boost::archive::text_oarchive* p_main_archive = archive_opener.GetCommonArchive();
    
    // And save
    PROBLEM_CLASS* const p_simulation_to_archive = &simulationToArchive;
    (*p_main_archive) & p_simulation_to_archive;
}

template<class PROBLEM_CLASS>
PROBLEM_CLASS* CardiacSimulationArchiver<PROBLEM_CLASS>::Load(std::string directory)
{
    // Open the archive files
    ArchiveOpener<boost::archive::text_iarchive, std::ifstream> archive_opener(directory, directory + ".arch", true);
    boost::archive::text_iarchive* p_main_archive = archive_opener.GetCommonArchive();

    // Load
    PROBLEM_CLASS *p_unarchived_simulation;
    (*p_main_archive) >> p_unarchived_simulation;

    return p_unarchived_simulation;
}

template<class PROBLEM_CLASS>
void CardiacSimulationArchiver<PROBLEM_CLASS>::SaveAsSequential(PROBLEM_CLASS& simulationToArchive, std::string directory, bool clearDirectory)
{
    // Clear directory if requested (and make sure it exists)
    OutputFileHandler handler(directory, clearDirectory);
    
    // Open the archive files
    ArchiveOpener<boost::archive::text_oarchive, std::ofstream> archive_opener(directory, directory + ".arch", true);
    boost::archive::text_oarchive* p_main_archive = archive_opener.GetCommonArchive();
    
    // And save
    ///\todo #1159
    PROBLEM_CLASS* const p_simulation_to_archive = &simulationToArchive;
    (*p_main_archive) & p_simulation_to_archive;
}

template<class PROBLEM_CLASS>
void CardiacSimulationArchiver<PROBLEM_CLASS>::MigrateToSequential(std::string inputDirectory, std::string outputDirectory, bool clearDirectory)
{
    //Assume that the archive has been written for the right number of processors
    if (PetscTools::IsSequential())
    {
        EXCEPTION("Archive doesn't need to be migrated since it is already sequential");
    }
    
    // Open the input archive files
    ArchiveOpener<boost::archive::text_iarchive, std::ifstream> archive_opener(inputDirectory, inputDirectory + ".arch");
    boost::archive::text_iarchive* p_main_archive = archive_opener.GetCommonArchive();

    // Load...
    PROBLEM_CLASS *p_unarchived_simulation;
    (*p_main_archive) >> p_unarchived_simulation;
    
    // ...and save
    SaveAsSequential(*p_unarchived_simulation, outputDirectory, clearDirectory);
    delete p_unarchived_simulation;
}

#endif /*CARDIACSIMULATIONARCHIVER_HPP_*/

