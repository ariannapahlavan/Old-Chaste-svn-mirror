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
#ifndef TESTVERTEXMESHWRITER_HPP_
#define TESTVERTEXMESHWRITER_HPP_

#include <cxxtest/TestSuite.h>

#include <string>
#include <fstream>

#include "MutableMesh.hpp"
#include "MutableVertexMesh.hpp"
#include "VertexMeshWriter.hpp"
#include "VtkMeshWriter.hpp"
#include "VertexMeshReader.hpp"

#ifdef CHASTE_VTK
#define _BACKWARD_BACKWARD_WARNING_H 1 // Cut out the strstream deprecated warning for now (gcc4.3)
#include <vtkVersion.h>
#endif

class TestVertexMeshWriter : public CxxTest::TestSuite
{
public:

    void TestVertexMeshWriterIn2d() throw(Exception)
    {
        std::vector<Node<2>*> basic_nodes;
        basic_nodes.push_back(new Node<2>(0, true, 0.0, 0.0));
        basic_nodes.push_back(new Node<2>(1, true, 1.0, 0.0));
        basic_nodes.push_back(new Node<2>(2, true, 1.5, 1.0));
        basic_nodes.push_back(new Node<2>(3, true, 1.0, 2.0));
        basic_nodes.push_back(new Node<2>(4, true, 0.0, 1.0));
        basic_nodes.push_back(new Node<2>(5, true, 2.0, 0.0));
        basic_nodes.push_back(new Node<2>(6, true, 2.0, 3.0));

        std::vector<Node<2>*> nodes_elem_0, nodes_elem_1;

        // Make two elements out of these nodes
        nodes_elem_0.push_back(basic_nodes[0]);
        nodes_elem_0.push_back(basic_nodes[1]);
        nodes_elem_0.push_back(basic_nodes[2]);
        nodes_elem_0.push_back(basic_nodes[3]);
        nodes_elem_0.push_back(basic_nodes[4]);

        nodes_elem_1.push_back(basic_nodes[2]);
        nodes_elem_1.push_back(basic_nodes[5]);
        nodes_elem_1.push_back(basic_nodes[6]);

        std::vector<VertexElement<2,2>*> basic_vertex_elements;
        basic_vertex_elements.push_back(new VertexElement<2,2>(0, nodes_elem_0));
        basic_vertex_elements.push_back(new VertexElement<2,2>(1, nodes_elem_1));

        // Make a vertex mesh
        VertexMesh<2,2> basic_vertex_mesh(basic_nodes, basic_vertex_elements);

        // Create a vertex mesh writer
        VertexMeshWriter<2,2> vertex_mesh_writer("TestVertexMeshWriterIn2d", "vertex_mesh_2d");

        // Test files are written correctly
        vertex_mesh_writer.WriteFilesUsingMesh(basic_vertex_mesh);

        OutputFileHandler handler("TestVertexMeshWriterIn2d", false);
        std::string results_file1 = handler.GetOutputDirectoryFullPath() + "vertex_mesh_2d.node";
        std::string results_file2 = handler.GetOutputDirectoryFullPath() + "vertex_mesh_2d.cell";

        // To ignore the provenance data we only go as far as
        TS_ASSERT_EQUALS(system(("diff -I \"Created by Chaste\" " + results_file1 + " mesh/test/data/TestVertexMeshWriter/vertex_mesh_2d.node").c_str()), 0);
        TS_ASSERT_EQUALS(system(("diff -I \"Created by Chaste\" " + results_file2 + " mesh/test/data/TestVertexMeshWriter/vertex_mesh_2d.cell").c_str()), 0);

#ifdef CHASTE_VTK
        std::vector<double> cell_ids;
        cell_ids.push_back(0.0);
        cell_ids.push_back(1.0);

        vertex_mesh_writer.AddCellData("Cell IDs", cell_ids);
        // Add distance from origin into the node "point" data
        std::vector<double> distance;
        for (unsigned i=0; i<basic_vertex_mesh.GetNumNodes(); i++)
        {
            distance.push_back(norm_2(basic_vertex_mesh.GetNode(i)->rGetLocation()));
        }
        vertex_mesh_writer.AddPointData("Distance from origin", distance);

        vertex_mesh_writer.WriteVtkUsingMesh(basic_vertex_mesh);

        {
            ///\todo #1076.  We need a way to test the contents of the VTK file
            std::string results_file3 = handler.GetOutputDirectoryFullPath() + "vertex_mesh_2d.vtu";
            FileFinder vtk_file(results_file3, RelativeTo::Absolute);
            TS_ASSERT(vtk_file.Exists());
        }

#else
        std::cout << "This test ran, but did not test VTK-dependent functions as VTK visualization is not enabled." << std::endl;
        std::cout << "If required please install and alter your hostconfig settings to switch on chaste support." << std::endl;
#endif //CHASTE_VTK
    }

    void TestVertexMeshWriterIn3dWithoutFaces() throw(Exception)
    {
        // Create 3D mesh
        std::vector<Node<3>*> nodes;
        nodes.push_back(new Node<3>(0, true, 0.0, 0.0, 0.0));
        nodes.push_back(new Node<3>(1, true, 1.0, 0.0, 0.0));
        nodes.push_back(new Node<3>(2, true, 1.0, 2.0, 0.0));
        nodes.push_back(new Node<3>(3, true, 0.0, 2.0, 0.0));
        nodes.push_back(new Node<3>(4, true, 0.0, 2.0, 3.0));
        nodes.push_back(new Node<3>(5, true, 1.0, 0.0, 3.0));
        nodes.push_back(new Node<3>(6, true, 1.0, 2.0, 3.0));
        nodes.push_back(new Node<3>(7, true, 0.0, 2.0, 3.0));

        std::vector<VertexElement<3,3>*> elements;
        elements.push_back(new VertexElement<3,3>(0, nodes));

        // Make a vertex mesh
        VertexMesh<3,3> mesh3d(nodes, elements);
        TS_ASSERT_DELTA(mesh3d.GetWidth(0), 1.0, 1e-4);
        TS_ASSERT_DELTA(mesh3d.GetWidth(1), 2.0, 1e-4);
        TS_ASSERT_DELTA(mesh3d.GetWidth(2), 3.0, 1e-4);

        // Create a vertex mesh writer
        VertexMeshWriter<3,3> vertex_mesh_writer("TestVertexMeshWriterIn3dWithoutFaces", "vertex_mesh_3d", false);

        // Test files are written correctly
        vertex_mesh_writer.WriteFilesUsingMesh(mesh3d);

        OutputFileHandler handler("TestVertexMeshWriterIn3dWithoutFaces", false);
        std::string results_file1 = handler.GetOutputDirectoryFullPath() + "vertex_mesh_3d.node";
        std::string results_file2 = handler.GetOutputDirectoryFullPath() + "vertex_mesh_3d.cell";

        // To ignore the provenance data we only go as far as
        TS_ASSERT_EQUALS(system(("diff -I \"Created by Chaste\" " + results_file1 + " mesh/test/data/TestVertexMeshWriter/vertex_mesh_3d.node").c_str()), 0);
        TS_ASSERT_EQUALS(system(("diff -I \"Created by Chaste\" " + results_file2 + " mesh/test/data/TestVertexMeshWriter/vertex_mesh_3d.cell").c_str()), 0);

#ifdef CHASTE_VTK
        std::vector<double> cell_ids;
        cell_ids.push_back(0.0);
        vertex_mesh_writer.AddCellData("Cell IDs", cell_ids);

         // Add distance from origin into the node "point" data
        std::vector<double> distance;
        for (unsigned i=0; i<mesh3d.GetNumNodes(); i++)
        {
            distance.push_back(norm_2(mesh3d.GetNode(i)->rGetLocation()));
        }
        vertex_mesh_writer.AddPointData("Distance from origin", distance);

        vertex_mesh_writer.WriteVtkUsingMesh(mesh3d, "42");

        {
            ///\todo #1076.  We need a way to test the contents of the VTK file
            std::string results_file3 = handler.GetOutputDirectoryFullPath() + "vertex_mesh_3d_42.vtu";
            FileFinder vtk_file(results_file3, RelativeTo::Absolute);
            TS_ASSERT(vtk_file.Exists());
        }

#else
        std::cout << "This test ran, but did not test VTK-dependent functions as VTK visualization is not enabled." << std::endl;
        std::cout << "If required please install and alter your hostconfig settings to switch on chaste support." << std::endl;
#endif //CHASTE_VTK
    }

    void TestVertexMeshWriterIn3dWithFaces() throw(Exception)
    {
        // Create a simple 3D mesh using the Voronoi constructor
        std::vector<Node<3>*> nodes;
        nodes.push_back(new Node<3>(0, true,  0.0, 0.0, 0.0));
        nodes.push_back(new Node<3>(1, true,  1.0, 1.0, 0.0));
        nodes.push_back(new Node<3>(2, true,  1.0, 0.0, 1.0));
        nodes.push_back(new Node<3>(3, true,  0.0, 1.0, 1.0));
        nodes.push_back(new Node<3>(4, false, 0.5, 0.5, 0.5));

        MutableMesh<3,3> delaunay_mesh(nodes);
        VertexMesh<3,3> mesh3d(delaunay_mesh);

        // Create a vertex mesh writer
        VertexMeshWriter<3,3> vertex_mesh_writer("TestVertexMeshWriterIn3dWithFaces", "vertex_mesh_3d_with_faces", false);

        // Test files are written correctly
        vertex_mesh_writer.WriteFilesUsingMesh(mesh3d);

        OutputFileHandler handler("TestVertexMeshWriterIn3dWithFaces", false);
        std::string results_file1 = handler.GetOutputDirectoryFullPath() + "vertex_mesh_3d_with_faces.node";
        std::string results_file2 = handler.GetOutputDirectoryFullPath() + "vertex_mesh_3d_with_faces.cell";

        //\todo #1468 the current saved results have no boundary nodes this ticket should fix that and you will need to change the saved results

        // To ignore the provenance data we only go as far as
        TS_ASSERT_EQUALS(system(("diff -I \"Created by Chaste\" " + results_file1 + " mesh/test/data/TestVertexMeshWriter/vertex_mesh_3d_with_faces.node").c_str()), 0);
        TS_ASSERT_EQUALS(system(("diff -I \"Created by Chaste\" " + results_file2 + " mesh/test/data/TestVertexMeshWriter/vertex_mesh_3d_with_faces.cell").c_str()), 0);

#ifdef CHASTE_VTK
        std::vector<double> cell_ids;
        cell_ids.push_back(0.0);
        vertex_mesh_writer.AddCellData("Cell IDs", cell_ids);

         // Add distance from origin into the node "point" data
        std::vector<double> distance;
        for (unsigned i=0; i<mesh3d.GetNumNodes(); i++)
        {
            distance.push_back(norm_2(mesh3d.GetNode(i)->rGetLocation()));
        }
        vertex_mesh_writer.AddPointData("Distance from origin", distance);

        vertex_mesh_writer.WriteVtkUsingMesh(mesh3d, "42");

        {
            ///\todo #1076.  We need a way to test the contents of the VTK file
            std::string results_file3 = handler.GetOutputDirectoryFullPath() + "vertex_mesh_3d_with_faces_42.vtu";
            FileFinder vtk_file(results_file3, RelativeTo::Absolute);
            TS_ASSERT(vtk_file.Exists());
        }

#else
        std::cout << "This test ran, but did not test VTK-dependent functions as VTK visualization is not enabled." << std::endl;
        std::cout << "If required please install and alter your hostconfig settings to switch on chaste support." << std::endl;
#endif //CHASTE_VTK
    }

    void TestMeshWriterWithDeletedNode() throw (Exception)
    {
        // Create mesh
        VertexMeshReader<2,2> mesh_reader("mesh/test/data/TestVertexMesh/honeycomb_vertex_mesh_3_by_3");
        MutableVertexMesh<2,2> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);

        TS_ASSERT_EQUALS(mesh.GetNumNodes(), 30u);
        TS_ASSERT_EQUALS(mesh.GetNumElements(), 9u);

        /*
         * Delete element 0. This element contains 3 nodes that are
         * not contained in any other element and so will be marked
         * as deleted.
         */
        mesh.DeleteElementPriorToReMesh(0);

        // Write mesh to file
        VertexMeshWriter<2,2> mesh_writer("TestMeshWriterWithDeletedNode", "vertex_mesh");
        TS_ASSERT_THROWS_NOTHING(mesh_writer.WriteFilesUsingMesh(mesh));

        // Read mesh back in from file
        std::string output_dir = mesh_writer.GetOutputDirectory();
        VertexMeshReader<2,2> mesh_reader2(output_dir + "vertex_mesh");

        // We should have one less element and three less nodes
        TS_ASSERT_EQUALS(mesh_reader2.GetNumNodes(), 27u);
        TS_ASSERT_EQUALS(mesh_reader2.GetNumElements(), 8u);
    }

    void TestReadingAndWritingElementAttributes() throw(Exception)
    {
        // Read in a mesh with element attributes
        VertexMeshReader<2,2> mesh_reader("mesh/test/data/TestVertexMeshReader2d/vertex_mesh_with_element_attributes");
        TS_ASSERT_EQUALS(mesh_reader.GetNumElements(), 2u);
        TS_ASSERT_EQUALS(mesh_reader.GetNumElementAttributes(), 1u);

        // Construct the mesh
        VertexMesh<2,2> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);
        TS_ASSERT_EQUALS(mesh.GetElement(0)->GetAttribute(), 97u);
        TS_ASSERT_EQUALS(mesh.GetElement(1)->GetAttribute(), 152u);

        // Write the mesh to file
        VertexMeshWriter<2,2> mesh_writer("TestReadingAndWritingElementAttributes", "vertex_mesh_with_element_attributes");
        mesh_writer.WriteFilesUsingMesh(mesh);

        // Now read in the mesh that was written
        OutputFileHandler handler("TestReadingAndWritingElementAttributes", false);
        VertexMeshReader<2,2> mesh_reader2(handler.GetOutputDirectoryFullPath() + "vertex_mesh_with_element_attributes");
        TS_ASSERT_EQUALS(mesh_reader2.GetNumElements(), 2u);
        TS_ASSERT_EQUALS(mesh_reader2.GetNumElementAttributes(), 1u);

        // Construct the mesh again
        VertexMesh<2,2> mesh2;
        mesh2.ConstructFromMeshReader(mesh_reader);
        TS_ASSERT_EQUALS(mesh2.GetElement(0)->GetAttribute(), 97u);
        TS_ASSERT_EQUALS(mesh2.GetElement(1)->GetAttribute(), 152u);

        // For coverage, repeat this test for a vertex mesh whose elements have faces
        VertexMeshReader<3,3> mesh_reader3d("mesh/test/data/TestVertexMeshWriter/vertex_mesh_3d_with_faces_and_attributes");
        TS_ASSERT_EQUALS(mesh_reader3d.GetNumElements(), 1u);
        TS_ASSERT_EQUALS(mesh_reader3d.GetNumElementAttributes(), 1u);

        // Construct the mesh
        VertexMesh<3,3> mesh3d;
        mesh3d.ConstructFromMeshReader(mesh_reader3d);
        TS_ASSERT_EQUALS(mesh3d.GetElement(0)->GetAttribute(), 49u);

        // Write the mesh to file
        VertexMeshWriter<3,3> mesh_writer3d("TestReadingAndWritingElementAttributes", "vertex_mesh_3d_with_faces_and_attributes");
        mesh_writer3d.WriteFilesUsingMesh(mesh3d);

        // Now read in the mesh that was written
        OutputFileHandler handler3d("TestReadingAndWritingElementAttributes", false);
        VertexMeshReader<3,3> mesh_reader3d2(handler3d.GetOutputDirectoryFullPath() + "vertex_mesh_3d_with_faces_and_attributes");
        TS_ASSERT_EQUALS(mesh_reader3d2.GetNumElements(), 1u);
        TS_ASSERT_EQUALS(mesh_reader3d2.GetNumElementAttributes(), 1u);

        // Construct the mesh again
        VertexMesh<3,3> mesh3d2;
        mesh3d2.ConstructFromMeshReader(mesh_reader3d);
        TS_ASSERT_EQUALS(mesh3d2.GetElement(0)->GetAttribute(), 49u);
    }
};

#endif /*TESTVERTEXMESHWRITER_HPP_*/
