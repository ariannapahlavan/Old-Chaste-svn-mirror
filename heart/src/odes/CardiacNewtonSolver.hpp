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
#ifndef CARDIACNEWTONSOLVER_HPP_
#define CARDIACNEWTONSOLVER_HPP_

#include <cmath>

#include "AbstractBackwardEulerCardiacCell.hpp"

/**
 * Specialised Newton solver for solving the nonlinear systems arising when
 * simulating a cardiac cell using Backward Euler.
 *
 * The class is templated by the size of the nonlinear system, and uses the
 * singleton pattern to ensure only 1 solver for any given system size is created.
 * This allows us to be both computationally and memory efficient.
 *
 * It would be nice to have a test of this class directly, but you need a cardiac
 * cell in order to test it.  So all tests occur when testing particular cardiac
 * cells, e.g. the BackwardEulerLuoRudyIModel1991.
 */
template<unsigned SIZE>
class CardiacNewtonSolver
{
public:
    /**
     * Call this method to obtain a solver instance.
     *
     * @return a single instance of the class
     */
    static CardiacNewtonSolver<SIZE>* Instance()
    {
        static CardiacNewtonSolver<SIZE> inst;
        return &inst;
    }

    /**
     * Use Newton's method to solve the given cell for the next timestep.
     *
     * @param rCell  the cell to solve
     * @param rCurrentGuess  the current guess at a solution.  Will be mUpdated on exit.
     */
    void Solve(AbstractBackwardEulerCardiacCell<SIZE> &rCell,
               double rCurrentGuess[SIZE])
    {
        unsigned counter = 0;
//        const double eps = 1e-6 * rCurrentGuess[0]; // Our tolerance (should use min(guess) perhaps?)
        const double eps = 1e-6; // JonW tolerance
        double norm = 2*eps;

        // check that the initial guess that was given gives a valid residual
        rCell.ComputeResidual(rCurrentGuess, mResidual);
        for (unsigned i=0; i<SIZE; i++)
        {
            assert(!std::isnan(mResidual[i]));
        }

        while (norm > eps)
        {
            // Calculate Jacobian for current guess
            rCell.ComputeJacobian(rCurrentGuess, mJacobian);

//            // Update norm (our style)
//            norm = ComputeNorm(mResidual);

            // Solve Newton linear system
            SolveLinearSystem();

            // Update norm (JonW style)
            norm = ComputeNorm(mUpdate);

            // Update current guess and recalculate residual
            for (unsigned i=0; i<SIZE; i++)
            {
                rCurrentGuess[i] -= mUpdate[i];
            }
            rCell.ComputeResidual(rCurrentGuess, mResidual);

            counter++;
            assert(counter < 15); // avoid infinite loops
        }
    }

/////// Alternative version of Solve which uses damping factors - may be
/////// needed in the future if a system which is difficult to solve and
/////// Newton diverges. THINK THIS IS NOT CURRENTLY WORKING - compare
/////// with above solve before use.
////
////    void Solve(AbstractBackwardEulerCardiacCell<SIZE> &rCell,
////               double rCurrentGuess[SIZE])
////    {
////        unsigned counter = 0;
////        double TOL = 1e-3;// * rCurrentGuess[0]; // Our tolerance (should use min(guess) perhaps?)
////
////        rCell.ComputeResidual(rCurrentGuess, mResidual);
////        for (unsigned i=0; i<SIZE; i++)
////        {
////            assert(!isnan(mResidual[i]));
////        }
////
////        double norm = ComputeNorm(mResidual);
////
////        std::vector<double> damping_values;
////        damping_values.push_back(-0.1);
////        for (unsigned i=1; i<=12; i++)
////        {
////            double val = double(i)/10;
////            damping_values.push_back(val);
////        }
////
////        while (norm > TOL)
////        {
////            // Calculate Jacobian and mResidual for current guess
////            rCell.ComputeJacobian(rCurrentGuess, mJacobian);
////
////            // Solve Newton linear system
////            SolveLinearSystem();
////
////            // go through all the possible damping values and
////            // choose the one which gives the smallest residual-norm
////            double best_damping_value = 1;
////            double best_residual_norm = DBL_MAX;
////            for (unsigned j=0; j<damping_values.size(); j++)
////            {
////                double test_vec[SIZE];
////                for (unsigned i=0; i<SIZE; i++)
////                {
////                    assert(!isnan( mUpdate[i] ));
////                    test_vec[i] = rCurrentGuess[i] - damping_values[j]*mUpdate[i];
////                }
////                double test_resid[SIZE];
////                rCell.ComputeResidual(test_vec, test_resid);
////                double test_vec_residual_norm = ComputeNorm(test_resid);
////                // std::cout << "s,|r|,|old resid|= " << damping_values[j] << " " << test_vec_residual_norm << " " << norm << std::endl;
////                if(test_vec_residual_norm <= best_residual_norm)
////                {
////                    best_damping_value = damping_values[j];
////                    best_residual_norm = test_vec_residual_norm;
////                }
////            }
////
////            // check best residual norm was smaller than previous
////            // norm
////            assert(best_residual_norm < norm);
////
////            // apply update
////            for (unsigned i=0; i<SIZE; i++)
////            {
////                rCurrentGuess[i] -= best_damping_value*mUpdate[i];
////            }
////            norm = best_residual_norm;
////            rCell.ComputeResidual(rCurrentGuess, mResidual);
////
////            counter++;
////            assert(counter < 15); // avoid infinite loops
////        }
////    }


protected:
    CardiacNewtonSolver()
    {}
    CardiacNewtonSolver(const CardiacNewtonSolver<SIZE>&);
    CardiacNewtonSolver<SIZE>& operator= (const CardiacNewtonSolver<SIZE>&);

    /**
     * Compute a norm of a vector.
     */
    double ComputeNorm(double vector[SIZE])
    {
        double norm = 0.0;
        for (unsigned i=0; i<SIZE; i++)
        {
            if (fabs(vector[i]) > norm)
            {
                norm = fabs(vector[i]);
            }
        }
        return norm;
    }

    /**
     * Solve a linear system to calculate the Newton update step
     */
    void SolveLinearSystem()
    {
        double fact;
        for (unsigned i=0; i<SIZE; i++)
        {
            for (unsigned ii=i+1; ii<SIZE; ii++)
            {
                fact = mJacobian[ii][i]/mJacobian[i][i];
                for (unsigned j=i; j<SIZE; j++)
                {
                    mJacobian[ii][j] -= fact*mJacobian[i][j];
                }
                mResidual[ii] -= fact*mResidual[i];
            }
        }
        /*This must be int, since an unsigned down-loop wouldn't terminate*/
        for (int i=SIZE-1; i>=0; i--)
        {
            mUpdate[i] = mResidual[i];
            for (unsigned j=i+1; j<SIZE; j++)
            {
                mUpdate[i] -= mJacobian[i][j]*mUpdate[j];
            }
            mUpdate[i] /= mJacobian[i][i];
        }
    }

private:
    /** Working memory : residual vector */
    double mResidual[SIZE];
    /** Working memory : Jacobian matrix */
    double mJacobian[SIZE][SIZE];
    /** Working memory : update vector */
    double mUpdate[SIZE];
};

#endif /*CARDIACNEWTONSOLVER_HPP_*/
