#ifndef ODECONVERGENCETESTER_HPP_
#define ODECONVERGENCETESTER_HPP_

#include "AbstractConvergenceTester.hpp"

template<class CELL, class CARDIAC_PROBLEM, unsigned DIM>
class OdeConvergenceTester : public AbstractConvergenceTester<CELL, CARDIAC_PROBLEM, DIM>
{
public:
    void SetInitialConvergenceParameters()
    {
        this->mPdeTimeStep = 2.5e-3;
        this->mOdeTimeStep = this->mPdeTimeStep;
    }
    void UpdateConvergenceParameters()
    {
        this->mOdeTimeStep *= 0.5;
    
    }
    bool GiveUpConvergence()
    {
        return this->mOdeTimeStep<=1e-8;
    }
    
    double Abscissa()
    {
        return this->mOdeTimeStep;
    }
    
};
#endif /*ODECONVERGENCETESTER_HPP_*/
