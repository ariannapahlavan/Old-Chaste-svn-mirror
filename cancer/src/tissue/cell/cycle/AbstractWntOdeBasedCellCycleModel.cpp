#include "AbstractWntOdeBasedCellCycleModel.hpp"

RungeKutta4IvpOdeSolver AbstractWntOdeBasedCellCycleModel::msSolver;

// PROTECTED FUNCTIONS

double AbstractWntOdeBasedCellCycleModel::GetDivideTime()
{
    assert(msSolver.StoppingEventOccured());
    return msSolver.GetStoppingTime() + GetWntSG2MDuration();
}

double AbstractWntOdeBasedCellCycleModel::GetWntSG2MDuration()
{   
    // overridden in subclass StochasticWntCellCycleModel
    return CancerParameters::Instance()->GetSG2MDuration();
}

// PUBLIC FUNCTIONS

void AbstractWntOdeBasedCellCycleModel::ResetModel()
{   
    AbstractOdeBasedCellCycleModel::ResetModel();
    
    assert(mpOdeSystem!=NULL);    
    // This model needs the protein concentrations and phase resetting to G0/G1.
    // Keep the Wnt pathway in the same state but reset the cell cycle part
    // Cell cycle is proteins 0 to 4 (first 5 ODEs)
    std::vector<double> init_conds = mpOdeSystem->GetInitialConditions();
    for (unsigned i = 0 ; i<5 ; i++)
    {
        mpOdeSystem->rGetStateVariables()[i] = init_conds[i];
    }
}


void AbstractWntOdeBasedCellCycleModel::UpdateCellType()
{
    assert(mpOdeSystem!=NULL);
    assert(mpCell!=NULL);
    if (SimulationTime::Instance()->GetDimensionalisedTime() > mLastTime)
    {
        EXCEPTION("WntCellCycleModel::UpdateCellType() should only be called when the cell cycle model has been evaluated to the current time\n");   
    }
    ChangeCellTypeDueToCurrentBetaCateninLevel();
}


