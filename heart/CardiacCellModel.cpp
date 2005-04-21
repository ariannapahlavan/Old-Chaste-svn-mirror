/**
 * CardiacCellModel.cpp
 * 
 * An  cardiac cell model.
 */
#include "CardiacCellModel.hpp"

/**
 * Constructor
 */
CardiacCellModel::CardiacCellModel()
{	
	double mTransmembranePotential = 0.0;
	double mStimulusCurrent = 0.0;
	double mTotalIonicCurrent = 0.0;
}

/**
 * Overloaded Constructor
 * 
 * @param transmembranePotential Initial transmembrane potential
 */
CardiacCellModel::CardiacCellModel(double transmembranePotential)
{	
	double mTransmembranePotential = transmembranePotential;
	double mStimulusCurrent = 0.0;
	double mTotalIonicCurrent = 0.0;
}

/**
 * Destructor
 */
CardiacCellModel::~CardiacCellModel()
{	
	// Do nothing
}

/**
 * Returns transmembrane potential
 */
double CardiacCellModel::GetTransmembranePotential(void)
{
	return mTransmembranePotential;
}

/**
 * Returns stimulus current
 */
double CardiacCellModel::GetStimulusCurrent(void)
{
	return mStimulusCurrent;
}

/**
 * Returns total ionic current
 */
double CardiacCellModel::GetTotalIonicCurrent(void)
{
	return mTotalIonicCurrent;
}

/**
 * Sets stimulus current
 * 
 * @param stimulusCurrent New stimulus current
 */
void CardiacCellModel::SetStimulusCurrent(double stimulusCurrent)
{
	mStimulusCurrent = stimulusCurrent;
}
