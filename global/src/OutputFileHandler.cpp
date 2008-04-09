/*
Copyright (C) Oxford University 2008

This file is part of CHASTE.

CHASTE is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

CHASTE is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with CHASTE.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "OutputFileHandler.hpp"

#include <cstdlib>
#include <sys/stat.h>

#ifndef SPECIAL_SERIAL
#include <petsc.h>
#endif //SPECIAL_SERIAL
#include "Exception.hpp"


OutputFileHandler::OutputFileHandler(const std::string &rDirectory,
                                     bool rCleanOutputDirectory)
{
    // Are we the master process?  Only the master should do any writing to disk
#ifndef SPECIAL_SERIAL
    PetscTruth is_there;
    PetscInitialized(&is_there);
    if (is_there == PETSC_TRUE)
    {
        PetscInt my_rank;
        MPI_Comm_rank(PETSC_COMM_WORLD, &my_rank);
        if (my_rank==0)
        {
            mAmMaster=true;
        }
        else
        {
            mAmMaster=false;
        }
    }
    else
#endif //SPECIAL_SERIAL
    {
        // Not using PETSc, so we're definitely the only process
        mAmMaster = true;
    }
    mDirectory = GetOutputDirectoryFullPath(rDirectory);
    
    // Clean the output dir?
    if (rCleanOutputDirectory && mAmMaster &&
        rDirectory != "" && rDirectory.find("..") == std::string::npos)
    {
        std::string directory_to_move_to = GetOutputDirectoryFullPath("last_cleaned_directory");
        system(("rm -rf " + directory_to_move_to).c_str());
        // Re-create the special directory
        mkdir(directory_to_move_to.c_str(), 0775);
        system(("mv " + mDirectory + " " + directory_to_move_to).c_str());
        //system(("rm -rf " + mDirectory).c_str());
        // Re-create the output directory
        mkdir(mDirectory.c_str(), 0775);
    }
}

std::string OutputFileHandler::GetChasteTestOutputDirectory()
{
    char *chaste_test_output = getenv("CHASTE_TEST_OUTPUT");
    std::string directory_root;
    if (chaste_test_output == NULL || *chaste_test_output == 0)
    {
        // Default to within the current directory
        directory_root = "./";
    }
    else
    {
        directory_root = std::string(chaste_test_output);
        // Add a trailing slash if not already there
        if (! ( *(directory_root.end()-1) == '/'))
        {
            directory_root = directory_root + "/";
        }
    }
    
    // TEMPORARY HACK for DTC chaste day -need to not write to /tmp/
    directory_root = "./results/";
    
    return directory_root;
}
    

std::string OutputFileHandler::GetOutputDirectoryFullPath(std::string directory)
{
    std::string directory_root = GetChasteTestOutputDirectory();
    directory = directory_root + directory;
    // Make sure it exists (ish)
    if (mAmMaster)
    {
        system(("mkdir -p " + directory).c_str());
    }
    
    // Add a trailing slash if not already there
    if (! ( *(directory.end()-1) == '/'))
    {
        directory = directory + "/";
    }
    return directory;
}


std::string OutputFileHandler::GetOutputDirectoryFullPath()
{
    return mDirectory;
}


out_stream OutputFileHandler::OpenOutputFile(std::string filename,
                                             std::ios_base::openmode mode)
{
    out_stream p_output_file(new std::ofstream((mDirectory+filename).c_str(), mode));
    if (!p_output_file->is_open())
    {
        EXCEPTION("Could not open file " + filename + " in " + mDirectory);
    }
    return p_output_file;
}


out_stream OutputFileHandler::OpenOutputFile(std::string fileName,
                                             unsigned number,
                                             std::string fileFormat,
                                             std::ios_base::openmode mode)
{
    std::stringstream string_stream;
    string_stream << fileName << number << fileFormat;
    return OpenOutputFile(string_stream.str(), mode);
}                                              

bool OutputFileHandler::IsMaster()
{
    return mAmMaster;
}
