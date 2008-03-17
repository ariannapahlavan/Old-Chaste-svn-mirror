#ifndef TESTHDF5DATAWRITER_HPP_
#define TESTHDF5DATAWRITER_HPP_

#include <hdf5.h>
#include <cxxtest/TestSuite.h>

#include "PetscSetupAndFinalize.hpp"
#include "OutputFileHandler.hpp"
#include "PetscTools.hpp"
#include "Hdf5DataWriter.hpp"

class TestHdf5DataWriter : public CxxTest::TestSuite
{
private:
    Hdf5DataWriter *mpTestWriter;    
    
public:
    void TestSimpleParallelWriteDirectlyWithHdf5()
    {
        // File to write
        OutputFileHandler oh("hdf5");
        std::string results_dir = oh.GetOutputDirectoryFullPath();
        std::string file_name = results_dir + "test.h5";
        
        // Set up a property list saying how we'll open the file
        hid_t plist_id = H5Pcreate(H5P_FILE_ACCESS);
        H5Pset_fapl_mpio(plist_id, PETSC_COMM_WORLD, MPI_INFO_NULL);
        
        // Create a file (collectively) and free the property list
        hid_t file_id = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
        H5Pclose(plist_id);
        
        // Initialise the data for this process
        const unsigned DIMS = 2;
        const unsigned X = 2;
        const unsigned Y = 5;
        int data[X][Y];
        for (unsigned i=0; i<X; i++)
        {
            for (unsigned j=0; j<Y; j++)
            {
                data[i][j] = 100*PetscTools::GetMyRank() + 10*i + j;
            }
        }
        
        // Create the dataspace for the dataset.
        hsize_t dimsf[DIMS]; // dataset dimensions
        int num_procs;
        MPI_Comm_size(PETSC_COMM_WORLD, &num_procs);
        dimsf[0] = X * num_procs;
        dimsf[1] = Y;
        hid_t filespace = H5Screate_simple(DIMS, dimsf, NULL);
        
        // Create the dataset with default properties and close filespace.
        hid_t dset_id = H5Dcreate(file_id, "IntArray", H5T_NATIVE_INT, filespace, H5P_DEFAULT);
        H5Sclose(filespace);

        // Define a dataset in memory for this process
        hsize_t count[DIMS] = {X, Y};
        hid_t memspace = H5Screate_simple(DIMS, count, NULL);
        
        // Select hyperslab in the file.
        hsize_t offset[DIMS] = {PetscTools::GetMyRank()*X, 0};
        filespace = H5Dget_space(dset_id);
        H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, NULL, count, NULL);
        
        // Create property list for collective dataset write, and write!  Finally.
        plist_id = H5Pcreate(H5P_DATASET_XFER);
        H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
    
        herr_t status = H5Dwrite(dset_id, H5T_NATIVE_INT, memspace, filespace, plist_id, data);
        TS_ASSERT_EQUALS(status, 0);
        
        // Create dataspace for the name, unit attribute
        hsize_t columns[2] = {Y, 21};
        hid_t colspace = H5Screate_simple(1, columns, NULL);
        
        //Create attribute
        char col_data[5][21];
        strcpy(col_data[0], "Noughth");
        strcpy(col_data[1], "First");
        strcpy(col_data[2], "Second");
        strcpy(col_data[3], "Third");
        strcpy(col_data[4], "Fourth");
        
        // create the type 'char'
        hid_t char_type = H5Tcopy(H5T_C_S1);
        //H5Tset_strpad(char_type, H5T_STR_NULLPAD);
        H5Tset_size(char_type, 21 );
        hid_t attr = H5Acreate(dset_id, "Name", char_type, colspace, H5P_DEFAULT  );
        // Write to the attribute        
        status = H5Awrite(attr, char_type, col_data); 
               

        
        //Close dataspace & attribute
        H5Sclose(colspace);
        H5Aclose(attr);
        
        // Release resources and close the file
        H5Dclose(dset_id);
        H5Sclose(filespace);
        H5Sclose(memspace);
        H5Pclose(plist_id);
        H5Fclose(file_id);
    }
    
    static const unsigned data_size=17;
    void TestPetscWriteDirectlyWithHdf5()
    {
        
        //Initialise a PETSc vector
        Vec a_vec=PetscTools::CreateVec(data_size);
        double* p_a_vec;
        VecGetArray(a_vec, &p_a_vec);
        int lo, hi;
        VecGetOwnershipRange(a_vec, &lo, &hi);
        for (int global_index=lo; global_index<hi; global_index++)
        {
            unsigned local_index = global_index - lo;
            p_a_vec[local_index] = global_index + 100*PetscTools::GetMyRank();
        }
        VecRestoreArray(a_vec, &p_a_vec);
        VecAssemblyBegin(a_vec);
        VecAssemblyEnd(a_vec);
        
        //VecView(a_vec, PETSC_VIEWER_STDOUT_WORLD);

        // File to write
        OutputFileHandler oh("hdf5", false);
        std::string results_dir = oh.GetOutputDirectoryFullPath();
        std::string file_name = results_dir + "vec.h5";
        
        // Set up a property list saying how we'll open the file
        hid_t plist_id = H5Pcreate(H5P_FILE_ACCESS);
        H5Pset_fapl_mpio(plist_id, PETSC_COMM_WORLD, MPI_INFO_NULL);
        
        // Create a file (collectively) and free the property list
        hid_t file_id = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
        H5Pclose(plist_id);
        
        const unsigned DIMS = 1;
        
        // Create the dataspace for the dataset.
        //TS_ASSERT_EQUALS(data_size, hi-lo);
        hsize_t dimsf[DIMS]={data_size}; // dataset dimensions
        
        hid_t filespace = H5Screate_simple(DIMS, dimsf, NULL);
        
        // Create the dataset with default properties and close filespace.
        hid_t dset_id = H5Dcreate(file_id, "TheVector", H5T_NATIVE_DOUBLE, filespace, H5P_DEFAULT);
        H5Sclose(filespace);

        // Define a dataset in memory for this process
        hsize_t count[DIMS] = {hi-lo};
        hid_t memspace = H5Screate_simple(DIMS, count, NULL);
        
        // Select hyperslab in the file.
        hsize_t offset[DIMS] = {lo};
        filespace = H5Dget_space(dset_id);
        H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, NULL, count, NULL);
        
        // Create property list for collective dataset write, and write!  Finally.
        plist_id = H5Pcreate(H5P_DATASET_XFER);
        H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
    
        VecGetArray(a_vec, &p_a_vec);
        herr_t status = H5Dwrite(dset_id, H5T_NATIVE_DOUBLE, memspace, filespace, plist_id, p_a_vec);
        VecRestoreArray(a_vec, &p_a_vec);
        
        TS_ASSERT_EQUALS(status, 0);
        
        // Release resources and close the file
        H5Dclose(dset_id);
        H5Sclose(filespace);
        H5Sclose(memspace);
        H5Pclose(plist_id);
        H5Fclose(file_id);
        
        VecDestroy(a_vec);
    }
    
    void TestReadAndChecksumHdf5()
    { 
        // File to read
        OutputFileHandler oh("hdf5", false);
        double data[data_size];
        std::string results_dir = oh.GetOutputDirectoryFullPath();
        std::string file_name = results_dir + "vec.h5";
        
        hsize_t file_id = H5Fopen(file_name.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
        //H5Tget_nmembers(file_id);
        hsize_t dataset_id = H5Dopen(file_id, "TheVector");
        hsize_t dxpl = H5Pcreate(H5P_DATASET_XFER);
        hsize_t edc = H5Pget_edc_check(dxpl);
        TS_ASSERT_EQUALS(edc, (hsize_t) 1) //Checksum is enabled
        
        herr_t status = H5Dread(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, dxpl, 
            data);
       
        TS_ASSERT_EQUALS(status, 0); 

        //Check the index
        for (unsigned i=0;i<data_size;i++)
        {
            TS_ASSERT_EQUALS(((unsigned)data[i]%100), i);
        }
        //Check the final component
        int num_procs;
        MPI_Comm_size(PETSC_COMM_WORLD, &num_procs);
        //The last component was owned by processor "num_procs-1"
        TS_ASSERT_EQUALS(((int)data[data_size-1]/100), num_procs-1);
        
        H5Pclose (dxpl);
        H5Dclose(dataset_id);
        H5Fclose(file_id);
    } 

    void TestHdf5DataWriterMultipleColumns() throw(Exception)
    {
        int number_nodes=100;
        DistributedVector::SetProblemSize(number_nodes);
               
        Hdf5DataWriter writer("hdf5", "hdf5_test_multi_column", false);
        writer.DefineFixedDimension(number_nodes);
        
        int node_id = writer.DefineVariable("Node","dimensionless");
        int ik_id = writer.DefineVariable("I_K","milliamperes");
        int ina_id = writer.DefineVariable("I_Na","milliamperes");

        writer.EndDefineMode();
      
        Vec petsc_data_1=DistributedVector::CreateVec();
        DistributedVector distributed_vector_1(petsc_data_1);
        
        Vec petsc_data_2=DistributedVector::CreateVec();
        DistributedVector distributed_vector_2(petsc_data_2);
        
        // write some values
        for (DistributedVector::Iterator index = DistributedVector::Begin();
             index!= DistributedVector::End();
             ++index)
        {
            distributed_vector_1[index] =  index.Global;
            distributed_vector_2[index] =  1000 + index.Global;
        }
        distributed_vector_1.Restore();
        distributed_vector_2.Restore();

        // write the vector
        writer.PutVector(node_id, petsc_data_1);
        writer.PutVector(ik_id, petsc_data_1);
        writer.PutVector(ina_id, petsc_data_2);
        
        writer.Close();
        
        if(PetscTools::AmMaster())
        {
            // call h5dump to take the binary hdf5 output file and print it
            // to a text file. Note that the first line of the txt file would
            // be the directory it has been printed to, but is this line is
            // removed by piping the output through sed to delete the first line  
            OutputFileHandler handler("hdf5",false);
            std::string file = handler.GetOutputDirectoryFullPath() + "/hdf5_test_multi_column.h5";
            std::string new_file = handler.GetOutputDirectoryFullPath() + "/hdf5_test_multi_column_dumped.txt";
            system( ("h5dump "+file+" | sed 1d > "+new_file).c_str() );
            
            TS_ASSERT_EQUALS(system(("diff " + new_file + " io/test/data/hdf5_test_multi_column_dumped.txt").c_str()), 0);
        }

        VecDestroy(petsc_data_1);
        VecDestroy(petsc_data_2);

    }    


    void TestHdf5DataWriterFullFormat() throw(Exception)
    {
        int number_nodes=100;
        DistributedVector::SetProblemSize(number_nodes);
               
        Hdf5DataWriter writer("hdf5", "hdf5_test_full_format", false);
        writer.DefineFixedDimension(number_nodes);
        
        int node_id = writer.DefineVariable("Node","dimensionless");
        int ik_id = writer.DefineVariable("I_K","milliamperes");
        int ina_id = writer.DefineVariable("I_Na","milliamperes");
        writer.DefineUnlimitedDimension("Time", "msec");

        writer.EndDefineMode();
      
        Vec petsc_data_1=DistributedVector::CreateVec();
        DistributedVector distributed_vector_1(petsc_data_1);
        
        Vec petsc_data_2=DistributedVector::CreateVec();
        DistributedVector distributed_vector_2(petsc_data_2);

        Vec petsc_data_3=DistributedVector::CreateVec();
        DistributedVector distributed_vector_3(petsc_data_3);
        
        for (unsigned time_step=0; time_step<10; time_step++)
        {
            // write some values
            for (DistributedVector::Iterator index = DistributedVector::Begin();
                 index!= DistributedVector::End();
                 ++index)
            {
                distributed_vector_1[index] =  index.Global;
                distributed_vector_2[index] =  time_step*1000 + 100 + index.Global;
                distributed_vector_3[index] =  time_step*1000 + 200 + index.Global;
            }
            distributed_vector_1.Restore();
            distributed_vector_2.Restore();
            distributed_vector_3.Restore();
                
            // write the vector
            writer.PutVector(node_id, petsc_data_1);
            writer.PutVector(ik_id, petsc_data_2);
            writer.PutVector(ina_id, petsc_data_3);
            writer.PutUnlimitedVariable(time_step);
            writer.AdvanceAlongUnlimitedDimension();
        }
        
        writer.Close();
        
        if(PetscTools::AmMaster())
        {
            // call h5dump to take the binary hdf5 output file and print it
            // to a text file. Note that the first line of the txt file would
            // be the directory it has been printed to, but is this line is
            // removed by piping the output through sed to delete the first line  
            OutputFileHandler handler("hdf5",false);
            std::string file = handler.GetOutputDirectoryFullPath() + "/hdf5_test_full_format.h5";
            std::string new_file = handler.GetOutputDirectoryFullPath() + "/hdf5_test_full_format_dumped.txt";
            system( ("h5dump "+file+" | sed 1d > "+new_file).c_str() );
            
            TS_ASSERT_EQUALS(system(("diff " + new_file + " io/test/data/hdf5_test_full_format_dumped.txt").c_str()), 0);
        }

        VecDestroy(petsc_data_1);
        VecDestroy(petsc_data_2);
        VecDestroy(petsc_data_3);
    }    


    void TestHdf5DataWriterFullFormatStriped() throw(Exception)
    {
        int number_nodes=100;
               
        Hdf5DataWriter writer("hdf5", "hdf5_test_full_format_striped", false);
        writer.DefineFixedDimension(number_nodes);
        
        int node_id = writer.DefineVariable("Node","dimensionless");
        int vm_id = writer.DefineVariable("V_m","millivolts");  
        int phi_e_id = writer.DefineVariable("Phi_e","millivolts");
        int ina_id = writer.DefineVariable("I_Na","milliamperes");
        
        writer.DefineUnlimitedDimension("Time", "msec");

        writer.EndDefineMode();   

        DistributedVector::SetProblemSize(number_nodes);

        Vec petsc_data_short=DistributedVector::CreateVec();
        DistributedVector distributed_vector_short(petsc_data_short);      

        Vec node_number=DistributedVector::CreateVec();
        DistributedVector distributed_node_number(node_number);      


        for (DistributedVector::Iterator index = DistributedVector::Begin();
             index!= DistributedVector::End();
             ++index)
        {
            distributed_node_number[index] = index.Global;
            distributed_vector_short[index] =  -0.5;
        }
        distributed_node_number.Restore();
        distributed_vector_short.Restore();
        
        DistributedVector::SetProblemSize(2*number_nodes);
        Vec petsc_data_long=DistributedVector::CreateVec();
        DistributedVector distributed_vector_long(petsc_data_long);      
        
        for (unsigned time_step=0; time_step<10; time_step++)
        {
            for (DistributedVector::Iterator index = DistributedVector::Begin();
                 index!= DistributedVector::End();
                 ++index)
            {
                distributed_vector_long[index] =  time_step*1000 + index.Global;
            }
            distributed_vector_long.Restore();
            
            writer.PutVector(node_id, node_number);                   
            writer.PutVector(ina_id, petsc_data_short);
            writer.PutStripedVector(vm_id, phi_e_id, petsc_data_long);
            writer.PutUnlimitedVariable(time_step);
            writer.AdvanceAlongUnlimitedDimension();
        }
        
        writer.Close();
        
        if(PetscTools::AmMaster())
        {
            // call h5dump to take the binary hdf5 output file and print it
            // to a text file. Note that the first line of the txt file would
            // be the directory it has been printed to, but is this line is
            // removed by piping the output through sed to delete the first line  
            OutputFileHandler handler("hdf5",false);
            std::string file = handler.GetOutputDirectoryFullPath() + "/hdf5_test_full_format_striped.h5";
            std::string new_file = handler.GetOutputDirectoryFullPath() + "/hdf5_test_full_format_striped_dumped.txt";
            system( ("h5dump "+file+" | sed 1d > "+new_file).c_str() );
            
            TS_ASSERT_EQUALS(system(("diff " + new_file + " io/test/data/hdf5_test_full_format_striped_dumped.txt").c_str()), 0);
        }

        VecDestroy(node_number);
        VecDestroy(petsc_data_long);
        VecDestroy(petsc_data_short);
    }
    
    void TestNonImplementedFeatures( void )
    {
        int number_nodes=100;
               
        Hdf5DataWriter writer("hdf5", "hdf5_test_non_implemented", false);
        writer.DefineFixedDimension(number_nodes);
        
        int vm_id = writer.DefineVariable("V_m","millivolts");  
        int ina_id = writer.DefineVariable("I_Na","milliamperes");
        int phi_e_id = writer.DefineVariable("Phi_e","millivolts");
        
        writer.EndDefineMode();   

        DistributedVector::SetProblemSize(number_nodes);
        Vec petsc_data_short=DistributedVector::CreateVec();
        DistributedVector distributed_vector_short(petsc_data_short);      

        for (DistributedVector::Iterator index = DistributedVector::Begin();
             index!= DistributedVector::End();
             ++index)
        {
            distributed_vector_short[index] =  -0.5;
        }
        distributed_vector_short.Restore();

        DistributedVector::SetProblemSize(2*number_nodes);
        Vec petsc_data_long=DistributedVector::CreateVec();
        DistributedVector distributed_vector_long(petsc_data_long);      
        
        for (DistributedVector::Iterator index = DistributedVector::Begin();
             index!= DistributedVector::End();
             ++index)
        {
            distributed_vector_long[index] =  index.Global;
        }
        distributed_vector_long.Restore();
               
        writer.PutVector(ina_id, petsc_data_short);
        TS_ASSERT_THROWS_ANYTHING(writer.PutStripedVector(vm_id, phi_e_id, petsc_data_long));
                
        writer.Close();
        
        VecDestroy(petsc_data_long);
        VecDestroy(petsc_data_short);        
    }    

/**
 *   Tests copied (with some minor modifications) from TestColumnDataReaderWriter: to be refactored at some point
 */ 
    void TestDefineUnlimitedDimension( void )
    {
        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new Hdf5DataWriter("", "test"));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->DefineUnlimitedDimension("Time","msecs"));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineUnlimitedDimension("Time","msecs"));
        
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineUnlimitedDimension("Time","m secs"));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineUnlimitedDimension("T,i,m,e","msecs"));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineUnlimitedDimension("","msecs"));
        
        delete mpTestWriter;
    }
    
    void TestDefineFixedDimension( void )
    {
        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new Hdf5DataWriter("", "test"));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->DefineFixedDimension(5000));
        
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineFixedDimension(5000));
        
        delete mpTestWriter;
    }
    
    void TestDefineVariable( void )
    {
        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new Hdf5DataWriter("", "test"));
        int ina_var_id = 0;
        int ik_var_id = 0;
        int ik2_var_id = 0;
        
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->DefineUnlimitedDimension("Time","msecs"));
        
        mpTestWriter->DefineFixedDimension(5000);       
        TS_ASSERT_THROWS_NOTHING(ina_var_id = mpTestWriter->DefineVariable("I_Na","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(ik_var_id = mpTestWriter->DefineVariable("I_K","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->DefineVariable("Dummy",""));
        
        // defined twice
        TS_ASSERT_THROWS_ANYTHING(ik2_var_id = mpTestWriter->DefineVariable("I_K","milliamperes"));
        
        // Bad variable names/units
        TS_ASSERT_THROWS_ANYTHING(ik_var_id = mpTestWriter->DefineVariable("I_K","milli amperes"));
        TS_ASSERT_THROWS_ANYTHING(ik_var_id = mpTestWriter->DefineVariable("I   K","milliamperes"));
        TS_ASSERT_THROWS_ANYTHING(ik_var_id = mpTestWriter->DefineVariable("I.K","milliamperes"));
        TS_ASSERT_THROWS_ANYTHING(ik_var_id = mpTestWriter->DefineVariable("","milliamperes"));
        
        TS_ASSERT_EQUALS(ina_var_id, 0);
        TS_ASSERT_EQUALS(ik_var_id, 1);
        
        delete mpTestWriter;
    }
    
    void TestEndDefineMode( void )
    {
        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new Hdf5DataWriter("", "testdefine"));
      
        //ending define mode without having defined at least a variable and a fixed dimension should raise an exception
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->EndDefineMode());
        
        int ina_var_id = 0;
        int ik_var_id = 0;
        
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->DefineUnlimitedDimension("Time","msecs"));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->EndDefineMode());
        
        TS_ASSERT_THROWS_NOTHING(ina_var_id = mpTestWriter->DefineVariable("I_Na","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(ik_var_id = mpTestWriter->DefineVariable("I_K","milliamperes"));
        
        //In Hdf5 a fixed dimension should be defined always
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->EndDefineMode());
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->DefineFixedDimension(5000));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->EndDefineMode());
        
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineVariable("I_Ca","milli amperes"));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineUnlimitedDimension("Time","msecs"));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineFixedDimension(5000));
        
        mpTestWriter->Close();
        delete mpTestWriter;                             
    }
    
    void TestCantAddUnlimitedAfterEndDefine ( void )
    {
        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new Hdf5DataWriter("", "testdefine"));
        int ina_var_id = 0;
        int ik_var_id = 0;
        
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineFixedDimension(0));
        mpTestWriter->DefineFixedDimension(5000);
        
        TS_ASSERT_THROWS_NOTHING(ina_var_id = mpTestWriter->DefineVariable("I_Na","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(ik_var_id = mpTestWriter->DefineVariable("I_K","milliamperes"));
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->EndDefineMode());
        
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->DefineUnlimitedDimension("Time","msecs"));
        mpTestWriter->Close();
        delete mpTestWriter;
    }
 
    void TestAdvanceAlongUnlimitedDimension ( void )
    {   
        TS_ASSERT_THROWS_NOTHING(mpTestWriter = new Hdf5DataWriter("", "testdefine"));
        
        int ina_var_id;
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->DefineFixedDimension(5000));        
        TS_ASSERT_THROWS_NOTHING(ina_var_id = mpTestWriter->DefineVariable("I_Na","milliamperes"));
        
        TS_ASSERT_THROWS_NOTHING(mpTestWriter->EndDefineMode());
        
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->PutUnlimitedVariable(0.0));
        TS_ASSERT_THROWS_ANYTHING(mpTestWriter->AdvanceAlongUnlimitedDimension());
        
        mpTestWriter->Close();
        delete mpTestWriter;
    }
    
    void TestCantWriteDataWhileInDefineMode ( void )
    {       
        int number_nodes=100;
               
        Hdf5DataWriter writer("", "testdefine", false);
//        writer.DefineFixedDimension(number_nodes);
//        
//        int node_id = writer.DefineVariable("Node","dimensionless");
//        int vm_id = writer.DefineVariable("V_m","millivolts");  
//        int phi_e_id = writer.DefineVariable("Phi_e","millivolts");
//        int ina_id = writer.DefineVariable("I_Na","milliamperes");
//        
//        writer.DefineUnlimitedDimension("Time", "msec");

//        writer.EndDefineMode();   

        int node_id = 1;
        int vm_id = 2;
        int phi_e_id = 3;
        int ina_id = 4;

        DistributedVector::SetProblemSize(number_nodes);

        Vec petsc_data_short=DistributedVector::CreateVec();
        DistributedVector distributed_vector_short(petsc_data_short);      

        Vec node_number=DistributedVector::CreateVec();
        DistributedVector distributed_node_number(node_number);      

        for (DistributedVector::Iterator index = DistributedVector::Begin();
             index!= DistributedVector::End();
             ++index)
        {
            distributed_node_number[index] = index.Global;
            distributed_vector_short[index] =  -0.5;
        }
        distributed_node_number.Restore();
        distributed_vector_short.Restore();
        
        DistributedVector::SetProblemSize(2*number_nodes);
        Vec petsc_data_long=DistributedVector::CreateVec();
        DistributedVector distributed_vector_long(petsc_data_long);      
        
        for (DistributedVector::Iterator index = DistributedVector::Begin();
             index!= DistributedVector::End();
             ++index)
        {
            distributed_vector_long[index] =  1000 + index.Global;
        }
        distributed_vector_long.Restore();
        
        TS_ASSERT_THROWS_ANYTHING(writer.PutVector(node_id, node_number));                   
        TS_ASSERT_THROWS_ANYTHING(writer.PutVector(ina_id, petsc_data_short));
        TS_ASSERT_THROWS_ANYTHING(writer.PutStripedVector(vm_id, phi_e_id, petsc_data_long));
        TS_ASSERT_THROWS_ANYTHING(writer.PutUnlimitedVariable(0.0));
        TS_ASSERT_THROWS_ANYTHING(writer.AdvanceAlongUnlimitedDimension());
        
        writer.Close();
        VecDestroy(petsc_data_short);
        VecDestroy(node_number);
        VecDestroy(petsc_data_long);
        
    }
    
};
#endif /*TESTHDF5DATAWRITER_HPP_*/
