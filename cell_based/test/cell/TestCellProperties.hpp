/*

Copyright (c) 2005-2012, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef TESTCELLPROPERTIES_HPP_
#define TESTCELLPROPERTIES_HPP_

#include <cxxtest/TestSuite.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "CellId.hpp"

#include "CellPropertyRegistry.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "AbstractCellBasedTestSuite.hpp"
#include "OutputFileHandler.hpp"
#include "SmartPointers.hpp"

class TestCellProperties : public AbstractCellBasedTestSuite
{
public:

    void TestCellIdMethods() throw(Exception)
    {
        // Resetting the Maximum cell Id to zero (to account for previous tests)
        CellId::ResetMaxCellId();

       	MAKE_PTR(CellId, p_cell_id);

        TS_ASSERT_THROWS_THIS(p_cell_id->GetCellId(), "AssignCellId must be called before using the CellID");
        TS_ASSERT_THROWS_THIS(p_cell_id->GetMaxCellId(), "AssignCellId must be called before using the CellID");

        // The ID is not assigned until this method is called.
    	p_cell_id->AssignCellId();

        TS_ASSERT_EQUALS(p_cell_id->GetCellId(), 0u);
        TS_ASSERT_EQUALS(p_cell_id->GetMaxCellId(), 1u);

    }

    void TestArchiveCellId() throw(Exception)
    {
    	/* In this test the Max Cell Id starts at 2 as continues from previous test.
    	 * If you add more tests this will fail.
    	 */

    	OutputFileHandler handler("archive", false);
        std::string archive_filename = handler.GetOutputDirectoryFullPath() + "mutation.arch";

        // Archive a mutation state
        {
        	CellId* p_cell_id = new CellId();
        	p_cell_id->AssignCellId();

            TS_ASSERT_EQUALS(p_cell_id->GetCellId(), 1u);
            TS_ASSERT_EQUALS(p_cell_id->GetMaxCellId(), 2u);

            // Create an output archive
            std::ofstream ofs(archive_filename.c_str());
            boost::archive::text_oarchive output_arch(ofs);

            // Write the cell to the archive
            const AbstractCellProperty* const p_const_cell_id = p_cell_id;
            output_arch << p_const_cell_id;

            delete p_cell_id;
        }

        // Restore mutation state
        {
            AbstractCellProperty* p_cell_id;

            // Restore the Cell ID
            std::ifstream ifs(archive_filename.c_str());
            boost::archive::text_iarchive input_arch(ifs);

            input_arch >> p_cell_id;

            CellId* p_real_cell_id = dynamic_cast<CellId*>(p_cell_id);
            TS_ASSERT(p_real_cell_id != NULL);

            TS_ASSERT_EQUALS(p_real_cell_id->GetCellId(), 1u);
            TS_ASSERT_EQUALS(p_real_cell_id->GetMaxCellId(), 2u);

            // Tidy up
            delete p_cell_id;
        }
    }
};

#endif /* TESTCELLPROPERTIES_HPP_ */
