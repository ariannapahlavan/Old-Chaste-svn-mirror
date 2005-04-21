#include "RungeKutta4IvpOdeSolver.hpp"
#include "AbstractIvpOdeSolver.hpp"
#include "AbstractOdeSystem.hpp"
#include "OdeSolution.hpp"

#include <iostream>
#include <vector>

/*
 * OdeSolution is an object containing an integer of the number of equations, 
 * a std::vector of times and a std::vector of std::vectors of the solution 
 * of the ODE system at those times
*/

OdeSolution RungeKutta4IvpOdeSolver::Solve(AbstractOdeSystem* pAbstractOdeSystem, 
				double startTime,
				double endTime,
				double timeStep,
				std::vector<double> initialConditions)
{

    int num_equations = pAbstractOdeSystem->mNumberOfEquations;
    int num_timesteps = ((int) ((endTime - startTime)/timeStep));
    
    double last_timestep = endTime - ((double) num_timesteps)*timeStep;
    
	OdeSolution solutions;
	solutions.mNumberOfTimeSteps = num_timesteps;
		
	solutions.mSolutions.push_back(initialConditions);
	solutions.mTime.push_back(startTime);
	
	std::vector<double> row(num_equations);	
	std::vector<double> k1(num_equations);
	std::vector<double> k2(num_equations);
	std::vector<double> k3(num_equations);
	std::vector<double> k4(num_equations);
	
	std::vector<double> yk2(num_equations);
	std::vector<double> yk3(num_equations);
	std::vector<double> yk4(num_equations);
	
	row=initialConditions;
	
	for(int timeindex=0;timeindex<num_timesteps;timeindex++)
	{
		// Apply RungeKutta4's method
		
		
        k1 = pAbstractOdeSystem->EvaluateYDerivatives(solutions.mTime[timeindex],row);
        
        for(int i=0;i<num_equations; i++) 
		{
			yk2[i] = row[i] + 0.5*k1[i];		
		}
        k2 = pAbstractOdeSystem->EvaluateYDerivatives(solutions.mTime[timeindex]+0.5*timeStep,yk2);
		
		for(int i=0;i<num_equations; i++) 
		{
			yk3[i] = row[i] + 0.5*k2[i];		
		}
        k3 = pAbstractOdeSystem->EvaluateYDerivatives(solutions.mTime[timeindex]+0.5*timeStep,yk3);        

		for(int i=0;i<num_equations; i++) 
		{
			yk4[i] = row[i] + k3[i];		
		}
        k4 = pAbstractOdeSystem->EvaluateYDerivatives(solutions.mTime[timeindex]+timeStep,yk4);                
		
		for(int i=0;i<num_equations; i++) 
		{
			row[i] = row[i] + timeStep*(k1[i]+2*k2[i]+2*k3[i]+k4[i])/6.0;		
		}
		
		solutions.mSolutions.push_back(row);
		
		solutions.mTime.push_back(solutions.mTime[timeindex]+timeStep);
	}
	
	// Extra step to get to exactly endTime
	if(last_timestep>0.00001)
	{	
		solutions.mNumberOfTimeSteps=num_timesteps+1;
		
		
		k1 = pAbstractOdeSystem->EvaluateYDerivatives(solutions.mTime[num_timesteps],row);
        
        for(int i=0;i<num_equations; i++) 
		{
			yk2[i] = row[i] + 0.5*k1[i];		
		}
        k2 = pAbstractOdeSystem->EvaluateYDerivatives(solutions.mTime[num_timesteps]+0.5*last_timestep,yk2);
		
		for(int i=0;i<num_equations; i++) 
		{
			yk3[i] = row[i] + 0.5*k2[i];		
		}
        k3 = pAbstractOdeSystem->EvaluateYDerivatives(solutions.mTime[num_timesteps]+0.5*last_timestep,yk3);        

		for(int i=0;i<num_equations; i++) 
		{
			yk4[i] = row[i] + k3[i];		
		}
        k4 = pAbstractOdeSystem->EvaluateYDerivatives(solutions.mTime[num_timesteps]+last_timestep,yk4);                
		
		for(int i=0;i<num_equations; i++) 
		{
			row[i] = row[i] + last_timestep*(k1[i]+2*k2[i]+2*k3[i]+k4[i])/6.0;		
		}
		
		
		solutions.mSolutions.push_back(row);
		
		solutions.mTime.push_back(solutions.mTime[num_timesteps]+last_timestep);
	
	}
	
			
	return solutions;
}
