#ifndef TESTCANCERPARAMETERS_HPP_
#define TESTCANCERPARAMETERS_HPP_

#include <cxxtest/TestSuite.h>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <fstream>

#include "OutputFileHandler.hpp"
#include "CancerParameters.hpp"

class TestCancerParameters : public CxxTest::TestSuite
{
public:
    void TestGettersAndSetters()
    {
        CancerParameters *inst1 = CancerParameters::Instance();
        
        inst1->SetSG2MDuration(11.0);
        inst1->SetStemCellCycleTime(-25.0);
        inst1->SetTransitCellCycleTime(-35.0);
        inst1->SetMaxTransitGenerations(666u);
        inst1->SetCryptLength(-1.0);
        inst1->SetMeinekeLambda(-2.0);
        inst1->SetApoptosisTime(0.3);
        
        CancerParameters *inst2 = CancerParameters::Instance();
        
        TS_ASSERT_DELTA(inst2->GetSG2MDuration(), 11.0 , 1e-12);
        TS_ASSERT_DELTA(inst2->GetStemCellCycleTime(), -25.0, 1e-12);
        TS_ASSERT_DELTA(inst2->GetTransitCellCycleTime(), -35.0, 1e-12);
        TS_ASSERT_EQUALS(inst2->GetMaxTransitGenerations(), 666u);
        TS_ASSERT_DELTA(inst2->GetCryptLength(), -1.0, 1e-12);
        TS_ASSERT_DELTA(inst2->GetMeinekeLambda(), -2.0, 1e-12);
        TS_ASSERT_DELTA(inst2->GetApoptosisTime(), 0.3, 1e-12);
    }

    void TestArchiveCancerParameters()
    {
        OutputFileHandler handler("archive");
        std::string archive_filename;
        archive_filename = handler.GetTestOutputDirectory() + "cancer_params.arch";
        
        // Create an ouput archive 
        {
            CancerParameters *inst1 = CancerParameters::Instance();
            // Mess up the cancer parameters
            inst1->SetSG2MDuration(11.0);
            inst1->SetStemCellCycleTime(-25.0);
            inst1->SetTransitCellCycleTime(-35.0);
            inst1->SetMaxTransitGenerations(666u);
            inst1->SetCryptLength(-1.0);
            inst1->SetMeinekeLambda(-2.0);
            inst1->SetApoptosisTime(0.3);
            
                       
            std::ofstream ofs(archive_filename.c_str());       
            boost::archive::text_oarchive output_arch(ofs);
            // save messed up parameters           
            output_arch << static_cast<const CancerParameters&>(*inst1);
            
        }
        
        {  
            CancerParameters *inst1 = CancerParameters::Instance();
            // restore to nice parameters
            inst1->SetSG2MDuration(10.0);
            inst1->SetStemCellCycleTime(24.0);
            inst1->SetTransitCellCycleTime(12.0);
            inst1->SetMaxTransitGenerations(3u);
            inst1->SetCryptLength(22.0);
            inst1->SetMeinekeLambda(30.0);
            inst1->SetApoptosisTime(0.25);

            // Create an input archive
            std::ifstream ifs(archive_filename.c_str(), std::ios::binary);       
            boost::archive::text_iarchive input_arch(ifs);
            
            // restore messed up parameters from the archive
            input_arch >> *inst1;
            // check they are messed up.
            TS_ASSERT_DELTA(inst1->GetSG2MDuration(), 11.0 , 1e-12);
            TS_ASSERT_DELTA(inst1->GetStemCellCycleTime(), -25.0, 1e-12);
            TS_ASSERT_DELTA(inst1->GetTransitCellCycleTime(), -35.0, 1e-12);
            TS_ASSERT_EQUALS(inst1->GetMaxTransitGenerations(), 666u);
            TS_ASSERT_DELTA(inst1->GetCryptLength(), -1.0, 1e-12);
            TS_ASSERT_DELTA(inst1->GetMeinekeLambda(), -2.0, 1e-12);
            TS_ASSERT_DELTA(inst1->GetApoptosisTime(), 0.3, 1e-12);
        }
    }

};

#endif /*TESTCANCERPARAMETERS_HPP_*/
