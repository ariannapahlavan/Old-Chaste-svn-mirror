#ifndef _ABSTRACTLINEARPARABOLICPDE_HPP_
#define _ABSTRACTLINEARPARABOLICPDE_HPP_

#include "UblasCustomFunctions.hpp"
#include "ChastePoint.hpp"
#include "Node.hpp"
#include <petscvec.h>


/**
 * AbstractLinearParabolicPde class.
 *
 * A general PDE of the form:
 * c(x) du/dt = Grad.(DiffusionTerm(x)*Grad(u))+LinearSourceTerm(x)+NonlinearSourceTerm(x, u)
 *
 */
template <unsigned SPACE_DIM>
class AbstractLinearParabolicPde
{
public:
    /**
     * The function c(x) in "c(x) du/dt = Grad.(DiffusionTerm(x)*Grad(u))+LinearSourceTerm(x)+NonlinearSourceTerm(x, u)" 
     */
    virtual double ComputeDuDtCoefficientFunction(ChastePoint<SPACE_DIM> x)=0;
    
    
    /**
    * Compute Nonlinear Source Term.
    * @param x The point in space at which the Nonlinear Source Term is computed.
    */
    virtual double ComputeNonlinearSourceTerm(ChastePoint<SPACE_DIM> x,
                                              double u)=0;

    virtual double ComputeNonlinearSourceTermAtNode(const Node<SPACE_DIM>& node, double u)
    {
        return ComputeNonlinearSourceTerm(node.GetPoint(), u);
    }


    /**
     * Compute Linear Source Term.
     * @param x The point in space at which the Linear Source Term is computed.
     */
    virtual double ComputeLinearSourceTerm(ChastePoint<SPACE_DIM> x)=0;
                                              
    /**
     * Compute Diffusion Term.
     * @param x The point in space at which the Diffusion Term is computed.
     * @return A matrix. 
     */
    virtual c_matrix<double, SPACE_DIM, SPACE_DIM> ComputeDiffusionTerm(ChastePoint<SPACE_DIM> x)=0;
    
    virtual double ComputeLinearSourceTermAtNode(const Node<SPACE_DIM>& node)
    {
        return ComputeLinearSourceTerm(node.GetPoint());
    }
    
    virtual ~AbstractLinearParabolicPde()
    {}
};

#endif //_ABSTRACTLINEARPARABOLICPDE_HPP_
