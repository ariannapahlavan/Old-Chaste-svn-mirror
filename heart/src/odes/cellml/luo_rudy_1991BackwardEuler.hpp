#ifndef CML_LUO_RUDY_1991_PE_LUT_BE_HPP_
#define CML_LUO_RUDY_1991_PE_LUT_BE_HPP_

//! @file
//! 
//! This source file was generated from CellML.
//! 
//! Model: luo_rudy_1991
//! 
//! Processed by pycml - CellML Tools in Python
//!     (translate: 8942, pycml: 8960, optimize: 8942)
//! on Mon May 24 13:51:25 2010
//! 
//! <autogenerated>

#include "ChasteSerialization.hpp"
#include <boost/serialization/base_object.hpp>
#include "AbstractBackwardEulerCardiacCell.hpp"
#include "AbstractStimulusFunction.hpp"

class CML_luo_rudy_1991_pe_lut_be : public AbstractBackwardEulerCardiacCell<1>
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & archive, const unsigned int version)
    {
        archive & boost::serialization::base_object<AbstractBackwardEulerCardiacCell<1> >(*this);
    }
    
    // 
    // Settable parameters and readable variables
    // 
    double var_membrane__I_stim;
    double var_membrane__i_K;
    double var_membrane__i_K1;
    double var_membrane__i_Kp;
    double var_membrane__i_Na;
    double var_membrane__i_b;
    double var_membrane__i_si;
    
public:
    double Get_membrane__I_stim();
    double Get_membrane__i_K();
    double Get_membrane__i_K1();
    double Get_membrane__i_Kp();
    double Get_membrane__i_Na();
    double Get_membrane__i_b();
    double Get_membrane__i_si();
    CML_luo_rudy_1991_pe_lut_be(boost::shared_ptr<AbstractIvpOdeSolver> /* unused; should be empty */, boost::shared_ptr<AbstractStimulusFunction> pIntracellularStimulus);
    ~CML_luo_rudy_1991_pe_lut_be();
    
private:
    // Lookup table indices
    unsigned _table_index_0;
    double _factor_0;
    double* _lt_0_row;
    
    
public:
    void VerifyStateVariables();
    double GetIIonic();
    void ComputeResidual(double var_environment__time, const double rCurrentGuess[1], double rResidual[1]);
    void ComputeJacobian(double var_environment__time, const double rCurrentGuess[1], double rJacobian[1][1]);
protected:
    void UpdateTransmembranePotential(double var_environment__time);
    void ComputeOneStepExceptVoltage(double var_environment__time);
    std::vector<double> ComputeDerivedQuantities(double var_environment__time, const std::vector<double> & rY);
};


// Needs to be included last
#include "SerializationExportWrapper.hpp"
CHASTE_CLASS_EXPORT(CML_luo_rudy_1991_pe_lut_be)

namespace boost
{
    namespace serialization
    {
        template<class Archive>
        inline void save_construct_data(
            Archive & ar, const CML_luo_rudy_1991_pe_lut_be * t, const unsigned int fileVersion)
        {
            const boost::shared_ptr<AbstractIvpOdeSolver> p_solver = t->GetSolver();
            const boost::shared_ptr<AbstractStimulusFunction> p_stimulus = t->GetStimulusFunction();
            ar << p_solver;
            ar << p_stimulus;
        }
        
        template<class Archive>
        inline void load_construct_data(
            Archive & ar, CML_luo_rudy_1991_pe_lut_be * t, const unsigned int fileVersion)
        {
            boost::shared_ptr<AbstractIvpOdeSolver> p_solver;
            boost::shared_ptr<AbstractStimulusFunction> p_stimulus;
            ar >> p_solver;
            ar >> p_stimulus;
            ::new(t)CML_luo_rudy_1991_pe_lut_be(p_solver, p_stimulus);
        }
        
    }
    
}

#endif // CML_LUO_RUDY_1991_PE_LUT_BE_HPP_
