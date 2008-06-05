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

#include "FastSlowLuoRudyIModel1991.hpp"
#include <cmath>

//#include <iostream>
#include "Exception.hpp"

/**
 * Constructor - sets up state variables and initial condition
 */
FastSlowLuoRudyIModel1991::FastSlowLuoRudyIModel1991(AbstractIvpOdeSolver *pSolver,
                                                     double dt,
                                                     AbstractStimulusFunction *pIntracellularStimulus,
                                                     AbstractStimulusFunction *pExtracellularStimulus)
        : AbstractFastSlowCardiacCell(pSolver, 8, 4, dt, pIntracellularStimulus, pExtracellularStimulus)
{
    // NOTE: above we set the number of state variables to be 8 (the second arg to AbstractCardiacCell),
    // but we don't know what the correct is until SetState is called. So we MUST correctly set 
    // mNumberOfStateVariables in SetState.
}


void FastSlowLuoRudyIModel1991::SetState(CellModelState state)
{
    assert(state != STATE_UNSET);
    assert(mState == STATE_UNSET); // SetState() has been called twice if this fails

    mState = state;
    
    // set mNumberOfStateVariables correctly (see comment in constructor)
    if(mState == FAST)
    {
        mNumberOfStateVariables = 6;
    }
    else
    {
        mNumberOfStateVariables = 8;
    } 
    
    // set the final paramter
    fast_sodium_current_E_Na = ((membrane_R * membrane_T) / membrane_F) *
                               log(ionic_concentrations_Nao / ionic_concentrations_Nai);
    
    // State variables
    mVariableNames.push_back("h");
    mVariableUnits.push_back("");
    mInitialConditions.push_back(0.9804713);
    
    mVariableNames.push_back("j");
    mVariableUnits.push_back("");
    mInitialConditions.push_back(0.98767124);
    
    mVariableNames.push_back("m");
    mVariableUnits.push_back("");
    mInitialConditions.push_back(0.00187018);
    
    mVariableNames.push_back("CaI");
    mVariableUnits.push_back("mMol");
    mInitialConditions.push_back(0.0002);
    
    mVariableNames.push_back("V");
    mVariableUnits.push_back("mV");
    mInitialConditions.push_back(-83.853);
       
    if(mState == SLOW)
    {
        mVariableNames.push_back("d");
        mVariableUnits.push_back("");
        mInitialConditions.push_back(0.00316354);
        
        mVariableNames.push_back("f");
        mVariableUnits.push_back("");
        mInitialConditions.push_back(0.99427859);
    }

    mVariableNames.push_back("x");
    mVariableUnits.push_back("");
    mInitialConditions.push_back(0.16647703);
    
    AbstractCardiacCell::Init();
}

/**
 * Destructor
 */
FastSlowLuoRudyIModel1991::~FastSlowLuoRudyIModel1991(void)
{    
}

double FastSlowLuoRudyIModel1991::GetIntracellularCalciumConcentration()
{
    return mStateVariables[3];
}

/**
 * Fill in a vector representing the RHS of the LuoRudyIModel1991OdeSystem system
 * of Odes at each time step, y' = [y1' ... yn'].
 * Some ODE solver will call this function repeatedly to solve for y = [y1 ... yn].
 *
 * @param time  the current time, in milliseconds
 * @param rY  current values of the state variables
 * @param rDY  to be filled in with derivatives
 */
void FastSlowLuoRudyIModel1991::EvaluateYDerivatives(double time,
                                                     const std::vector<double> &rY,
                                                     std::vector<double> &rDY)
{
    assert(mState!=STATE_UNSET); // SetState() hasn't been called if this fails
    
    if(mState==FAST)
    {
        assert(mSlowValues.size()==2); // verify that SetSlowValues() has been called.
    }
    
    double fast_sodium_current_h_gate_h = rY[0];
    double fast_sodium_current_j_gate_j = rY[1];
    double fast_sodium_current_m_gate_m = rY[2];
    double intracellular_calcium_concentration_Cai = rY[3];
    double membrane_V = rY[4];
    double slow_inward_current_d_gate_d;
    double slow_inward_current_f_gate_f;
    double time_dependent_potassium_current_X_gate_X;
    
    // set up last three variable depending on which mode we are in
    if(mState==FAST)
    {
        slow_inward_current_d_gate_d = mSlowValues[0];
        slow_inward_current_f_gate_f = mSlowValues[1];
        time_dependent_potassium_current_X_gate_X = rY[5];
    }
    else
    {
        slow_inward_current_d_gate_d = rY[5];
        slow_inward_current_f_gate_f = rY[6];
        time_dependent_potassium_current_X_gate_X = rY[7];
    }
    
    VerifyStateVariables();    
    
    double background_current_i_b = background_current_g_b*(membrane_V-background_current_E_b);
    
    double fast_sodium_current_h_gate_alpha_h;
    
    if (membrane_V < -40.0)
    {
        fast_sodium_current_h_gate_alpha_h = 0.135*exp((80.0+membrane_V)/-6.8);
    }
    else
    {
        fast_sodium_current_h_gate_alpha_h = 0.0;
    }
    
    double fast_sodium_current_h_gate_beta_h;
    
    if (membrane_V < -40.0)
    {
        fast_sodium_current_h_gate_beta_h = 3.56*exp(0.079*membrane_V)+3.1e5*exp(0.35*membrane_V);
    }
    else
    {
        fast_sodium_current_h_gate_beta_h = 1.0/(0.13*(1.0+exp((membrane_V+10.66)/-11.1)));
    }
    
    double fast_sodium_current_h_gate_h_prime = fast_sodium_current_h_gate_alpha_h*(1.0-fast_sodium_current_h_gate_h)-fast_sodium_current_h_gate_beta_h*fast_sodium_current_h_gate_h;
    
    double fast_sodium_current_j_gate_alpha_j;
    
    if (membrane_V < -40.0)
    {
        fast_sodium_current_j_gate_alpha_j = (-1.2714e5*exp(0.2444*membrane_V)-3.474e-5*exp(-0.04391*membrane_V))*(membrane_V+37.78)/(1.0+exp(0.311*(membrane_V+79.23)));
    }
    else
    {
        fast_sodium_current_j_gate_alpha_j = 0.0;
    }
    
    double fast_sodium_current_j_gate_beta_j;
    
    if (membrane_V < -40.0)
    {
        fast_sodium_current_j_gate_beta_j = 0.1212*exp(-0.01052*membrane_V)/(1.0+exp(-0.1378*(membrane_V+40.14)));
    }
    else
    {
        fast_sodium_current_j_gate_beta_j = 0.3*exp(-2.535e-7*membrane_V)/(1.0+exp(-0.1*(membrane_V+32.0)));
    }
    
    double fast_sodium_current_j_gate_j_prime = fast_sodium_current_j_gate_alpha_j*(1.0-fast_sodium_current_j_gate_j)-fast_sodium_current_j_gate_beta_j*fast_sodium_current_j_gate_j;
    double fast_sodium_current_m_gate_alpha_m = 0.32*(membrane_V+47.13)/(1.0-exp(-0.1*(membrane_V+47.13)));
    double fast_sodium_current_m_gate_beta_m = 0.08*exp(-membrane_V/11.0);
    double fast_sodium_current_m_gate_m_prime = fast_sodium_current_m_gate_alpha_m*(1.0-fast_sodium_current_m_gate_m)-fast_sodium_current_m_gate_beta_m*fast_sodium_current_m_gate_m;
    double fast_sodium_current_i_Na = fast_sodium_current_g_Na*pow(fast_sodium_current_m_gate_m, 3.0)*fast_sodium_current_h_gate_h*fast_sodium_current_j_gate_j*(membrane_V-fast_sodium_current_E_Na);
    
    double slow_inward_current_E_si = 7.7-13.0287*log(intracellular_calcium_concentration_Cai);
    double slow_inward_current_i_si = 0.09*slow_inward_current_d_gate_d*slow_inward_current_f_gate_f*(membrane_V-slow_inward_current_E_si);
    double intracellular_calcium_concentration_Cai_prime = -1e-4*slow_inward_current_i_si+0.07*(1e-4-intracellular_calcium_concentration_Cai);
    double time_dependent_potassium_current_g_K = 0.282*sqrt(ionic_concentrations_Ko/5.4);
    
    double time_dependent_potassium_current_Xi_gate_Xi;
    
    if (membrane_V > -100.0)
    {
        time_dependent_potassium_current_Xi_gate_Xi = 2.837*(exp(0.04*(membrane_V+77.0))-1.0)/((membrane_V+77.0)*exp(0.04*(membrane_V+35.0)));
    }
    else
    {
        #define COVERAGE_IGNORE
        time_dependent_potassium_current_Xi_gate_Xi = 1.0;
        #undef COVERAGE_IGNORE
    }
    
    double time_dependent_potassium_current_X_gate_alpha_X = 0.0005*exp(0.083*(membrane_V+50.0))/(1.0+exp(0.057*(membrane_V+50.0)));
    double time_dependent_potassium_current_X_gate_beta_X = 0.0013*exp(-0.06*(membrane_V+20.0))/(1.0+exp(-0.04*(membrane_V+20.0)));
    double time_dependent_potassium_current_X_gate_X_prime = time_dependent_potassium_current_X_gate_alpha_X*(1.0-time_dependent_potassium_current_X_gate_X)-time_dependent_potassium_current_X_gate_beta_X*time_dependent_potassium_current_X_gate_X;
    
    double time_dependent_potassium_current_E_K = ((membrane_R*membrane_T)/membrane_F)*log((ionic_concentrations_Ko+time_dependent_potassium_current_PR_NaK*ionic_concentrations_Nao)/(ionic_concentrations_Ki+time_dependent_potassium_current_PR_NaK*ionic_concentrations_Nai));
    double time_dependent_potassium_current_i_K = time_dependent_potassium_current_g_K*time_dependent_potassium_current_X_gate_X*time_dependent_potassium_current_Xi_gate_Xi*(membrane_V-time_dependent_potassium_current_E_K);
    double time_independent_potassium_current_g_K1 = 0.6047*sqrt(ionic_concentrations_Ko/5.4);
    double time_independent_potassium_current_E_K1 =((membrane_R*membrane_T)/membrane_F)*log(ionic_concentrations_Ko/ionic_concentrations_Ki);
    double time_independent_potassium_current_K1_gate_alpha_K1 = 1.02/(1.0+exp(0.2385*(membrane_V-time_independent_potassium_current_E_K1-59.215)));
    double time_independent_potassium_current_K1_gate_beta_K1 = (0.49124*exp(0.08032*(membrane_V+5.476-time_independent_potassium_current_E_K1))+exp(0.06175*(membrane_V-(time_independent_potassium_current_E_K1+594.31))))/(1.0+exp(-0.5143*(membrane_V-time_independent_potassium_current_E_K1+4.753)));
    double time_independent_potassium_current_K1_gate_K1_infinity = time_independent_potassium_current_K1_gate_alpha_K1/(time_independent_potassium_current_K1_gate_alpha_K1+time_independent_potassium_current_K1_gate_beta_K1);
    double time_independent_potassium_current_i_K1 = time_independent_potassium_current_g_K1*time_independent_potassium_current_K1_gate_K1_infinity*(membrane_V-time_independent_potassium_current_E_K1);
    double plateau_potassium_current_Kp = 1.0/(1.0+exp((7.488-membrane_V)/5.98));
    double plateau_potassium_current_E_Kp = time_independent_potassium_current_E_K1;
    double plateau_potassium_current_i_Kp = plateau_potassium_current_g_Kp*plateau_potassium_current_Kp*(membrane_V-plateau_potassium_current_E_Kp);
    double i_stim = GetStimulus(time);
    
    //calculate dV
    double membrane_V_prime = (-1.0/membrane_C)*(fast_sodium_current_i_Na+slow_inward_current_i_si+time_dependent_potassium_current_i_K+time_independent_potassium_current_i_K1+plateau_potassium_current_i_Kp+background_current_i_b + i_stim);
    // do not update voltage if the mSetVoltageDerivativeToZero flag has been set
    if (mSetVoltageDerivativeToZero)
    {
        membrane_V_prime = 0;
    }
    
    rDY[0] = fast_sodium_current_h_gate_h_prime;
    rDY[1] = fast_sodium_current_j_gate_j_prime;
    rDY[2] = fast_sodium_current_m_gate_m_prime;
    rDY[3] = intracellular_calcium_concentration_Cai_prime;
    rDY[4] = membrane_V_prime;

    if(mState==SLOW)
    {
        double slow_inward_current_d_gate_alpha_d = 0.095*exp(-0.01*(membrane_V-5.0))/(1.0+exp(-0.072*(membrane_V-5.0)));
        double slow_inward_current_d_gate_beta_d = 0.07*exp(-0.017*(membrane_V+44.0))/(1.0+exp(0.05*(membrane_V+44.0)));
        double slow_inward_current_d_gate_d_prime = slow_inward_current_d_gate_alpha_d*(1.0-slow_inward_current_d_gate_d)-slow_inward_current_d_gate_beta_d*slow_inward_current_d_gate_d;
        
        double slow_inward_current_f_gate_alpha_f = 0.012*exp(-0.008*(membrane_V+28.0))/(1.0+exp(0.15*(membrane_V+28.0)));
        double slow_inward_current_f_gate_beta_f = 0.0065*exp(-0.02*(membrane_V+30.0))/(1.0+exp(-0.2*(membrane_V+30.0)));
        double slow_inward_current_f_gate_f_prime = slow_inward_current_f_gate_alpha_f*(1.0-slow_inward_current_f_gate_f)-slow_inward_current_f_gate_beta_f*slow_inward_current_f_gate_f;

        rDY[5] = slow_inward_current_d_gate_d_prime;
        rDY[6] = slow_inward_current_f_gate_f_prime;
        rDY[7] = time_dependent_potassium_current_X_gate_X_prime;
    }
    else
    {
        rDY[5] = time_dependent_potassium_current_X_gate_X_prime;
    }
}

void FastSlowLuoRudyIModel1991::SetSlowValues(const std::vector<double> &rSlowValues)
{
    assert(mState!=STATE_UNSET); // SetState() hasn't been called if this fails

    assert(mState==FAST);
    assert(rSlowValues.size() == 2);
    
    double slow_inward_current_d_gate_d = rSlowValues[0];
    double slow_inward_current_f_gate_f = rSlowValues[1];
    
    if (!(0.0<=slow_inward_current_d_gate_d && slow_inward_current_d_gate_d<=1.0))
    {
        EXCEPTION(DumpState("d gate for slow inward current has gone out of range. Check model parameters, for example spatial stepsize"));
    }
    
    if (!(0.0<=slow_inward_current_f_gate_f && slow_inward_current_f_gate_f<=1.0))
    {
        EXCEPTION(DumpState("f gate for slow inward current has gone out of range. Check model parameters, for example spatial stepsize"));
    }
    
    mSlowValues = rSlowValues;
}

void FastSlowLuoRudyIModel1991::GetSlowValues(std::vector<double> &rSlowValues)
{
    assert(mState!=STATE_UNSET); // SetState() hasn't been called if this fails

    assert(mState==SLOW);
    rSlowValues.resize(2);
    rSlowValues[0] = mStateVariables[5];
    rSlowValues[1] = mStateVariables[6];
}
    

double FastSlowLuoRudyIModel1991::GetIIonic()
{
    assert(mState!=STATE_UNSET); // SetState() hasn't been called if this fails

    double fast_sodium_current_h_gate_h = mStateVariables[0];
    double fast_sodium_current_j_gate_j = mStateVariables[1];
    double fast_sodium_current_m_gate_m = mStateVariables[2];
    double intracellular_calcium_concentration_Cai = mStateVariables[3];
    double membrane_V = mStateVariables[4];

    double slow_inward_current_d_gate_d;
    double slow_inward_current_f_gate_f;
    double time_dependent_potassium_current_X_gate_X;
    
    if(mState==FAST)
    {
        slow_inward_current_d_gate_d = mSlowValues[0];
        slow_inward_current_f_gate_f = mSlowValues[1];
        time_dependent_potassium_current_X_gate_X = mStateVariables[5];
    }
    else
    {
        slow_inward_current_d_gate_d = mStateVariables[5];
        slow_inward_current_f_gate_f = mStateVariables[6];
        time_dependent_potassium_current_X_gate_X = mStateVariables[7];
    }
    
    /*
     * Compute the LuoRudyIModel1991OdeSystem model
     */
    double background_current_i_b = background_current_g_b*(membrane_V-background_current_E_b);
    
    double fast_sodium_current_i_Na = fast_sodium_current_g_Na*pow(fast_sodium_current_m_gate_m, 3.0)*fast_sodium_current_h_gate_h*fast_sodium_current_j_gate_j*(membrane_V-fast_sodium_current_E_Na);
    
    double slow_inward_current_E_si = 7.7-13.0287*log(intracellular_calcium_concentration_Cai);
    double slow_inward_current_i_si = 0.09*slow_inward_current_d_gate_d*slow_inward_current_f_gate_f*(membrane_V-slow_inward_current_E_si);
    
    double time_dependent_potassium_current_g_K = 0.282*sqrt(ionic_concentrations_Ko/5.4);
    double time_dependent_potassium_current_Xi_gate_Xi;
    
    // Although the equation below looks strange (particularly the arguments of the
    // exponentials, it is in fact correct.
    if (membrane_V > -100.0)
    {
        time_dependent_potassium_current_Xi_gate_Xi = 2.837*(exp(0.04*(membrane_V+77.0))-1.0)/((membrane_V+77.0)*exp(0.04*(membrane_V+35.0)));
    }
    else
    {
        #define COVERAGE_IGNORE
        time_dependent_potassium_current_Xi_gate_Xi = 1.0;
        #undef COVERAGE_IGNORE
    }
    double time_dependent_potassium_current_E_K = ((membrane_R*membrane_T)/membrane_F)*log((ionic_concentrations_Ko+time_dependent_potassium_current_PR_NaK*ionic_concentrations_Nao)/(ionic_concentrations_Ki+time_dependent_potassium_current_PR_NaK*ionic_concentrations_Nai));
    double time_dependent_potassium_current_i_K = time_dependent_potassium_current_g_K*time_dependent_potassium_current_X_gate_X*time_dependent_potassium_current_Xi_gate_Xi*(membrane_V-time_dependent_potassium_current_E_K);
    
    double time_independent_potassium_current_g_K1 = 0.6047*sqrt(ionic_concentrations_Ko/5.4);
    double time_independent_potassium_current_E_K1 =((membrane_R*membrane_T)/membrane_F)*log(ionic_concentrations_Ko/ionic_concentrations_Ki);
    double time_independent_potassium_current_K1_gate_alpha_K1 = 1.02/(1.0+exp(0.2385*(membrane_V-time_independent_potassium_current_E_K1-59.215)));
    double time_independent_potassium_current_K1_gate_beta_K1 = (0.49124*exp(0.08032*(membrane_V+5.476-time_independent_potassium_current_E_K1))+exp(0.06175*(membrane_V-(time_independent_potassium_current_E_K1+594.31))))/(1.0+exp(-0.5143*(membrane_V-time_independent_potassium_current_E_K1+4.753)));
    double time_independent_potassium_current_K1_gate_K1_infinity = time_independent_potassium_current_K1_gate_alpha_K1/(time_independent_potassium_current_K1_gate_alpha_K1+time_independent_potassium_current_K1_gate_beta_K1);
    double time_independent_potassium_current_i_K1 = time_independent_potassium_current_g_K1*time_independent_potassium_current_K1_gate_K1_infinity*(membrane_V-time_independent_potassium_current_E_K1);
    
    double plateau_potassium_current_Kp = 1.0/(1.0+exp((7.488-membrane_V)/5.98));
    double plateau_potassium_current_E_Kp = time_independent_potassium_current_E_K1;
    double plateau_potassium_current_i_Kp = plateau_potassium_current_g_Kp*plateau_potassium_current_Kp*(membrane_V-plateau_potassium_current_E_Kp);
    
    double i_ionic = fast_sodium_current_i_Na+slow_inward_current_i_si+time_dependent_potassium_current_i_K+time_independent_potassium_current_i_K1+plateau_potassium_current_i_Kp+background_current_i_b;
    
    assert(!isnan(i_ionic));
    return i_ionic;
}

void FastSlowLuoRudyIModel1991::VerifyStateVariables()
{
    assert(mState!=STATE_UNSET); // SetState() hasn't been called if this fails

//#ifndef NDEBUG
    const std::vector<double>& rY = rGetStateVariables();
 
    const double fast_sodium_current_h_gate_h = rY[0];            // gating
    const double fast_sodium_current_j_gate_j = rY[1];            // gating
    const double fast_sodium_current_m_gate_m = rY[2];            // gating
    const double intracellular_calcium_concentration_Cai = rY[3]; // concentration

    double slow_inward_current_d_gate_d;
    double slow_inward_current_f_gate_f;
    double time_dependent_potassium_current_X_gate_X;
    
    if(mState==FAST)
    {
        slow_inward_current_d_gate_d = mSlowValues[0];
        slow_inward_current_f_gate_f = mSlowValues[1];
        time_dependent_potassium_current_X_gate_X = rY[5];
    }
    else
    {
        slow_inward_current_d_gate_d = rY[5];
        slow_inward_current_f_gate_f = rY[6];
        time_dependent_potassium_current_X_gate_X = rY[7];
    }
    

    #define COVERAGE_IGNORE
    if (!(0.0<=fast_sodium_current_h_gate_h && fast_sodium_current_h_gate_h<=1.0))
    {
        EXCEPTION(DumpState("h gate for fast sodium current has gone out of range. Check model parameters, for example spatial stepsize"));
    }
    
    if (!(0.0<=fast_sodium_current_j_gate_j && fast_sodium_current_j_gate_j<=1.0))
    {
        EXCEPTION(DumpState("j gate for fast sodium current has gone out of range. Check model parameters, for example spatial stepsize"));
    }
    
    if (!(0.0<=fast_sodium_current_m_gate_m && fast_sodium_current_m_gate_m<=1.0))
    {
        EXCEPTION(DumpState("m gate for fast sodium current has gone out of range. Check model parameters, for example spatial stepsize"));
    }

    if (!(0.0<intracellular_calcium_concentration_Cai))
    {
        EXCEPTION(DumpState("intracellular_calcium_concentration_Cai has become non-positive, ie gone out of range. Check model parameters, for example spatial stepsize"));
    }
    
    if (!(0.0<=time_dependent_potassium_current_X_gate_X && time_dependent_potassium_current_X_gate_X<=1.0))
    {
        EXCEPTION(DumpState("X gate for time dependent potassium current has gone out of range. Check model parameters, for example spatial stepsize"));
    }
    
    if(mState==SLOW)
    {
        if (!(0.0<=slow_inward_current_d_gate_d && slow_inward_current_d_gate_d<=1.0))
        {
            EXCEPTION(DumpState("d gate for slow inward current has gone out of range. Check model parameters, for example spatial stepsize"));
        }
    
        if (!(0.0<=slow_inward_current_f_gate_f && slow_inward_current_f_gate_f<=1.0))
        {
            EXCEPTION(DumpState("f gate for slow inward current has gone out of range. Check model parameters, for example spatial stepsize"));
        }
    }
    #undef COVERAGE_IGNORE
//#endif
}
