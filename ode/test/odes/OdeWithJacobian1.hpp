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


#ifndef _ODEWITHJACOBIAN1_HPP_
#define _ODEWITHJACOBIAN1_HPP_

#include "AbstractOdeSystemWithAnalyticJacobian.hpp"
#include "OdeSystemInformation.hpp"

/*
 * Concrete Jacobian class
 */
class OdeWithJacobian1 : public AbstractOdeSystemWithAnalyticJacobian
{
public :
    OdeWithJacobian1()
            : AbstractOdeSystemWithAnalyticJacobian(1) // 1 here is the number of variables
    {
        mpSystemInfo = OdeSystemInformation<OdeWithJacobian1>::Instance();
    }

    void EvaluateYDerivatives(double time, const std::vector<double>& rY, std::vector<double>& rDY)
    {
        rDY[0] = rY[0]*rY[0];
    }

    void AnalyticJacobian(const std::vector<double>& rSolutionGuess, double** jacobian, double time, double timeStep)
    {
        jacobian[0][0] = 1 - 2.0*timeStep*rSolutionGuess[0];
    }
};

template<>
void OdeSystemInformation<OdeWithJacobian1>::Initialise()
{
    this->mVariableNames.push_back("Variable 1");
    this->mVariableUnits.push_back("dimensionless");
    this->mInitialConditions.push_back(0.0);

    this->mInitialised = true;
}


#endif //_ODEWITHJACOBIAN1_HPP_
