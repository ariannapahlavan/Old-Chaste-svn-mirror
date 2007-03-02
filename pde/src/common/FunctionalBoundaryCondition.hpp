#ifndef _FUNCTIONALBOUNDARYCONDITION_HPP_
#define _FUNCTIONALBOUNDARYCONDITION_HPP_

#include "AbstractBoundaryCondition.hpp"

/**
 * A boundary condition that takes a function pointer in its constructor, and
 * evaluates the function to determine the value of the condition at a given
 * point.
 */
template<unsigned SPACE_DIM>
class FunctionalBoundaryCondition : public AbstractBoundaryCondition<SPACE_DIM>
{
private :
    double (*mFunction)(const Point<SPACE_DIM> x);
    
public :
    /**
     * Typical use:
     *  pBoundaryCondition = new FunctionalBoundaryCondition(&function_name);
     * 
     * @param func Pointer to a function to be used for evaluating this boundary
     *     condition.
     */
    FunctionalBoundaryCondition(double (*func)(const Point<SPACE_DIM> x)) : mFunction(func)
    {};
    
    double GetValue( const Point<SPACE_DIM> x) const
    {
        return mFunction(x);
        ;
    }
};

#endif //_FUNCTIONALBOUNDARYCONDITION_HPP_
