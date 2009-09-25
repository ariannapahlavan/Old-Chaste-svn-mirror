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
#include "OutputFileHandler.hpp"
#include "Exception.hpp"
#include "ChastePoint.hpp"
#include "Version.hpp"

#include <cassert>

// Coping with changes to XSD interface
#if (XSD_INT_VERSION >= 3000000L)
#define XSD_SEQUENCE_TYPE(base) base##_sequence
#define XSD_ITERATOR_TYPE(base) base##_iterator
#define XSD_NESTED_TYPE(t) t##_type
#define XSD_ANON_TYPE(t1, t2) \
    t1::t2##_type
#else
#define XSD_SEQUENCE_TYPE(base) base::container
#define XSD_ITERATOR_TYPE(base) base::iterator
#define XSD_NESTED_TYPE(t) t::type
#define XSD_ANON_TYPE(t1, t2) \
    t1::t2::_xsd_##t2##_::t2
#endif

// These are for convenience
#define XSD_ANON_SEQUENCE_TYPE(t1, t2, t3) \
    XSD_SEQUENCE_TYPE(XSD_ANON_TYPE(t1, t2)::t3)
#define XSD_ANON_ITERATOR_TYPE(t1, t2, t3) \
    XSD_ITERATOR_TYPE(XSD_ANON_TYPE(t1, t2)::t3)

// Newer versions don't allow you to set fixed attributes
#if (XSD_INT_VERSION >= 3020000L)
#define XSD_CREATE_WITH_FIXED_ATTR(type, name, attr) \
    type name
#define XSD_CREATE_WITH_FIXED_ATTR1(type, name, arg1, attr) \
    type name(arg1)
#define XSD_CREATE_WITH_FIXED_ATTR2(type, name, arg1, arg2, attr) \
    type name(arg1, arg2)
#define XSD_CREATE_WITH_FIXED_ATTR3(type, name, arg1, arg2, arg3, attr) \
    type name(arg1, arg2, arg3)
#else
#define XSD_CREATE_WITH_FIXED_ATTR(type, name, attr) \
    type name(attr)
#define XSD_CREATE_WITH_FIXED_ATTR1(type, name, arg1, attr) \
    type name(arg1, attr)
#define XSD_CREATE_WITH_FIXED_ATTR2(type, name, arg1, arg2, attr) \
    type name(arg1, arg2, attr)
#define XSD_CREATE_WITH_FIXED_ATTR3(type, name, arg1, arg2, arg3, attr) \
    type name(arg1, arg2, arg3, attr)
#endif

using namespace xsd::cxx::tree;

//
// Definition of static member variables
//
std::auto_ptr<HeartConfig> HeartConfig::mpInstance;

//
// Methods
//

HeartConfig* HeartConfig::Instance()
{
    if (mpInstance.get() == NULL)
    {
        mpInstance.reset(new HeartConfig);
    }
    return mpInstance.get();
}

HeartConfig::HeartConfig()
{
    assert(mpInstance.get() == NULL);
    mUseFixedSchemaLocation = true;

    SetDefaultsFile("ChasteDefaults.xml");

    mpUserParameters = mpDefaultParameters;
    //CheckTimeSteps(); // necessity of this line of code is not tested -- remove with caution!
}

HeartConfig::~HeartConfig()
{
}

void HeartConfig::SetDefaultsFile(const std::string& rFileName)
{
    bool same_target = (mpUserParameters == mpDefaultParameters);

    mpDefaultParameters = ReadFile(rFileName);

    if (same_target)
    {
        mpUserParameters = mpDefaultParameters;
    }
    CheckTimeSteps();
}

void HeartConfig::Write(bool useArchiveLocationInfo)
{
    //Output file
    std::string output_dirname;
    if (useArchiveLocationInfo)
    {
        output_dirname = ArchiveLocationInfo::GetArchiveDirectory();
    }
    else
    {
        OutputFileHandler handler(GetOutputDirectory(), false);
        output_dirname =  handler.GetOutputDirectoryFullPath() + "output/";
    }

    out_stream p_defaults_file( new std::ofstream( (output_dirname+"ChasteDefaults.xml").c_str() ) );
    out_stream p_parameters_file( new std::ofstream( (output_dirname+"ChasteParameters.xml").c_str() ) );

    if (!p_defaults_file->is_open() || !p_parameters_file->is_open())
    {
        EXCEPTION("Could not open XML file in HeartConfig");
    }

    //Schema map
    //Note - this location is relative to where we are storing the xml
    xml_schema::namespace_infomap map;
    char buf[10000];
    std::string absolute_path_to_xsd=getcwd(buf, 10000);
    absolute_path_to_xsd += "/heart/src/io/ChasteParameters.xsd";
    map[""].schema = absolute_path_to_xsd;

    ChasteParameters(*p_parameters_file, *mpUserParameters, map);
    ChasteParameters(*p_defaults_file, *mpDefaultParameters, map);
}

boost::shared_ptr<chaste_parameters_type> HeartConfig::ReadFile(const std::string& rFileName)
{
    // Determine whether to use the schema path given in the input XML, or our own schema
    ::xml_schema::properties p;
    if (mUseFixedSchemaLocation)
    {
        std::string chaste_root = GetChasteRoot();
        p.no_namespace_schema_location(chaste_root + "/heart/src/io/ChasteParameters.xsd");
    }

    // get the parameters using the method 'ChasteParameters(rFileName)',
    // which returns a std::auto_ptr. We convert to a shared_ptr for easier semantics.
    try
    {
        std::auto_ptr<chaste_parameters_type> p_default(ChasteParameters(rFileName, 0, p));
        return boost::shared_ptr<chaste_parameters_type>(p_default);
    }
    catch (const xml_schema::exception& e)
    {
         std::cerr << e << std::endl;
         // Make sure we don't store invalid parameters
         mpUserParameters.reset();
         mpDefaultParameters.reset();
         EXCEPTION("XML parsing error in configuration file: " + rFileName);
    }
}

void HeartConfig::SetParametersFile(const std::string& rFileName)
{
    mpUserParameters = ReadFile(rFileName);

    CheckTimeSteps(); // For consistency with SetDefaultsFile
}


void HeartConfig::Reset()
{
    // Throw it away first, so that mpInstance is NULL when we...
    mpInstance.reset();
    // ...make a new one
    mpInstance.reset(new HeartConfig);
}

bool HeartConfig::IsSimulationDefined() const
{
    return mpUserParameters->Simulation().present();     
}

bool HeartConfig::IsSimulationResumed() const
{
    return mpUserParameters->ResumeSimulation().present();         
}         


template<class TYPE>
TYPE* HeartConfig::DecideLocation(TYPE* ptr1, TYPE* ptr2, const std::string& nameParameter) const
{
    if (ptr1->present())
    {
        return ptr1;
    }
    if (ptr2->present())
    {
        return ptr2;
    }
    EXCEPTION("No " + nameParameter + " provided (neither default nor user defined)");
}

void HeartConfig::CheckSimulationIsDefined(std::string callingMethod) const
{
    if(!mpUserParameters->Simulation().present())
    {
        EXCEPTION(callingMethod + " information is not available in a resumed simulation.");
    }   
}

unsigned HeartConfig::GetSpaceDimension() const
{
    CheckSimulationIsDefined("SpaceDimension");
    
    return DecideLocation( & mpUserParameters->Simulation().get().SpaceDimension(),
                           & mpDefaultParameters->Simulation().get().SpaceDimension(),
                           "SpaceDimension")->get();
}

double HeartConfig::GetSimulationDuration() const
{
    return DecideLocation( & mpUserParameters->Simulation().get().SimulationDuration(),
                           & mpDefaultParameters->Simulation().get().SimulationDuration(),
                           "SimulationDuration")->get();
}

domain_type HeartConfig::GetDomain() const
{
    return DecideLocation( & mpUserParameters->Simulation().get().Domain(),
                           & mpDefaultParameters->Simulation().get().Domain(),
                           "Domain")->get();
}

ionic_models_available_type HeartConfig::GetDefaultIonicModel() const
{
    return DecideLocation( & mpUserParameters->Simulation().get().IonicModels(),
                           & mpDefaultParameters->Simulation().get().IonicModels(),
                           "IonicModel")->get().Default();
}

void HeartConfig::GetIonicModelRegions(std::vector<ChasteCuboid>& definedRegions,
                                       std::vector<ionic_models_available_type>& ionicModels) const
{
    XSD_SEQUENCE_TYPE(ionic_models_type::Region)&
         regions = DecideLocation( & mpUserParameters->Simulation().get().IonicModels(),
                                   & mpDefaultParameters->Simulation().get().IonicModels(),
                                   "IonicModel")->get().Region();

    for (XSD_ITERATOR_TYPE(ionic_models_type::Region) i = regions.begin();
         i != regions.end();
         ++i)
    {
        ionic_model_region_type ionic_model_region(*i);
        if (ionic_model_region.Location().Cuboid())
        {
            point_type point_a = ionic_model_region.Location().Cuboid()->LowerCoordinates();
            point_type point_b = ionic_model_region.Location().Cuboid()->UpperCoordinates();
    
            ChastePoint<3> chaste_point_a ( point_a.x(),
                                            point_a.y(),
                                            point_a.z());
    
            ChastePoint<3> chaste_point_b ( point_b.x(),
                                            point_b.y(),
                                            point_b.z());
    
            definedRegions.push_back(ChasteCuboid( chaste_point_a, chaste_point_b ));
            ionicModels.push_back(ionic_model_region.IonicModel());
        }
    }
}


bool HeartConfig::IsMeshProvided() const
{
    try
    {
        DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                        & mpDefaultParameters->Simulation().get().Mesh(),
                        "Mesh");
        return true;
    }
    catch (Exception& e)
    {
        return false;
    }
}

bool HeartConfig::GetCreateMesh() const
{
    mesh_type mesh = DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                                     & mpDefaultParameters->Simulation().get().Mesh(),
                                     "Mesh")->get();

    return (mesh.Slab().present() || mesh.Sheet().present() || mesh.Fibre().present());
}

bool HeartConfig::GetCreateSlab() const
{
    mesh_type mesh = DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                                     & mpDefaultParameters->Simulation().get().Mesh(),
                                     "Mesh")->get();

    return (mesh.Slab().present());
}

bool HeartConfig::GetCreateSheet() const
{
    mesh_type mesh = DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                                     & mpDefaultParameters->Simulation().get().Mesh(),
                                     "Mesh")->get();

    return (mesh.Sheet().present());
}

bool HeartConfig::GetCreateFibre() const
{
    mesh_type mesh = DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                                     & mpDefaultParameters->Simulation().get().Mesh(),
                                     "Mesh")->get();

    return (mesh.Fibre().present());
}


bool HeartConfig::GetLoadMesh() const
{
    return (DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                            & mpDefaultParameters->Simulation().get().Mesh(),
                            "Mesh")->get().LoadMesh().present());
}

void HeartConfig::GetSlabDimensions(c_vector<double, 3>& slabDimensions) const
{
    if (GetSpaceDimension()!=3 || !GetCreateSlab())
    {
        EXCEPTION("Tissue slabs can only be defined in 3D");
    }

    optional<slab_type, false> slab_dimensions = DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                                                                  & mpDefaultParameters->Simulation().get().Mesh(),
                                                                  "Slab")->get().Slab();

    slabDimensions[0] = slab_dimensions->x();
    slabDimensions[1] = slab_dimensions->y();
    slabDimensions[2] = slab_dimensions->z();
}

void HeartConfig::GetSheetDimensions(c_vector<double, 2>& sheetDimensions) const
{
    if (GetSpaceDimension()!=2 || !GetCreateSheet())
    {
        EXCEPTION("Tissue sheets can only be defined in 2D");
    }

    optional<sheet_type, false> sheet_dimensions = DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                                                                  & mpDefaultParameters->Simulation().get().Mesh(),
                                                                  "Sheet")->get().Sheet();

    sheetDimensions[0] = sheet_dimensions->x();
    sheetDimensions[1] = sheet_dimensions->y();
}

void HeartConfig::GetFibreLength(c_vector<double, 1>& fibreLength) const
{
    if (GetSpaceDimension()!=1 || !GetCreateFibre())
    {
        EXCEPTION("Tissue fibres can only be defined in 1D");
    }

    optional<fibre_type, false> fibre_length = DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                                                                  & mpDefaultParameters->Simulation().get().Mesh(),
                                                                  "Fibre")->get().Fibre();

    fibreLength[0] = fibre_length->x();
}


double HeartConfig::GetInterNodeSpace() const
{
    assert(GetCreateMesh());

    switch(GetSpaceDimension())
    {
        case 3:
            return DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                                   & mpDefaultParameters->Simulation().get().Mesh(),
                                   "Slab")->get().Slab()->inter_node_space();
            break;
        case 2:
            return DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                                   & mpDefaultParameters->Simulation().get().Mesh(),
                                   "Sheet")->get().Sheet()->inter_node_space();
            break;
        case 1:
            return DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                                   & mpDefaultParameters->Simulation().get().Mesh(),
                                   "Fibre")->get().Fibre()->inter_node_space();
            break;
        default:
            NEVER_REACHED;
    }


}

std::string HeartConfig::GetMeshName() const
{
    assert(GetLoadMesh());

    return DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                           & mpDefaultParameters->Simulation().get().Mesh(),
                           "LoadMesh")->get().LoadMesh()->name();
}

media_type HeartConfig::GetConductivityMedia() const
{
    assert(GetLoadMesh());

    return DecideLocation( & mpUserParameters->Simulation().get().Mesh(),
                           & mpDefaultParameters->Simulation().get().Mesh(),
                           "LoadMesh")->get().LoadMesh()->conductivity_media();
}

void HeartConfig::GetStimuli(std::vector<boost::shared_ptr<SimpleStimulus> >& rStimuliApplied,
                             std::vector<ChasteCuboid>& rStimulatedAreas) const
{
    XSD_ANON_SEQUENCE_TYPE(simulation_type, Stimuli, Stimulus)&
         stimuli = DecideLocation( & mpUserParameters->Simulation().get().Stimuli(),
                           & mpDefaultParameters->Simulation().get().Stimuli(),
                           "Stimuli")->get().Stimulus();
    for (XSD_ANON_ITERATOR_TYPE(simulation_type, Stimuli, Stimulus) i = stimuli.begin();
         i != stimuli.end();
         ++i)
    {
        stimulus_type stimulus(*i);
        if (stimulus.Location().Cuboid())
        {
            point_type point_a = stimulus.Location().Cuboid()->LowerCoordinates();
            point_type point_b = stimulus.Location().Cuboid()->UpperCoordinates();
    
            ChastePoint<3> chaste_point_a ( point_a.x(),
                                            point_a.y(),
                                            point_a.z());
    
            ChastePoint<3> chaste_point_b ( point_b.x(),
                                            point_b.y(),
                                            point_b.z());
    
            boost::shared_ptr<SimpleStimulus> stim(new SimpleStimulus(stimulus.Strength(),
                                                                      stimulus.Duration(),
                                                                      stimulus.Delay()));
            rStimuliApplied.push_back( stim );
            rStimulatedAreas.push_back( ChasteCuboid( chaste_point_a, chaste_point_b ) );
        }
    }
}

void HeartConfig::GetCellHeterogeneities(std::vector<ChasteCuboid>& cellHeterogeneityAreas,
                                         std::vector<double>& scaleFactorGks,
                                         std::vector<double>& scaleFactorIto,
                                         std::vector<double>& scaleFactorGkr) const
{
    XSD_ANON_SEQUENCE_TYPE(simulation_type, CellHeterogeneities, CellHeterogeneity)&
         cell_heterogeneity = DecideLocation( & mpUserParameters->Simulation().get().CellHeterogeneities(),
                                                 & mpDefaultParameters->Simulation().get().CellHeterogeneities(),
                                                 "CellHeterogeneities")->get().CellHeterogeneity();

    for (XSD_ANON_ITERATOR_TYPE(simulation_type, CellHeterogeneities, CellHeterogeneity) i = cell_heterogeneity.begin();
         i != cell_heterogeneity.end();
         ++i)
    {
        cell_heterogeneity_type ht(*i);
        if (ht.Location().Cuboid())
        {
            point_type point_a = ht.Location().Cuboid()->LowerCoordinates();
            point_type point_b = ht.Location().Cuboid()->UpperCoordinates();

            ChastePoint<3> chaste_point_a (point_a.x(),
                                           point_a.y(),
                                           point_a.z());
    
            ChastePoint<3> chaste_point_b (point_b.x(),
                                           point_b.y(),
                                           point_b.z());
    
            scaleFactorGks.push_back (ht.ScaleFactorGks());
            scaleFactorIto.push_back (ht.ScaleFactorIto());
            scaleFactorGkr.push_back (ht.ScaleFactorGkr());
            cellHeterogeneityAreas.push_back( ChasteCuboid( chaste_point_a, chaste_point_b ) );
        }
    }
}

bool HeartConfig::GetConductivityHeterogeneitiesProvided() const
{
    try
    {
        DecideLocation( & mpUserParameters->Simulation().get().ConductivityHeterogeneities(),
                        & mpDefaultParameters->Simulation().get().ConductivityHeterogeneities(),
                        "CellHeterogeneities");
        return true;
    }
    catch (Exception& e)
    {
        return false;
    }
}

void HeartConfig::GetConductivityHeterogeneities(
        std::vector<ChasteCuboid>& conductivitiesHeterogeneityAreas,
        std::vector< c_vector<double,3> >& intraConductivities,
        std::vector< c_vector<double,3> >& extraConductivities) const
{
    XSD_ANON_SEQUENCE_TYPE(simulation_type, ConductivityHeterogeneities, ConductivityHeterogeneity)&
         conductivity_heterogeneity = DecideLocation( & mpUserParameters->Simulation().get().ConductivityHeterogeneities(),
                                                      & mpDefaultParameters->Simulation().get().ConductivityHeterogeneities(),
                                                      "CellHeterogeneities")->get().ConductivityHeterogeneity();

    for (XSD_ANON_ITERATOR_TYPE(simulation_type, ConductivityHeterogeneities, ConductivityHeterogeneity) i = conductivity_heterogeneity.begin();
         i != conductivity_heterogeneity.end();
         ++i)
    {
        conductivity_heterogeneity_type ht(*i);
        if (ht.Location().Cuboid())
        {
            point_type point_a = ht.Location().Cuboid()->LowerCoordinates();
            point_type point_b = ht.Location().Cuboid()->UpperCoordinates();
    
            ChastePoint<3> chaste_point_a (point_a.x(),
                                           point_a.y(),
                                           point_a.z());
    
            ChastePoint<3> chaste_point_b (point_b.x(),
                                           point_b.y(),
                                           point_b.z());
    
            conductivitiesHeterogeneityAreas.push_back( ChasteCuboid( chaste_point_a, chaste_point_b ) );
        }

        if (ht.IntracellularConductivities().present())
        {
            double intra_x = ht.IntracellularConductivities().get().longi();
            double intra_y = ht.IntracellularConductivities().get().trans();
            double intra_z = ht.IntracellularConductivities().get().normal();

            intraConductivities.push_back( Create_c_vector(intra_x, intra_y, intra_z) );
        }
        else
        {
            c_vector<double, 3> intra_conductivities;
            GetIntracellularConductivities(intra_conductivities);
            intraConductivities.push_back(intra_conductivities);
        }

        if (ht.ExtracellularConductivities().present())
        {
            double extra_x = ht.ExtracellularConductivities().get().longi();
            double extra_y = ht.ExtracellularConductivities().get().trans();
            double extra_z = ht.ExtracellularConductivities().get().normal();

            extraConductivities.push_back( Create_c_vector(extra_x, extra_y, extra_z) );
        }
        else
        {
            c_vector<double, 3> extra_conductivities;
            GetExtracellularConductivities(extra_conductivities);
            extraConductivities.push_back(extra_conductivities);
        }

    }
}

std::string HeartConfig::GetOutputDirectory() const
{
    return DecideLocation( & mpUserParameters->Simulation().get().OutputDirectory(),
                           & mpDefaultParameters->Simulation().get().OutputDirectory(),
                           "OutputDirectory")->get();
}

std::string HeartConfig::GetOutputFilenamePrefix() const
{
    return DecideLocation( & mpUserParameters->Simulation().get().OutputFilenamePrefix(),
                           & mpDefaultParameters->Simulation().get().OutputFilenamePrefix(),
                           "OutputFilenamePrefix")->get();
}

bool HeartConfig::GetOutputVariablesProvided() const
{
    try
    {
        DecideLocation( & mpUserParameters->Simulation().get().OutputVariables(),
                        & mpDefaultParameters->Simulation().get().OutputVariables(),
                        "OutputVariables");                        
        return true;
    }
    catch (Exception& e)
    {            
        return false;
    }        
}

void HeartConfig::GetOutputVariables(std::vector<std::string> &outputVariables) const
{
    XSD_SEQUENCE_TYPE(output_variables_type::Var)&
         output_variables = DecideLocation( & mpUserParameters->Simulation().get().OutputVariables(),
                           & mpDefaultParameters->Simulation().get().OutputVariables(),
                           "OutputVariables")->get().Var();
    
    for (XSD_ITERATOR_TYPE(output_variables_type::Var) i = output_variables.begin();
         i != output_variables.end();
         ++i)
    {
        var_type var(*i);
        
        // Add to outputVariables the string returned by var.name() 
        outputVariables.push_back(var.name());
    }
}

bool HeartConfig::GetSaveSimulation() const
{
    try
    {
        DecideLocation( & mpUserParameters->Simulation().get().SaveSimulation(),
                        & mpDefaultParameters->Simulation().get().SaveSimulation(),
                        "SaveSimulation");                        
        return true;
    }
    catch (Exception& e)
    {            
        return false;
    }            
}

void HeartConfig::GetIntracellularConductivities(c_vector<double, 3>& intraConductivities) const
{
    optional<conductivities_type, false>* intra_conductivities  = DecideLocation( & mpUserParameters->Physiological().IntracellularConductivities(),
                                                                                  & mpDefaultParameters->Physiological().IntracellularConductivities(),
                                                                                  "IntracellularConductivities");
    double intra_x_cond = intra_conductivities->get().longi();
    double intra_y_cond = intra_conductivities->get().trans();
    double intra_z_cond = intra_conductivities->get().normal();;

    assert(intra_y_cond != DBL_MAX);
    assert(intra_z_cond != DBL_MAX);

    intraConductivities[0] = intra_x_cond;
    intraConductivities[1] = intra_y_cond;
    intraConductivities[2] = intra_z_cond;
}

void HeartConfig::GetIntracellularConductivities(c_vector<double, 2>& intraConductivities) const
{
    optional<conductivities_type, false>* intra_conductivities  = DecideLocation( & mpUserParameters->Physiological().IntracellularConductivities(),
                                                                                  & mpDefaultParameters->Physiological().IntracellularConductivities(),
                                                                                  "IntracellularConductivities");
    double intra_x_cond = intra_conductivities->get().longi();
    double intra_y_cond = intra_conductivities->get().trans();

    assert(intra_y_cond != DBL_MAX);

    intraConductivities[0] = intra_x_cond;
    intraConductivities[1] = intra_y_cond;
}

void HeartConfig::GetIntracellularConductivities(c_vector<double, 1>& intraConductivities) const
{
    optional<conductivities_type, false>* intra_conductivities  = DecideLocation( & mpUserParameters->Physiological().IntracellularConductivities(),
                                                                                  & mpDefaultParameters->Physiological().IntracellularConductivities(),
                                                                                  "IntracellularConductivities");
    double intra_x_cond = intra_conductivities->get().longi();

    intraConductivities[0] = intra_x_cond;
}

void HeartConfig::GetExtracellularConductivities(c_vector<double, 3>& extraConductivities) const
{
    optional<conductivities_type, false>* extra_conductivities  = DecideLocation( & mpUserParameters->Physiological().ExtracellularConductivities(),
                                                                                  & mpDefaultParameters->Physiological().ExtracellularConductivities(),
                                                                                  "ExtracellularConductivities");
    double extra_x_cond = extra_conductivities->get().longi();
    double extra_y_cond = extra_conductivities->get().trans();
    double extra_z_cond = extra_conductivities->get().normal();;

    assert(extra_y_cond != DBL_MAX);
    assert(extra_z_cond != DBL_MAX);

    extraConductivities[0] = extra_x_cond;
    extraConductivities[1] = extra_y_cond;
    extraConductivities[2] = extra_z_cond;
}

void HeartConfig::GetExtracellularConductivities(c_vector<double, 2>& extraConductivities) const
{
    optional<conductivities_type, false>* extra_conductivities  = DecideLocation( & mpUserParameters->Physiological().ExtracellularConductivities(),
                                                                                  & mpDefaultParameters->Physiological().ExtracellularConductivities(),
                                                                                  "ExtracellularConductivities");
    double extra_x_cond = extra_conductivities->get().longi();
    double extra_y_cond = extra_conductivities->get().trans();

    assert(extra_y_cond != DBL_MAX);

    extraConductivities[0] = extra_x_cond;
    extraConductivities[1] = extra_y_cond;
}

void HeartConfig::GetExtracellularConductivities(c_vector<double, 1>& extraConductivities) const
{
    optional<conductivities_type, false>* extra_conductivities  = DecideLocation( & mpUserParameters->Physiological().ExtracellularConductivities(),
                                                                                  & mpDefaultParameters->Physiological().ExtracellularConductivities(),
                                                                                  "ExtracellularConductivities");
    double extra_x_cond = extra_conductivities->get().longi();

    extraConductivities[0] = extra_x_cond;
}

double HeartConfig::GetBathConductivity() const
{
    /*bath conductivity mS/cm*/
    return DecideLocation( & mpUserParameters->Physiological().BathConductivity(),
                           & mpDefaultParameters->Physiological().BathConductivity(),
                           "BathConductivity")->get();
}

double HeartConfig::GetSurfaceAreaToVolumeRatio() const
{
    /*surface area to volume ratio: 1/cm*/
    return DecideLocation( & mpUserParameters->Physiological().SurfaceAreaToVolumeRatio(),
                           & mpDefaultParameters->Physiological().SurfaceAreaToVolumeRatio(),
                           "SurfaceAreaToVolumeRatio")->get();
}

double HeartConfig::GetCapacitance() const
{
    //         capacitance                 : uF/cm^2
    return DecideLocation( & mpUserParameters->Physiological().Capacitance(),
                           & mpDefaultParameters->Physiological().Capacitance(),
                           "Capacitance")->get();
}

double HeartConfig::GetOdeTimeStep() const
{
    return DecideLocation( & mpUserParameters->Numerical().TimeSteps(),
                           & mpDefaultParameters->Numerical().TimeSteps(),
                           "ode TimeStep")->get().ode();
}

double HeartConfig::GetPdeTimeStep() const
{
    return DecideLocation( & mpUserParameters->Numerical().TimeSteps(),
                           & mpDefaultParameters->Numerical().TimeSteps(),
                           "pde TimeStep")->get().pde();
}

double HeartConfig::GetPrintingTimeStep() const
{
    return DecideLocation( & mpUserParameters->Numerical().TimeSteps(),
                           & mpDefaultParameters->Numerical().TimeSteps(),
                           "printing TimeStep")->get().printing();
}

bool HeartConfig::GetUseAbsoluteTolerance() const
{
     return DecideLocation( & mpUserParameters->Numerical().KSPTolerances(),
                            & mpDefaultParameters->Numerical().KSPTolerances(),
                            "KSPTolerances")->get().KSPAbsolute().present();
}

double HeartConfig::GetAbsoluteTolerance() const
{
    if (!GetUseAbsoluteTolerance())
    {
        EXCEPTION("Absolute tolerance is not set in Chaste parameters");
    }
    return DecideLocation( & mpUserParameters->Numerical().KSPTolerances(),
                           & mpDefaultParameters->Numerical().KSPTolerances(),
                           "KSPTolerances")->get().KSPAbsolute().get();
}

bool HeartConfig::GetUseRelativeTolerance() const
{
     return DecideLocation( & mpUserParameters->Numerical().KSPTolerances(),
                            & mpDefaultParameters->Numerical().KSPTolerances(),
                            "KSPTolerances")->get().KSPRelative().present();
}

double HeartConfig::GetRelativeTolerance() const
{
    if (!GetUseRelativeTolerance())
    {
        EXCEPTION("Relative tolerance is not set in Chaste parameters");
    }

    return DecideLocation( & mpUserParameters->Numerical().KSPTolerances(),
                           & mpDefaultParameters->Numerical().KSPTolerances(),
                           "KSPTolerances")->get().KSPRelative().get();
}

const char* HeartConfig::GetKSPSolver() const
{
    switch ( DecideLocation( & mpUserParameters->Numerical().KSPSolver(),
                             & mpDefaultParameters->Numerical().KSPSolver(),
                            "KSPSolver")->get() )
    {
        case ksp_solver_type::gmres :
            return "gmres";
        case ksp_solver_type::cg :
            return "cg";
        case ksp_solver_type::symmlq :
            return "symmlq";
    }
#define COVERAGE_IGNORE
    EXCEPTION("Unknown ksp solver");
#undef COVERAGE_IGNORE
}

const char* HeartConfig::GetKSPPreconditioner() const
{
    switch ( DecideLocation( & mpUserParameters->Numerical().KSPPreconditioner(),
                             & mpDefaultParameters->Numerical().KSPPreconditioner(),
                             "KSPPreconditioner")->get() )
    {
        case ksp_preconditioner_type::ilu :
            return "ilu";
        case ksp_preconditioner_type::jacobi :
            return "jacobi";
        case ksp_preconditioner_type::bjacobi :
            return "bjacobi";
        case ksp_preconditioner_type::hypre :
            return "hypre";
        case ksp_preconditioner_type::blockdiagonal :
            return "blockdiagonal";
        case ksp_preconditioner_type::none :
            return "none";

    }
#define COVERAGE_IGNORE
    EXCEPTION("Unknown ksp preconditioner");
#undef COVERAGE_IGNORE
}

/*
 * PostProcessing
 */

bool HeartConfig::IsPostProcessingSectionPresent() const
{
    try
    {
        DecideLocation( & mpUserParameters->PostProcessing(),
                           & mpDefaultParameters->PostProcessing(),
                           "PostProcessing")->present();
        //If there's a section
        return true;
    }
    catch (Exception &e)
    {
        //No section
        return false;
    }
}

bool HeartConfig::IsPostProcessingRequested() const
{
    if (IsPostProcessingSectionPresent() == false)
    {
        return false;
    }
    else 
    {
        return(IsApdMapsRequested() || 
               IsUpstrokeTimeMapsRequested() || 
               IsMaxUpstrokeVelocityMapRequested() || 
               IsConductionVelocityMapsRequested());
    }
}
bool HeartConfig::IsApdMapsRequested() const
{
    assert(IsPostProcessingSectionPresent());

    XSD_SEQUENCE_TYPE(postprocessing_type::ActionPotentialDurationMap)&
        apd_maps = DecideLocation( & mpUserParameters->PostProcessing(),
                                   & mpDefaultParameters->PostProcessing(),
                                   "ActionPotentialDurationMap")->get().ActionPotentialDurationMap();
    return (apd_maps.begin() != apd_maps.end());
}

void HeartConfig::GetApdMaps(std::vector<std::pair<double,double> >& apd_maps) const
{
    assert(IsApdMapsRequested());
    apd_maps.clear();

    XSD_SEQUENCE_TYPE(postprocessing_type::ActionPotentialDurationMap)&
        apd_maps_sequence = DecideLocation( & mpUserParameters->PostProcessing(),
                                   & mpDefaultParameters->PostProcessing(),
                                   "ActionPotentialDurationMap")->get().ActionPotentialDurationMap();

    for (XSD_ITERATOR_TYPE(postprocessing_type::ActionPotentialDurationMap) i = apd_maps_sequence.begin();
         i != apd_maps_sequence.end();
         ++i)
    {
        std::pair<double,double> map(i->repolarisation_percentage(),i->threshold());

        apd_maps.push_back(map);
    }
}

bool HeartConfig::IsUpstrokeTimeMapsRequested() const
{
    assert(IsPostProcessingSectionPresent());

    XSD_SEQUENCE_TYPE(postprocessing_type::UpstrokeTimeMap)&
        upstroke_map = DecideLocation( & mpUserParameters->PostProcessing(),
                                   & mpDefaultParameters->PostProcessing(),
                                   "UpstrokeTimeMap")->get().UpstrokeTimeMap();
    return (upstroke_map.begin() != upstroke_map.end());
}
void HeartConfig::GetUpstrokeTimeMaps (std::vector<double>& upstroke_time_maps) const
{
    assert(IsUpstrokeTimeMapsRequested());
    assert(upstroke_time_maps.size() == 0);

    XSD_SEQUENCE_TYPE(postprocessing_type::UpstrokeTimeMap)&
        upstroke_maps_sequence = DecideLocation( & mpUserParameters->PostProcessing(),
                                   & mpDefaultParameters->PostProcessing(),
                                   "UpstrokeTimeMap")->get().UpstrokeTimeMap();

    for (XSD_ITERATOR_TYPE(postprocessing_type::UpstrokeTimeMap) i = upstroke_maps_sequence.begin();
         i != upstroke_maps_sequence.end();
         ++i)
    {
        upstroke_time_maps.push_back(i->threshold());
    }
}

bool HeartConfig::IsMaxUpstrokeVelocityMapRequested() const
{
    assert(IsPostProcessingSectionPresent());
    
    XSD_SEQUENCE_TYPE(postprocessing_type::MaxUpstrokeVelocityMap)&
    max_upstroke_velocity_map = DecideLocation( & mpUserParameters->PostProcessing(),
                            & mpDefaultParameters->PostProcessing(),
                            "MaxUpstrokeVelocityMap")->get().MaxUpstrokeVelocityMap();
                            
    return (max_upstroke_velocity_map.begin() != max_upstroke_velocity_map.end());
}

void HeartConfig::GetMaxUpstrokeVelocityMaps(std::vector<double>& upstroke_velocity_maps) const
{
    assert(IsMaxUpstrokeVelocityMapRequested());
    assert(upstroke_velocity_maps.size() == 0);

    XSD_SEQUENCE_TYPE(postprocessing_type::MaxUpstrokeVelocityMap)&
        max_upstroke_velocity_maps_sequence = DecideLocation( & mpUserParameters->PostProcessing(),
                                   & mpDefaultParameters->PostProcessing(),
                                   "MaxUpstrokeVelocityMap")->get().MaxUpstrokeVelocityMap();

    for (XSD_ITERATOR_TYPE(postprocessing_type::MaxUpstrokeVelocityMap) i = max_upstroke_velocity_maps_sequence.begin();
         i != max_upstroke_velocity_maps_sequence.end();
         ++i)
    {
        upstroke_velocity_maps.push_back(i->threshold());       
    }
}

bool HeartConfig::IsConductionVelocityMapsRequested() const
{
    assert(IsPostProcessingSectionPresent());

    XSD_SEQUENCE_TYPE(postprocessing_type::ConductionVelocityMap)&
        cond_vel_maps = DecideLocation( & mpUserParameters->PostProcessing(),
                                   & mpDefaultParameters->PostProcessing(),
                                   "ConductionVelocityMap")->get().ConductionVelocityMap();
    return (cond_vel_maps.begin() != cond_vel_maps.end());
}

void HeartConfig::GetConductionVelocityMaps(std::vector<unsigned>& conduction_velocity_maps) const
{
    assert(IsConductionVelocityMapsRequested());
    assert(conduction_velocity_maps.size() == 0);

    XSD_SEQUENCE_TYPE(postprocessing_type::ConductionVelocityMap)&
        cond_vel_maps_sequence = DecideLocation( & mpUserParameters->PostProcessing(),
                                   & mpDefaultParameters->PostProcessing(),
                                   "ConductionVelocityMap")->get().ConductionVelocityMap();

    for (XSD_ITERATOR_TYPE(postprocessing_type::ConductionVelocityMap) i = cond_vel_maps_sequence.begin();
         i != cond_vel_maps_sequence.end();
         ++i)
    {
        conduction_velocity_maps.push_back(i->origin_node());
    }
}


/*
 *  Set methods
 */
void HeartConfig::SetSpaceDimension(unsigned spaceDimension)
{
    mpUserParameters->Simulation().get().SpaceDimension().set(spaceDimension);
}

void HeartConfig::SetSimulationDuration(double simulationDuration)
{
    XSD_CREATE_WITH_FIXED_ATTR1(time_type, time, simulationDuration, "ms");
    mpUserParameters->Simulation().get().SimulationDuration().set(time);
}

void HeartConfig::SetDomain(domain_type domain)
{
    mpUserParameters->Simulation().get().Domain().set(domain);
}

void HeartConfig::SetDefaultIonicModel(ionic_models_available_type ionicModel)
{
    mpUserParameters->Simulation().get().IonicModels().set(ionicModel);
}

void HeartConfig::SetSlabDimensions(double x, double y, double z, double inter_node_space)
{
    if ( ! mpUserParameters->Simulation().get().Mesh().present())
    {
        XSD_CREATE_WITH_FIXED_ATTR(mesh_type, mesh_to_load, "cm");
        mpUserParameters->Simulation().get().Mesh().set(mesh_to_load);
    }

    slab_type slab_definition(x, y, z, inter_node_space);
    mpUserParameters->Simulation().get().Mesh().get().Slab().set(slab_definition);
}

void HeartConfig::SetSheetDimensions(double x, double y, double inter_node_space)
{
    if ( ! mpUserParameters->Simulation().get().Mesh().present())
    {
        XSD_CREATE_WITH_FIXED_ATTR(mesh_type, mesh_to_load, "cm");
        mpUserParameters->Simulation().get().Mesh().set(mesh_to_load);
    }

    sheet_type sheet_definition(x, y, inter_node_space);
    mpUserParameters->Simulation().get().Mesh().get().Sheet().set(sheet_definition);
}

void HeartConfig::SetFibreLength(double x, double inter_node_space)
{
    if ( ! mpUserParameters->Simulation().get().Mesh().present())
    {
        XSD_CREATE_WITH_FIXED_ATTR(mesh_type, mesh_to_load, "cm");
        mpUserParameters->Simulation().get().Mesh().set(mesh_to_load);
    }

    fibre_type fibre_definition(x, inter_node_space);
    mpUserParameters->Simulation().get().Mesh().get().Fibre().set(fibre_definition);
}

void HeartConfig::SetMeshFileName(std::string meshPrefix, media_type fibreDefinition)
{
    if ( ! mpUserParameters->Simulation().get().Mesh().present())
    {
        XSD_CREATE_WITH_FIXED_ATTR(mesh_type, mesh_to_load, "cm");
        mpUserParameters->Simulation().get().Mesh().set(mesh_to_load);
    }

    XSD_NESTED_TYPE(mesh_type::LoadMesh) mesh_prefix(meshPrefix, fibreDefinition);
    mpUserParameters->Simulation().get().Mesh().get().LoadMesh().set(mesh_prefix);
}

void HeartConfig::SetConductivityHeterogeneities(
        std::vector< c_vector<double,3> >& cornerA,
        std::vector< c_vector<double,3> >& cornerB,
        std::vector< c_vector<double,3> >& intraConductivities,
        std::vector< c_vector<double,3> >& extraConductivities)
{
    assert ( cornerA.size() == cornerB.size() );
    assert ( cornerB.size() == intraConductivities.size() );
    assert ( intraConductivities.size() == extraConductivities.size());

    XSD_ANON_SEQUENCE_TYPE(simulation_type, ConductivityHeterogeneities, ConductivityHeterogeneity) heterogeneities_container;

    for (unsigned region_index=0; region_index<cornerA.size(); region_index++)
    {
        point_type point_a(cornerA[region_index][0],
                           cornerA[region_index][1],
                           cornerA[region_index][2]);

        point_type point_b(cornerB[region_index][0],
                           cornerB[region_index][1],
                           cornerB[region_index][2]);
    
        XSD_CREATE_WITH_FIXED_ATTR(location_type, locn, "cm");
        locn.Cuboid().set(box_type(point_a, point_b));
        conductivity_heterogeneity_type ht(locn);

        XSD_CREATE_WITH_FIXED_ATTR3(conductivities_type, intra,
                                    intraConductivities[region_index][0],
                                    intraConductivities[region_index][1],
                                    intraConductivities[region_index][2],
                                    "mS/cm");

        ht.IntracellularConductivities(intra);

        XSD_CREATE_WITH_FIXED_ATTR3(conductivities_type, extra,
                                    extraConductivities[region_index][0],
                                    extraConductivities[region_index][1],
                                    extraConductivities[region_index][2],
                                    "mS/cm");

        ht.ExtracellularConductivities(extra);

        heterogeneities_container.push_back(ht);
    }

    XSD_ANON_TYPE(simulation_type, ConductivityHeterogeneities) heterogeneities_object;
    heterogeneities_object.ConductivityHeterogeneity(heterogeneities_container);

    mpUserParameters->Simulation().get().ConductivityHeterogeneities().set(heterogeneities_object);
}


void HeartConfig::SetOutputDirectory(const std::string& rOutputDirectory)
{
    mpUserParameters->Simulation().get().OutputDirectory().set(rOutputDirectory);
}

void HeartConfig::SetOutputFilenamePrefix(const std::string& rOutputFilenamePrefix)
{
    mpUserParameters->Simulation().get().OutputFilenamePrefix().set(rOutputFilenamePrefix);
}

void HeartConfig::SetOutputVariables(const std::vector<std::string>& rOutputVariables)
{
    if ( ! mpUserParameters->Simulation().get().OutputVariables().present())
    {
        output_variables_type variables_requested;
        mpUserParameters->Simulation().get().OutputVariables().set(variables_requested);
    }
        
    XSD_SEQUENCE_TYPE(output_variables_type::Var)&
    var_type_sequence = mpUserParameters->Simulation().get().OutputVariables()->Var();
    //Erase or create a sequence
    var_type_sequence.clear();

    for (unsigned i=0; i<rOutputVariables.size(); i++)
    {
        var_type temp(rOutputVariables[i]);
        var_type_sequence.push_back(temp);
    }
}

void HeartConfig::SetSaveSimulation(bool saveSimulation)
{
    if (saveSimulation)
    {
        /// \todo: find a way of defining an empty element, so we don't have to pass a simulation_type::SaveSimulation_type object here.
        mpUserParameters->Simulation().get().SaveSimulation().set(simulation_type::SaveSimulation_type(""));
    }
    else
    {
        mpUserParameters->Simulation().get().SaveSimulation().reset();
    }        
}

// Physiological
void HeartConfig::SetIntracellularConductivities(const c_vector<double, 3>& intraConductivities)
{
    XSD_CREATE_WITH_FIXED_ATTR3(conductivities_type, intra,
                                intraConductivities[0],
                                intraConductivities[1],
                                intraConductivities[2],
                                "mS/cm");

    mpUserParameters->Physiological().IntracellularConductivities().set(intra);
}

void HeartConfig::SetIntracellularConductivities(const c_vector<double, 2>& intraConductivities)
{
    XSD_CREATE_WITH_FIXED_ATTR3(conductivities_type, intra,
                                intraConductivities[0],
                                intraConductivities[1],
                                0.0, "mS/cm");

    mpUserParameters->Physiological().IntracellularConductivities().set(intra);
}

void HeartConfig::SetIntracellularConductivities(const c_vector<double, 1>& intraConductivities)
{
    XSD_CREATE_WITH_FIXED_ATTR3(conductivities_type, intra,
                                intraConductivities[0],
                                0.0, 0.0, "mS/cm");

    mpUserParameters->Physiological().IntracellularConductivities().set(intra);
}

void HeartConfig::SetExtracellularConductivities(const c_vector<double, 3>& extraConductivities)
{
    XSD_CREATE_WITH_FIXED_ATTR3(conductivities_type, extra,
                                extraConductivities[0],
                                extraConductivities[1],
                                extraConductivities[2],
                                "mS/cm");

    mpUserParameters->Physiological().ExtracellularConductivities().set(extra);
}

void HeartConfig::SetExtracellularConductivities(const c_vector<double, 2>& extraConductivities)
{
    XSD_CREATE_WITH_FIXED_ATTR3(conductivities_type, extra,
                                extraConductivities[0],
                                extraConductivities[1],
                                0.0, "mS/cm");

    mpUserParameters->Physiological().ExtracellularConductivities().set(extra);
}

void HeartConfig::SetExtracellularConductivities(const c_vector<double, 1>& extraConductivities)
{
    XSD_CREATE_WITH_FIXED_ATTR3(conductivities_type, extra,
                                extraConductivities[0],
                                0.0, 0.0, "mS/cm");

    mpUserParameters->Physiological().ExtracellularConductivities().set(extra);
}

void HeartConfig::SetBathConductivity(double bathConductivity)
{
    XSD_CREATE_WITH_FIXED_ATTR1(conductivity_type, cond, bathConductivity, "mS/cm");
    mpUserParameters->Physiological().BathConductivity().set(cond);
}

void HeartConfig::SetSurfaceAreaToVolumeRatio(double ratio)
{
    XSD_CREATE_WITH_FIXED_ATTR1(inverse_length_type, ratio_object, ratio, "1/cm");
    mpUserParameters->Physiological().SurfaceAreaToVolumeRatio().set(ratio_object);
}

void HeartConfig::SetCapacitance(double capacitance)
{
    XSD_CREATE_WITH_FIXED_ATTR1(capacitance_type, capacitance_object, capacitance, "uF/cm^2");
    mpUserParameters->Physiological().Capacitance().set(capacitance_object);
}


// Numerical
void HeartConfig::SetOdePdeAndPrintingTimeSteps(double odeTimeStep, double pdeTimeStep, double printingTimeStep)
{
    XSD_CREATE_WITH_FIXED_ATTR3(time_steps_type, time_steps,
                                odeTimeStep, pdeTimeStep, printingTimeStep, "ms");
    mpUserParameters->Numerical().TimeSteps().set(time_steps);
    CheckTimeSteps();
}

void HeartConfig::SetOdeTimeStep(double odeTimeStep)
{
    SetOdePdeAndPrintingTimeSteps(odeTimeStep, GetPdeTimeStep(), GetPrintingTimeStep());
}

void HeartConfig::SetPdeTimeStep(double pdeTimeStep)
{
    SetOdePdeAndPrintingTimeSteps(GetOdeTimeStep(), pdeTimeStep, GetPrintingTimeStep());
}

void HeartConfig::SetPrintingTimeStep(double printingTimeStep)
{
    SetOdePdeAndPrintingTimeSteps(GetOdeTimeStep(), GetPdeTimeStep(), printingTimeStep);
}

void HeartConfig::CheckTimeSteps() const
{
    if (GetOdeTimeStep() <= 0)
    {
        EXCEPTION("Ode time-step should be positive");
    }
    if (GetPdeTimeStep() <= 0)
    {
        EXCEPTION("Pde time-step should be positive");
    }
    if (GetPrintingTimeStep() <= 0.0)
    {
        EXCEPTION("Printing time-step should be positive");
    }

    if (GetPdeTimeStep()>GetPrintingTimeStep())
    {
        EXCEPTION("Printing time-step should not be smaller than PDE time step");
    }

    //If pde divides printing then the floating remainder ought to be close to
    //zero(+a smidge) or pde-a smidge
    double remainder=fmod(GetPrintingTimeStep(), GetPdeTimeStep());

    if ( remainder > DBL_EPSILON && remainder < GetPdeTimeStep()-DBL_EPSILON)
    {
        EXCEPTION("Printing time-step should be a multiple of PDE time step");
    }

    if ( GetOdeTimeStep() > GetPdeTimeStep() )
    {
        EXCEPTION("Ode time-step should not be greater than pde time-step");
    }
}


void HeartConfig::SetUseRelativeTolerance(double relativeTolerance)
{
    //Remove any reference to tolerances is user parameters
    mpUserParameters->Numerical().KSPTolerances().get().KSPAbsolute().reset();
    mpUserParameters->Numerical().KSPTolerances().get().KSPRelative().set(relativeTolerance);
}

void HeartConfig::SetUseAbsoluteTolerance(double absoluteTolerance)
{
    //Remove any reference to tolerances is user parameters
    mpUserParameters->Numerical().KSPTolerances().get().KSPRelative().reset();
    mpUserParameters->Numerical().KSPTolerances().get().KSPAbsolute().set(absoluteTolerance);
}

void HeartConfig::SetKSPSolver(const char* kspSolver)
{
    /* Note that changes in these conditions need to be reflected in the Doxygen*/
    if ( strcmp(kspSolver, "gmres") == 0)
    {
        mpUserParameters->Numerical().KSPSolver().set(ksp_solver_type::gmres);
        return;
    }
    if ( strcmp(kspSolver, "cg") == 0)
    {
        mpUserParameters->Numerical().KSPSolver().set(ksp_solver_type::cg);
        return;
    }
    if ( strcmp(kspSolver, "symmlq") == 0)
    {
        mpUserParameters->Numerical().KSPSolver().set(ksp_solver_type::symmlq);
        return;
    }

    EXCEPTION("Unknown solver type provided");
}

void HeartConfig::SetKSPPreconditioner(const char* kspPreconditioner)
{
    /* Note that changes in these conditions need to be reflected in the Doxygen*/
    if ( strcmp(kspPreconditioner, "ilu") == 0)
    {
        mpUserParameters->Numerical().KSPPreconditioner().set(ksp_preconditioner_type::ilu);
        return;
    }
    if ( strcmp(kspPreconditioner, "jacobi") == 0)
    {
        mpUserParameters->Numerical().KSPPreconditioner().set(ksp_preconditioner_type::jacobi);
        return;
    }
    if ( strcmp(kspPreconditioner, "bjacobi") == 0)
    {
        mpUserParameters->Numerical().KSPPreconditioner().set(ksp_preconditioner_type::bjacobi);
        return;
    }
    if ( strcmp(kspPreconditioner, "hypre") == 0)
    {
        mpUserParameters->Numerical().KSPPreconditioner().set(ksp_preconditioner_type::hypre);
        return;
    }
    if ( strcmp(kspPreconditioner, "blockdiagonal") == 0)
    {
        mpUserParameters->Numerical().KSPPreconditioner().set(ksp_preconditioner_type::blockdiagonal);
        return;
    }
    if ( strcmp(kspPreconditioner, "none") == 0)
    {
        mpUserParameters->Numerical().KSPPreconditioner().set(ksp_preconditioner_type::none);
        return;
    }

    EXCEPTION("Unknown preconditioner type provided");
}

void HeartConfig::SetApdMaps(const std::vector<std::pair<double,double> >& apdMaps)
{
    XSD_SEQUENCE_TYPE(postprocessing_type::ActionPotentialDurationMap)& apd_maps_sequence
      = mpUserParameters->PostProcessing()->ActionPotentialDurationMap();
    //Erase or create a sequence
    apd_maps_sequence.clear();

    for (unsigned i=0; i<apdMaps.size(); i++)
    {
        XSD_CREATE_WITH_FIXED_ATTR2(apd_map_type, temp,
                                    apdMaps[i].first, apdMaps[i].second,
                                    "mV");
        apd_maps_sequence.push_back( temp);
    }
}


void HeartConfig::SetUpstrokeTimeMaps (std::vector<double>& upstrokeTimeMaps)
{
    XSD_SEQUENCE_TYPE(postprocessing_type::UpstrokeTimeMap)& var_type_sequence 
      = mpUserParameters->PostProcessing()->UpstrokeTimeMap();

    //Erase or create a sequence
    var_type_sequence.clear();

    for (unsigned i=0; i<upstrokeTimeMaps.size(); i++)
    {
        XSD_CREATE_WITH_FIXED_ATTR1(upstrokes_map_type, temp,
                                    upstrokeTimeMaps[i],
                                    "mV");
        var_type_sequence.push_back(temp);
    }
}

void HeartConfig::SetMaxUpstrokeVelocityMaps (std::vector<double>& maxUpstrokeVelocityMaps)
{
    XSD_SEQUENCE_TYPE(postprocessing_type::MaxUpstrokeVelocityMap)& max_upstroke_velocity_maps_sequence
      = mpUserParameters->PostProcessing()->MaxUpstrokeVelocityMap();

    //Erase or create a sequence
    max_upstroke_velocity_maps_sequence.clear();

    for (unsigned i=0; i<maxUpstrokeVelocityMaps.size(); i++)
    {
        XSD_CREATE_WITH_FIXED_ATTR1(max_upstrokes_velocity_map_type, temp,
                                    maxUpstrokeVelocityMaps[i],
                                    "mV");
                                    
        
        max_upstroke_velocity_maps_sequence.push_back(temp);
    }
    
}

void HeartConfig::SetConductionVelocityMaps (std::vector<unsigned>& conductionVelocityMaps)
{
    XSD_SEQUENCE_TYPE(postprocessing_type::ConductionVelocityMap)& conduction_velocity_maps_sequence
      = mpUserParameters->PostProcessing()->ConductionVelocityMap();

    //Erase or create a sequence
    conduction_velocity_maps_sequence.clear();

    for (unsigned i=0; i<conductionVelocityMaps.size(); i++)
    {
        conduction_velocity_map_type temp(conductionVelocityMaps[i]);        
        conduction_velocity_maps_sequence.push_back(temp);
    }
}

void HeartConfig::SetUseFixedSchemaLocation(bool useFixedSchemaLocation)
{
    mUseFixedSchemaLocation = useFixedSchemaLocation;
}
