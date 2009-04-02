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


#ifndef HEARTCONFIG_HPP_
#define HEARTCONFIG_HPP_

#include "UblasCustomFunctions.hpp"
#include "ChasteParameters.hpp"
#include <iostream>
#include "Exception.hpp"
#include <vector>
#include "AbstractStimulusFunction.hpp"
#include "SimpleStimulus.hpp"
#include "ChasteCuboid.hpp"
#include "OutputFileHandler.hpp"

class HeartConfig
{
private:
    void CheckTimeSteps() const;

public:
    /**
     * Call this method to access the global parameters holder.
     *
     * @return a single instance of the class
     */
    static HeartConfig* Instance();

    void SetDefaultsFile(std::string fileName);
    void SetParametersFile(std::string fileName);
    void Write(std::string dirName, std::string fileName);
    static void Reset();

    /*
     *  Get methods
     */
    // Simulation
    unsigned GetSpaceDimension() const;
    double GetSimulationDuration() const;
    domain_type GetDomain() const;
    ionic_models_available_type GetDefaultIonicModel() const;
    void GetIonicModelRegions(std::vector<ChasteCuboid>& definedRegions,
                              std::vector<ionic_models_available_type>& ionicModels) const;


    bool GetIsMeshProvided() const;
    bool GetCreateMesh() const;
    bool GetCreateSlab() const;
    bool GetCreateSheet() const;
    bool GetCreateFibre() const;
    bool GetLoadMesh() const;

    void GetSlabDimensions(c_vector<double, 3>& slabDimensions) const;
    void GetSheetDimensions(c_vector<double, 2>& sheetDimensions) const;
    void GetFibreLength(c_vector<double, 1>& fibreLength) const;
    double GetInterNodeSpace() const;

    std::string GetMeshName() const;
    media_type GetConductivityMedia() const;

    void GetStimuli(std::vector<SimpleStimulus>& stimuliApplied, std::vector<ChasteCuboid>& stimulatedAreas) const;
    void GetCellHeterogeneities(std::vector<ChasteCuboid>& cellHeterogeneityAreas,
                                std::vector<double>& scaleFactorGks,
                                std::vector<double>& scaleFactorIto,
                                std::vector<double>& scaleFactorGkr) const;
    bool GetConductivityHeterogeneitiesProvided() const;
    void GetConductivityHeterogeneities(std::vector<ChasteCuboid>& conductivitiesHeterogeneityAreas,
                                        std::vector< c_vector<double,3> >& intraConductivities,
                                        std::vector< c_vector<double,3> >& extraConductivities) const;
    std::string GetOutputDirectory() const;
    std::string GetOutputFilenamePrefix() const;

    // Physiological
    void GetIntracellularConductivities(c_vector<double, 3>& intraConductivities) const;
    void GetIntracellularConductivities(c_vector<double, 2>& intraConductivities) const;
    void GetIntracellularConductivities(c_vector<double, 1>& intraConductivities) const;

    void GetExtracellularConductivities(c_vector<double, 3>& extraConductivities) const;
    void GetExtracellularConductivities(c_vector<double, 2>& extraConductivities) const;
    void GetExtracellularConductivities(c_vector<double, 1>& extraConductivities) const;

    double GetBathConductivity() const;

    double GetSurfaceAreaToVolumeRatio() const;
    double GetCapacitance() const;

    // Numerical
    double GetOdeTimeStep() const;
    double GetPdeTimeStep() const;
    double GetPrintingTimeStep() const;

    bool GetUseAbsoluteTolerance() const;
    double GetAbsoluteTolerance() const;

    bool GetUseRelativeTolerance() const;
    double GetRelativeTolerance() const;

    const char* GetKSPSolver() const;
    const char* GetKSPPreconditioner() const;


    /*
     *  Set methods
     */
    // Simulation
    void SetSpaceDimension(unsigned spaceDimension);
    void SetSimulationDuration(double simulationDuration);
    void SetDomain(domain_type domain);
    void SetDefaultIonicModel(ionic_models_available_type ionicModel);

    void SetSlabDimensions(double x, double y, double z, double inter_node_space);
    void SetSheetDimensions(double x, double y, double inter_node_space);
    void SetFibreLength(double x, double inter_node_space);

    void SetMeshFileName(std::string meshPrefix, media_type fibreDefinition=media_type::NoFibreOrientation);
    void SetConductivityHeterogeneities(std::vector< c_vector<double,3> >& cornerA,
                                        std::vector< c_vector<double,3> >& cornerB,
                                        std::vector< c_vector<double,3> >& intraConductivities,
                                        std::vector< c_vector<double,3> >& extraConductivities);

    void SetOutputDirectory(std::string outputDirectory);
    void SetOutputFilenamePrefix(std::string outputFilenamePrefix);

    // Physiological
    void SetIntracellularConductivities(const c_vector<double, 3>& intraConductivities);
    void SetIntracellularConductivities(const c_vector<double, 2>& intraConductivities);
    void SetIntracellularConductivities(const c_vector<double, 1>& intraConductivities);

    void SetExtracellularConductivities(const c_vector<double, 3>& extraConductivities);
    void SetExtracellularConductivities(const c_vector<double, 2>& extraConductivities);
    void SetExtracellularConductivities(const c_vector<double, 1>& extraConductivities);

    void SetBathConductivity(double bathConductivity);

    void SetSurfaceAreaToVolumeRatio(double ratio);
    void SetCapacitance(double capacitance);

    // Numerical
    void SetOdePdeAndPrintingTimeSteps(double odeTimeStep, double pdeTimeStep, double printingTimeStep);
    void SetOdeTimeStep(double odeTimeStep);
    void SetPdeTimeStep(double pdeTimeStep);
    void SetPrintingTimeStep(double printingTimeStep);

    void SetUseRelativeTolerance(double relativeTolerance);
    void SetUseAbsoluteTolerance(double absoluteTolerance);

    void SetKSPSolver(const char* kspSolver);
    void SetKSPPreconditioner(const char* kspPreconditioner);

    ~HeartConfig();
protected:
    // Only to be accesed by the tests
    friend class TestHeartConfig;

    chaste_parameters_type* UserParameters();
    chaste_parameters_type* DefaultParameters();


private:
    HeartConfig();

    chaste_parameters_type* mpUserParameters;
    chaste_parameters_type* mpDefaultParameters;

    /** The single instance of the class */
    static std::auto_ptr<HeartConfig> mpInstance;

    // Misc
    template<class TYPE>
    TYPE* DecideLocation(TYPE* ptr1, TYPE* ptr2, const std::string& nameParameter) const;
    //Utility method to parse an XML parameters file
    chaste_parameters_type* ReadFile(std::string fileName);

};

#endif /*HEARTCONFIG_HPP_*/
