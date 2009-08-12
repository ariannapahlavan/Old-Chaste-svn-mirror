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

#include "UblasCustomFunctions.hpp"
#include "HeartConfig.hpp"
#include "PostProcessingWriter.hpp"
#include "PetscTools.hpp"
#include "OutputFileHandler.hpp"
#include <iostream>

PostProcessingWriter::PostProcessingWriter(std::string directory, std::string hdf5File, bool isAbsolute)
{
 //   mOutputDirectory = directory + "/output";
    mpDataReader = new Hdf5DataReader(directory, hdf5File, isAbsolute);
    mpCalculator = new PropagationPropertiesCalculator(mpDataReader);
    mNumberOfNodes = mpDataReader->GetNumberOfRows();
}

void PostProcessingWriter::WritePostProcessingFiles()
{
    if (HeartConfig::Instance()->IsApdMapsRequested()) 
    {
        std::vector<std::pair<double,double> > apd_maps;
        HeartConfig::Instance()->GetApdMaps(apd_maps);
        for (unsigned i=0; i<apd_maps.size(); i++) 
        {
            WriteApdMapFile(apd_maps[i].first, apd_maps[i].second);
        }
    }
    
    if (HeartConfig::Instance()->IsUpstrokeTimeMapsRequested()) 
    {
        std::vector<double> upstroke_time_maps;
        HeartConfig::Instance()->GetUpstrokeTimeMaps(upstroke_time_maps);
        for (unsigned i=0; i<upstroke_time_maps.size(); i++) 
        {
            WriteUpstrokeTimeMap(upstroke_time_maps[i]);
        }
    }
    
    if (HeartConfig::Instance()->IsMaxUpstrokeVelocityMapRequested()) 
    {
        std::vector<double> upstroke_velocity_maps;
        HeartConfig::Instance()->GetMaxUpstrokeVelocityMaps(upstroke_velocity_maps);
        for (unsigned i=0; i<upstroke_velocity_maps.size(); i++) 
        {
            WriteMaxUpstrokeVelocityMap(upstroke_velocity_maps[i]);
        }
    }

///// unfinished..
////    if (HeartConfig::Instance()->IsConductionVelocityMapsRequested()) 
////    {
////        std::vector<unsigned> conduction_velocity_maps;
////        HeartConfig::Instance()->GetConductionVelocityMaps(conduction_velocity_maps);
////        for (unsigned i=0; i<conduction_velocity_maps.size(); i++) 
////        {
////            WriteConductionVelocityMap(conduction_velocity_maps[i]);
////        }
////    }
} 
    
    
    
PostProcessingWriter::~PostProcessingWriter()
{
    delete mpDataReader;
    delete mpCalculator;
}


void PostProcessingWriter::WriteApdMapFile(double repolarisationPercentage, double threshold)
{
    
    if(PetscTools::AmMaster())
    {
        out_stream p_file=out_stream(NULL);
        std::stringstream stream;
        stream << "Apd_" << repolarisationPercentage << "_" << threshold << "_Map.dat";
        OutputFileHandler output_file_handler(HeartConfig::Instance()->GetOutputDirectory() + "/output", false);
        p_file = output_file_handler.OpenOutputFile(stream.str());
        for (unsigned node_index = 0; node_index < mNumberOfNodes; node_index++)
        { 
            std::vector<double> apds;
            try
            {
                apds = mpCalculator->CalculateAllActionPotentialDurations(repolarisationPercentage, node_index, threshold);
                assert(apds.size() != 0);
            }
            catch(Exception& e)
            {                    
                apds.push_back(0);
                assert(apds.size() == 1);
            }
            for (unsigned i = 0; i < apds.size(); i++)
            {
                *p_file << apds[i] << "\t";
            }
            *p_file << std::endl;
        }
        p_file->close();
    }
}


void PostProcessingWriter::WriteUpstrokeTimeMap(double threshold)
{    
    if(PetscTools::AmMaster())
    {
        out_stream p_file=out_stream(NULL);
        std::stringstream stream;
        stream << "UpstrokeTimeMap_" << threshold << ".dat";
        OutputFileHandler output_file_handler(HeartConfig::Instance()->GetOutputDirectory() + "/output", false);
        p_file = output_file_handler.OpenOutputFile(stream.str());
        for (unsigned node_index = 0; node_index < mNumberOfNodes; node_index++)
        { 
            std::vector<double> upstroke_times;
            upstroke_times = mpCalculator->CalculateUpstrokeTimes(node_index, threshold);
            for (unsigned i = 0; i < upstroke_times.size(); i++)
            {
                *p_file << upstroke_times[i] << "\t";
            }
            *p_file << std::endl;
        }
        p_file->close();
    }
}

void PostProcessingWriter::WriteMaxUpstrokeVelocityMap(double threshold)
{
    if(PetscTools::AmMaster())
    {
        out_stream p_file=out_stream(NULL);
        std::stringstream stream;
        stream << "MaxUpstrokeVelocityMap_" << threshold << ".dat";
        OutputFileHandler output_file_handler(HeartConfig::Instance()->GetOutputDirectory() + "/output", false);
        p_file = output_file_handler.OpenOutputFile(stream.str());
        for (unsigned node_index = 0; node_index < mNumberOfNodes; node_index++)
        { 
            std::vector<double> upstroke_velocities;
            upstroke_velocities = mpCalculator->CalculateAllMaximumUpstrokeVelocities(node_index, threshold);
            for (unsigned i = 0; i < upstroke_velocities.size(); i++)
            {
                *p_file << upstroke_velocities[i] << "\t";
            }
            *p_file << std::endl;
         }
         p_file->close();
    }
}

void PostProcessingWriter::WriteConductionVelocityMap(unsigned originNode, std::vector<double> distancesFromOriginNode)
{
    if(PetscTools::AmMaster())
    {
        out_stream p_file=out_stream(NULL);
        OutputFileHandler output_file_handler(HeartConfig::Instance()->GetOutputDirectory() + "/output", false);
        
        std::stringstream filename;
        filename << "ConductionVelocityFromNode" << originNode << ".dat";               
        p_file = output_file_handler.OpenOutputFile(filename.str());
        for (unsigned dest_node = 0; dest_node < mNumberOfNodes; dest_node++)
        { 
            std::vector<double> conduction_velocities;
            conduction_velocities = mpCalculator->CalculateAllConductionVelocities(originNode, dest_node, distancesFromOriginNode[dest_node]);
            assert(conduction_velocities.size()!=0);
            for (unsigned i = 0; i < conduction_velocities.size(); i++)
            {
                *p_file << conduction_velocities[i] << "\t";
            }
            *p_file << std::endl;
         }
         p_file->close();
    }        
}

