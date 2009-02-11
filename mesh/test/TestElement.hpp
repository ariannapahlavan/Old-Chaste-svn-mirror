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


#ifndef _TESTELEMENT_HPP_
#define _TESTELEMENT_HPP_

#include "MutableMesh.hpp"

#include "Exception.hpp"
#include "TrianglesMeshReader.hpp"
#include <cxxtest/TestSuite.h>
//#include <iostream>

#include <vector>

class TestElement : public CxxTest::TestSuite
{
    /**
     * Create a node with given index that has a point at the origin and is
     * not on the boundary.
     *
     * @param index The global index of the created node.
     * @returns A pointer to a new 3d node.
     */
    Node<3> *CreateZeroPointNode(int index)
    {
        c_vector<double,3> zero_point;
        zero_point.clear();

        Node<3> *p_node = new Node<3>(index, zero_point, false);
        return p_node;
    }


public:
    void TestConstructionForLinearBasisFunctions()
    {
        std::vector<Node<3>*> corner_nodes;
        corner_nodes.push_back(new Node<3>(0, false, 0.0, 0.0, 0.0));
        corner_nodes.push_back(new Node<3>(1, false, 1.0, 0.0, 0.0));
        corner_nodes.push_back(new Node<3>(2, false, 0.0, 1.0, 0.0));
        corner_nodes.push_back(new Node<3>(3, false, 0.0, 0.0, 1.0));
        Element<3,3> element(INDEX_IS_NOT_USED, corner_nodes);

        // Check nodes on the new element have the right indices
        for (unsigned i=0; i<4; i++)
        {
            TS_ASSERT_EQUALS(element.GetNodeGlobalIndex(i), i);
        }


        TS_ASSERT_DELTA(element.GetVolume(), 1.0/6.0,1e-5);

        for (unsigned i=0; i<corner_nodes.size(); i++)
        {
            delete corner_nodes[i];
        }
    }


    void TestEquals()
    {
        std::vector<Node<3>*> corner_nodes;
        corner_nodes.push_back(new Node<3>(0, false, 0.0, 0.0, 0.0));
        corner_nodes.push_back(new Node<3>(1, false, 1.0, 0.0, 0.0));
        corner_nodes.push_back(new Node<3>(2, false, 0.0, 1.0, 0.0));
        corner_nodes.push_back(new Node<3>(3, false, 0.0, 0.0, 1.0));
        Element<3,3> element(INDEX_IS_NOT_USED, corner_nodes);

        std::vector<Node<3>*> more_nodes;
        more_nodes.push_back(new Node<3>(0, false, 10.0, 10.0, 10.0));
        more_nodes.push_back(new Node<3>(1, false, 11.0, 10.0, 10.0));
        more_nodes.push_back(new Node<3>(2, false, 10.0, 11.0, 10.0));
        more_nodes.push_back(new Node<3>(3, false, 10.0, 10.0, 11.0));
        Element<3,3> another_element(INDEX_IS_NOT_USED, more_nodes);

        // test (and cover) equals operator
        another_element = element;

        for (int i=0; i<4; i++)
        {
            for (int j=0; j<3; j++)
            {
                TS_ASSERT_DELTA(another_element.GetNode(i)->GetPoint()[j], element.GetNode(i)->GetPoint()[j], 1e-10);
            }
        }

        for (unsigned i=0; i<corner_nodes.size(); i++)
        {
            delete corner_nodes[i];
            delete more_nodes[i];
        }
    }


    void TestGetSetAbstractTetrahedralElementMethods()
    {
        std::vector<Node<3>*> corner_nodes;
        corner_nodes.push_back(new Node<3>(0, false, 0.0, 0.0, 0.0));
        corner_nodes.push_back(new Node<3>(1, false, 1.0, 0.0, 0.0));
        corner_nodes.push_back(new Node<3>(2, false, 0.0, 1.0, 0.0));
        corner_nodes.push_back(new Node<3>(3, false, 0.0, 0.0, 1.0));
        Element<3,3> element(INDEX_IS_NOT_USED, corner_nodes);

        element.SetOwnership(true);

        TS_ASSERT_EQUALS(element.GetOwnership(),true);

        for (unsigned i=0; i<corner_nodes.size(); i++)
        {
            delete corner_nodes[i];
        }

    }

    void TestJacobian()
    {
        // 1d
        std::vector<Node<1>*> nodes1d;
        nodes1d.push_back(new Node<1>(0, false, 2.0));
        nodes1d.push_back(new Node<1>(1, false, 2.5));
        Element<1,1> element1d(INDEX_IS_NOT_USED, nodes1d);
        c_matrix<double, 1, 1> J1d;
        double det_1d;
        c_matrix<double, 1, 1> J1dinv;
        element1d.CalculateInverseJacobian(J1d, det_1d, J1dinv);
        TS_ASSERT_DELTA(J1d(0,0), 0.5, 1e-12);
        TS_ASSERT_DELTA(element1d.GetVolume(), 0.5,1e-5);
        TS_ASSERT_DELTA(det_1d, 0.5, 1e-12);
        TS_ASSERT_DELTA(J1dinv(0,0), 2.0, 1e-12);

        delete nodes1d[0];
        delete nodes1d[1];
        // 2d easy

        std::vector<Node<2>*> nodes2d;
        nodes2d.push_back(new Node<2>(0, false, 0.0, 0.0));
        nodes2d.push_back(new Node<2>(1, false, 1.0, 0.0));
        nodes2d.push_back(new Node<2>(2, false, 0.0, 1.0));
        Element<2,2> element2d(INDEX_IS_NOT_USED, nodes2d);
        c_matrix<double, 2, 2> J2d;
        double det;
        element2d.CalculateJacobian(J2d, det);
        TS_ASSERT_DELTA(J2d(0,0), 1.0, 1e-12);
        TS_ASSERT_DELTA(J2d(0,1), 0.0, 1e-12);
        TS_ASSERT_DELTA(J2d(1,0), 0.0, 1e-12);
        TS_ASSERT_DELTA(J2d(1,1), 1.0, 1e-12);
        TS_ASSERT_DELTA(element2d.GetVolume(), 0.5 ,1e-5);
        delete nodes2d[0];
        delete nodes2d[1];
        delete nodes2d[2];

        //2d general
        std::vector<Node<2>*> nodes2d2;
        nodes2d2.push_back(new Node<2>(0, false, 1.0, -2.0));
        nodes2d2.push_back(new Node<2>(1, false, 4.0, -3.0));
        nodes2d2.push_back(new Node<2>(2, false, 2.0, -1.0));
        Element<2,2> element2d2(INDEX_IS_NOT_USED, nodes2d2);
        c_matrix<double, 2, 2> J2d2;
        c_matrix<double, 2, 2> J2d2inv;
        double det_2d;
        element2d2.CalculateInverseJacobian(J2d2, det_2d,J2d2inv);
        TS_ASSERT_DELTA(J2d2(0,0), 3.0, 1e-12);
        TS_ASSERT_DELTA(J2d2(0,1), 1.0, 1e-12);
        TS_ASSERT_DELTA(J2d2(1,0), -1.0, 1e-12);
        TS_ASSERT_DELTA(J2d2(1,1), 1.0, 1e-12);

        TS_ASSERT_DELTA(det_2d, 4.0, 1e-12);
        TS_ASSERT_DELTA(J2d2inv(0,0), 0.25, 1e-12);
        TS_ASSERT_DELTA(J2d2inv(0,1), -0.25, 1e-12);
        TS_ASSERT_DELTA(J2d2inv(1,0), 0.25, 1e-12);
        TS_ASSERT_DELTA(J2d2inv(1,1), 0.75, 1e-12);

        delete nodes2d2[0];
        delete nodes2d2[1];
        delete nodes2d2[2];


        // 3d easy
        std::vector<Node<3>*> nodes3d;
        nodes3d.push_back(new Node<3>(0, false, 0.0, 0.0, 0.0));
        nodes3d.push_back(new Node<3>(1, false, 1.0, 0.0, 0.0));
        nodes3d.push_back(new Node<3>(2, false, 0.0, 1.0, 0.0));
        nodes3d.push_back(new Node<3>(3, false, 0.0, 0.0, 1.0));
        Element<3,3> element3d(INDEX_IS_NOT_USED, nodes3d);
        c_matrix<double, 3, 3> J3d;
        double det_3d;
        element3d.CalculateJacobian(J3d,det_3d);
        TS_ASSERT_DELTA(J3d(0,0), 1.0, 1e-12);
        TS_ASSERT_DELTA(J3d(0,1), 0.0, 1e-12);
        TS_ASSERT_DELTA(J3d(0,2), 0.0, 1e-12);
        TS_ASSERT_DELTA(J3d(1,0), 0.0, 1e-12);
        TS_ASSERT_DELTA(J3d(1,1), 1.0, 1e-12);
        TS_ASSERT_DELTA(J3d(1,2), 0.0, 1e-12);
        TS_ASSERT_DELTA(J3d(2,0), 0.0, 1e-12);
        TS_ASSERT_DELTA(J3d(2,1), 0.0, 1e-12);
        TS_ASSERT_DELTA(J3d(2,2), 1.0, 1e-12);

        delete nodes3d[0];
        delete nodes3d[1];
        delete nodes3d[2];
        delete nodes3d[3];


        // 3d general
        std::vector<Node<3>*> nodes3d2;
        nodes3d2.push_back(new Node<3>(0, false, 1.0, 2.0, 3.0));
        nodes3d2.push_back(new Node<3>(1, false, 2.0, 1.0, 3.0));
        nodes3d2.push_back(new Node<3>(2, false, 5.0, 5.0, 5.0));
        nodes3d2.push_back(new Node<3>(3, false, 0.0, 3.0, 4.0));
        Element<3,3> element3d2(INDEX_IS_NOT_USED, nodes3d2);
        c_matrix<double, 3, 3> J3d2;
        c_matrix<double, 3, 3> J3d2inv;
        element3d2.CalculateInverseJacobian(J3d2,det_3d,J3d2inv);
        TS_ASSERT_DELTA(J3d2(0,0), 1.0, 1e-4);
        TS_ASSERT_DELTA(J3d2(0,1), 4.0, 1e-4);
        TS_ASSERT_DELTA(J3d2(0,2), -1.0, 1e-4);
        TS_ASSERT_DELTA(J3d2(1,0), -1.0, 1e-4);
        TS_ASSERT_DELTA(J3d2(1,1), 3.0, 1e-4);
        TS_ASSERT_DELTA(J3d2(1,2), 1.0, 1e-4);
        TS_ASSERT_DELTA(J3d2(2,0), 0.0, 1e-4);
        TS_ASSERT_DELTA(J3d2(2,1), 2.0, 1e-4);
        TS_ASSERT_DELTA(J3d2(2,2), 1.0, 1e-4);
        // checks Jacobian determinant
        TS_ASSERT_DELTA(det_3d, 7.0, 1e-4);
        // checks inverse Jacobian
        TS_ASSERT_DELTA(J3d2inv(0,0), 1.0/7.0, 1e-4);
        TS_ASSERT_DELTA(J3d2inv(0,1), -6.0/7.0, 1e-4);
        TS_ASSERT_DELTA(J3d2inv(0,2), 1.0, 1e-4);
        TS_ASSERT_DELTA(J3d2inv(1,0), 1.0/7.0, 1e-4);
        TS_ASSERT_DELTA(J3d2inv(1,1), 1.0/7.0, 1e-4);
        TS_ASSERT_DELTA(J3d2inv(1,2), 0.0, 1e-4);
        TS_ASSERT_DELTA(J3d2inv(2,0), -2.0/7.0, 1e-4);
        TS_ASSERT_DELTA(J3d2inv(2,1), -2.0/7.0, 1e-4);
        TS_ASSERT_DELTA(J3d2inv(2,2), 1.0, 1e-4);

        delete nodes3d2[0];
        delete nodes3d2[1];
        delete nodes3d2[2];
        delete nodes3d2[3];
    }

    void TestNodeToElementConversion(void)
    {
        ChastePoint<1> point1(1.0);
        ChastePoint<2> point2(2.0,-1.0);

        Node<1> node1(0, point1);
        Node<2> node2(0, point2);

        BoundaryElement<0,1> element1(INDEX_IS_NOT_USED, &node1);
        BoundaryElement<0,2> element2(INDEX_IS_NOT_USED, &node2);

        TS_ASSERT_EQUALS(element1.GetNode(0)->GetPoint()[0], point1[0]);

        TS_ASSERT_EQUALS(element2.GetNode(0)->GetPoint()[0], point2[0]);
        TS_ASSERT_EQUALS(element2.GetNode(0)->GetPoint()[1], point2[1]);
    }

    void TestGetNodeLocation() throw(Exception)
    {
        ChastePoint<2> point1(0.0,1.0);
        ChastePoint<2> point2(4.0,6.0);
        ChastePoint<2> point3(2.0,3.0);

        std::vector<Node<2>*> element_nodes;
        element_nodes.push_back(new Node<2>(0, point1));
        element_nodes.push_back(new Node<2>(0, point2));
        element_nodes.push_back(new Node<2>(0, point3));

        Element<2,2> element(INDEX_IS_NOT_USED, element_nodes);

        // note that nodes 2 and 3 are swapped by the element constructor
        // to ensure that the jacobian determinant is positive
        TS_ASSERT_EQUALS(element.GetNodeLocation(0,0),0.0);
        TS_ASSERT_EQUALS(element.GetNodeLocation(0)(0),0.0);
        TS_ASSERT_EQUALS(element.GetNodeLocation(1,0),2.0);
        TS_ASSERT_EQUALS(element.GetNodeLocation(1)(0),2.0);
        TS_ASSERT_EQUALS(element.GetNodeLocation(2,0),4.0);
        TS_ASSERT_EQUALS(element.GetNodeLocation(2)(0),4.0);

        delete element_nodes[0];
        delete element_nodes[1];
        delete element_nodes[2];
    }

    void TestElementSwapsNodesIfJacobianIsNegative()
    {
        ChastePoint<1> a0(0),    a1(1);
        ChastePoint<2> b0(0,0),  b1(1,0)  ,b2(0,1);
        ChastePoint<3> c0(0,0,0),c1(1,0,0),c2(0,1,0),c3(0,0,1);

        Node<1>  na0(0,a0), na1(1,a1);
        Node<2>  nb0(0,b0), nb1(1,b1), nb2(2,b2);
        Node<3>  nc0(0,c0), nc1(1,c1), nc2(2,c2), nc3(3,c3);


        ////////////////////////////////////////////
        // 1d
        ////////////////////////////////////////////
        std::vector<Node<1>*> nodes_1d_correct;
        nodes_1d_correct.push_back(&na0);
        nodes_1d_correct.push_back(&na1);

        std::vector<Node<1>*> nodes_1d_incorrect;
        nodes_1d_incorrect.push_back(&na1);
        nodes_1d_incorrect.push_back(&na0);

        Element<1,1>   e_1d_correct_orientation(INDEX_IS_NOT_USED, nodes_1d_correct);
        Element<1,1> e_1d_incorrect_orientation(INDEX_IS_NOT_USED, nodes_1d_incorrect);

        // index of second node should be 1
        TS_ASSERT_EQUALS( e_1d_correct_orientation.GetNode(1)->GetIndex(), 1U);
        // index of second node for incorrect orientation element should also be 1
        // because the element should have swapped the nodes around
        TS_ASSERT_EQUALS( e_1d_incorrect_orientation.GetNode(1)->GetIndex(), 1U);


        ////////////////////////////////////////////
        // 2d
        ////////////////////////////////////////////
        std::vector<Node<2>*> nodes_2d_correct;
        nodes_2d_correct.push_back(&nb0);
        nodes_2d_correct.push_back(&nb1);
        nodes_2d_correct.push_back(&nb2);

        std::vector<Node<2>*> nodes_2d_incorrect;
        nodes_2d_incorrect.push_back(&nb1);
        nodes_2d_incorrect.push_back(&nb0);
        nodes_2d_incorrect.push_back(&nb2);

        Element<2,2>   e_2d_correct_orientation(INDEX_IS_NOT_USED, nodes_2d_correct);
        Element<2,2> e_2d_incorrect_orientation(INDEX_IS_NOT_USED, nodes_2d_incorrect);

        // index of last node should be 2
        TS_ASSERT_EQUALS( e_2d_correct_orientation.GetNode(2)->GetIndex(), 2U);

        // index of last node for incorrect orientation element should be 0
        // because the element should have swapped the last two nodes around
        TS_ASSERT_EQUALS( e_2d_incorrect_orientation.GetNode(2)->GetIndex(), 0U);


        ////////////////////////////////////////////
        // 3d
        ////////////////////////////////////////////
        std::vector<Node<3>*> nodes_3d_correct;
        nodes_3d_correct.push_back(&nc0);
        nodes_3d_correct.push_back(&nc1);
        nodes_3d_correct.push_back(&nc2);
        nodes_3d_correct.push_back(&nc3);

        std::vector<Node<3>*> nodes_3d_incorrect;
        nodes_3d_incorrect.push_back(&nc0);
        nodes_3d_incorrect.push_back(&nc1);
        nodes_3d_incorrect.push_back(&nc3);
        nodes_3d_incorrect.push_back(&nc2);

        Element<3,3>   e_3d_correct_orientation(INDEX_IS_NOT_USED, nodes_3d_correct);
        Element<3,3> e_3d_incorrect_orientation(INDEX_IS_NOT_USED, nodes_3d_incorrect);

        // index of last node should be 3
        TS_ASSERT_EQUALS( e_3d_correct_orientation.GetNode(3)->GetIndex(), 3U);

        // index of last node for incorrect orientation element should be 3
        // because the element should have swapped the last two nodes around
        TS_ASSERT_EQUALS( e_3d_incorrect_orientation.GetNode(3)->GetIndex(), 3U);

    }

    void TestElementCopyConstructor(void)
    {
        std::vector<Node<3>*> corner_nodes;
        corner_nodes.push_back(new Node<3>(0, false, 0.0, 0.0, 0.0));
        corner_nodes.push_back(new Node<3>(1, false, 1.0, 0.0, 3.0));
        corner_nodes.push_back(new Node<3>(2, false, 0.0, 1.0, 0.0));
        corner_nodes.push_back(new Node<3>(3, false, 0.0, 0.0, 1.0));
        Element<3,3> element(31415, corner_nodes);

        // Create a copy of the element and test that it's the same as the original one

        Element<3,3> copied_element(element);

        TS_ASSERT_EQUALS(element.GetNumNodes(), copied_element.GetNumNodes());
        //check that this version of the copy constructor gives the copied
        //element the same index number
        TS_ASSERT_EQUALS(copied_element.GetIndex(), 31415u);

        for (int node = 0; node < 4; node++)
        {
            for (int dimension = 0; dimension < 3; dimension++)
            {
                TS_ASSERT_EQUALS(element.GetNodeLocation(node, dimension), copied_element.GetNodeLocation(node, dimension));
            }
        }

        Element<3,3> another_copied_element(element, 2345);
        TS_ASSERT_EQUALS(another_copied_element.GetIndex(), 2345u);

        // update a node of the element
        Node<3>* update_for_node= new Node<3>(4, false, 0.0, 0.0, 2.0);
        another_copied_element.UpdateNode(1, update_for_node);
        TS_ASSERT_EQUALS(another_copied_element.GetNodeLocation(1, 2), 2.0);

        for (unsigned i=0; i<corner_nodes.size(); i++)
        {
            delete corner_nodes[i];
        }
        delete update_for_node;

    }

    void TestBoundaryElement()
    {
        std::vector<Node<3>*> nodes;
        nodes.push_back(new Node<3>(0, false, 0.0, 0.0, 0.0));
        nodes.push_back(new Node<3>(1, false, 1.0, 0.0, 0.0));
        nodes.push_back(new Node<3>(2, false, 0.0, 1.0, 0.0));

        BoundaryElement<2,3> element(INDEX_IS_NOT_USED, nodes);
        c_vector<double, 3> weighted_direction;
        double det;
        element.CalculateWeightedDirection(weighted_direction,det);
        TS_ASSERT_DELTA(det, 1.0, 1e-6);

        //Alter to be collinear (for coverage)
        nodes[2]->rGetModifiableLocation()[1]=0.0;
        TS_ASSERT_THROWS_ANYTHING(element.CalculateWeightedDirection(weighted_direction,det));

        //Alter to be deleted (for coverage)
        element.MarkAsDeleted();
        TS_ASSERT_THROWS_ANYTHING(element.CalculateWeightedDirection(weighted_direction,det));


        Node<3> *fake_node = new Node<3>(0, false, 0.0, 0.0, 0.0);
        TS_ASSERT_THROWS_ANYTHING(element.ReplaceNode(fake_node,fake_node));
        delete fake_node;

        for (unsigned i=0; i<nodes.size(); i++)
        {
            delete nodes[i];
        }
        
        element.SetRegion(3);
        TS_ASSERT_EQUALS(element.GetRegion(),3U);
    }

    void TestCircum1d(void)
    {
        std::vector<Node<1>*> nodes;
        nodes.push_back(new Node<1>(0, false, 10.0));
        nodes.push_back(new Node<1>(1, false, 15.0));

        Element<1,1> element(0, nodes);

        c_vector <double, 2> circum=element.CalculateCircumsphere();
        TS_ASSERT_DELTA(circum[0], 12.5, 1e-7);
        TS_ASSERT_DELTA(sqrt(circum[1]), 2.5, 1e-7);

        TS_ASSERT_DELTA(element.CalculateCircumsphereVolume(), 5.0, 1e-7);
        TS_ASSERT_DELTA(element.CalculateQuality(), 1.0, 1e-7);

        for (unsigned i=0; i<nodes.size(); i++)
        {
            delete nodes[i];
        }
    }

    void TestCircum2d(void)
    {
        std::vector<Node<2>*> equilateral_nodes;
        equilateral_nodes.push_back(new Node<2>(0, false, 2.0, 0.0));
        equilateral_nodes.push_back(new Node<2>(1, false, -1.0,sqrt(3)));
        equilateral_nodes.push_back(new Node<2>(2, false, -1.0,-sqrt(3)));

        Element<2,2> equilateral_element(0, equilateral_nodes);

        c_vector <double, 3> circum=equilateral_element.CalculateCircumsphere();
        TS_ASSERT_DELTA(circum[0], 0.0, 1e-7);
        TS_ASSERT_DELTA(circum[1], 0.0, 1e-7);
        TS_ASSERT_DELTA(sqrt(circum[2]), 2.0, 1e-7);

        TS_ASSERT_DELTA(equilateral_element.CalculateCircumsphereVolume(), 4.0*M_PI, 1e-7);
        TS_ASSERT_DELTA(equilateral_element.CalculateQuality(), 1.0, 1e-7);

        std::vector<Node<2>*> right_angle_nodes;
        right_angle_nodes.push_back(new Node<2>(0, false, 0.0, 0.0));
        right_angle_nodes.push_back(new Node<2>(1, false, 1.0, 0.0));
        right_angle_nodes.push_back(new Node<2>(2, false, 0.0, 1.0));
        Element<2,2> right_angle_element(0, right_angle_nodes);

        c_vector <double, 3> circum2=right_angle_element.CalculateCircumsphere();
        TS_ASSERT_DELTA(circum2[0], 0.5, 1e-7);
        TS_ASSERT_DELTA(circum2[1], 0.5, 1e-7);
        TS_ASSERT_DELTA(sqrt(circum2[2]), 1.0/sqrt(2.0), 1e-7);

        TS_ASSERT_DELTA(right_angle_element.CalculateCircumsphereVolume(), M_PI_2, 1e-7);
        TS_ASSERT_DELTA(right_angle_element.CalculateQuality(), 4.0*sqrt(3.0)/9.0, 1e-7);

        for (unsigned i=0; i<equilateral_nodes.size(); i++)
        {
            delete equilateral_nodes[i];
        }
        for (unsigned i=0; i<right_angle_nodes.size(); i++)
        {
            delete right_angle_nodes[i];
        }
    }

    void TestCircum3d(void)
    {
        std::vector<Node<3>*> nodes;
        nodes.push_back(new Node<3>(0, false,  1.0,  1.0,  1.0));
        nodes.push_back(new Node<3>(1, false, -1.0, -1.0,  1.0));
        nodes.push_back(new Node<3>(2, false, -1.0,  1.0, -1.0));
        nodes.push_back(new Node<3>(3, false,  1.0, -1.0, -1.0));


        Element<3,3> element(0, nodes);

        c_vector <double, 4> circum=element.CalculateCircumsphere();
        TS_ASSERT_DELTA(circum[0], 0.0, 1e-7);
        TS_ASSERT_DELTA(circum[1], 0.0, 1e-7);
        TS_ASSERT_DELTA(circum[2], 0.0, 1e-7);
        TS_ASSERT_DELTA(sqrt(circum[3]), sqrt(3.0), 1e-7);

        TS_ASSERT_DELTA(element.CalculateCircumsphereVolume(), 4.0*M_PI*sqrt(3), 1e-7);
        TS_ASSERT_DELTA(element.CalculateCircumsphereVolume(), 4.0*M_PI*sqrt(3), 1e-7);
        TS_ASSERT_DELTA(element.CalculateQuality(), 1.0, 1e-7);

        std::vector<Node<3>*> right_angle_nodes;
        right_angle_nodes.push_back(new Node<3>(0, false, 0.0, 0.0, 0.0));
        right_angle_nodes.push_back(new Node<3>(1, false, 1.0, 0.0, 0.0));
        right_angle_nodes.push_back(new Node<3>(3, false, 0.0, 1.0, 0.0));
        right_angle_nodes.push_back(new Node<3>(2, false, 0.0, 0.0, 1.0));

        Element<3,3> right_angle_element(0, right_angle_nodes);

        c_vector <double, 4> circum2=right_angle_element.CalculateCircumsphere();
        TS_ASSERT_DELTA(circum2[0], 0.5, 1e-7);
        TS_ASSERT_DELTA(circum2[1], 0.5, 1e-7);
        TS_ASSERT_DELTA(circum2[2], 0.5, 1e-7);
        TS_ASSERT_DELTA(sqrt(circum2[3]), sqrt(3.0)/2.0, 1e-7);

        TS_ASSERT_DELTA(right_angle_element.CalculateCircumsphereVolume(), sqrt(3)*M_PI_2, 1e-7);
        TS_ASSERT_DELTA(right_angle_element.CalculateQuality(), 0.5, 1e-7);

        for (unsigned i=0; i<right_angle_nodes.size(); i++)
        {
            delete right_angle_nodes[i];
        }
        for (unsigned i=0; i<nodes.size(); i++)
        {
            delete nodes[i];
        }
    }

    void TestCentroidAndDirection(void)
    {
        c_vector<double,3> direction;
        c_vector<double,3> centroid;

        std::vector<Node<3>*> nodes;
        nodes.push_back(new Node<3>(0, false, 0.0, 0.0, 0.0));
        nodes.push_back(new Node<3>(1, false, 1.0, 0.0, 0.0));
        BoundaryElement<1,3> element_1d(0, nodes);

        double det;
        element_1d.CalculateWeightedDirection(direction,det);
        //1D element in higher space is orientated by vector between endpoints
        TS_ASSERT_EQUALS(direction[0],1.0);
        TS_ASSERT_EQUALS(direction[1],0.0);
        TS_ASSERT_EQUALS(direction[2],0.0);
        TS_ASSERT_EQUALS(det,1.0);

        centroid=element_1d.CalculateCentroid();
        TS_ASSERT_EQUALS(centroid[0], 0.5);
        TS_ASSERT_EQUALS(centroid[1], 0.0);
        TS_ASSERT_EQUALS(centroid[2], 0.0);


        nodes.push_back(new Node<3>(3, false, 0.0, 1.0, 0.0));
        BoundaryElement<2,3> element_2d(0, nodes);

        element_2d.CalculateWeightedDirection(direction,det);
        //2D element in higher space is orientated by a normal
        TS_ASSERT_EQUALS(direction[0],0.0);
        TS_ASSERT_EQUALS(direction[1],0.0);
        TS_ASSERT_EQUALS(direction[2],-1.0);
         TS_ASSERT_EQUALS(det,1.0);
         
        //Note it's negative z-axis:
        //Vertices are *clockwise* when we look at them from outward facing normal
        centroid=element_2d.CalculateCentroid();
        TS_ASSERT_DELTA(centroid[0], 1.0/3.0, 1e-8);
        TS_ASSERT_DELTA(centroid[1], 1.0/3.0, 1e-8);
        TS_ASSERT_EQUALS(centroid[2], 0.0);

        nodes.push_back(new Node<3>(2, false, 0.0, 0.0, 1.0));
        Element<3,3> element_3d(0, nodes);

        TS_ASSERT_THROWS_ANYTHING(element_3d.CalculateWeightedDirection(direction,det));
        //3D element in 3D space has no orientation (other than JacobianDeterminant)

        centroid=element_3d.CalculateCentroid();
        TS_ASSERT_DELTA(centroid[0], 0.25, 1e-8);
        TS_ASSERT_DELTA(centroid[1], 0.25, 1e-8);
        TS_ASSERT_EQUALS(centroid[2], 0.25 );

        for (unsigned i=0; i<nodes.size(); i++)
        {
            delete nodes[i];
        }
    }

    void TestFlagging()
    {
        std::vector<Node<3>*> nodes;
        nodes.push_back(new Node<3>(0, false, 0.0, 0.0, 0.0));
        nodes.push_back(new Node<3>(1, false, 1.0, 0.0, 0.0));
        nodes.push_back(new Node<3>(2, false, 0.0, 1.0, 0.0));
        nodes.push_back(new Node<3>(3, false, 0.0, 0.0, 1.0));
        Element<3,3> element(INDEX_IS_NOT_USED, nodes);

        TS_ASSERT_EQUALS(element.IsFlagged(), false);

        element.Flag();
        TS_ASSERT_EQUALS(element.IsFlagged(), true);

        element.Unflag();
        TS_ASSERT_EQUALS(element.IsFlagged(), false);

        for (unsigned i=0; i<nodes.size(); i++)
        {
            delete nodes[i];
        }
    }

    void Test1DRefineElement()
    {
        TrianglesMeshReader<1,1> mesh_reader("mesh/test/data/1D_0_to_1_10_elements");

        MutableMesh<1,1> mesh;

        mesh.ConstructFromMeshReader(mesh_reader);

        ChastePoint<1> new_point(0.01);
        Element<1,1>* p_first_element=mesh.GetElement(0);

        TS_ASSERT_THROWS_NOTHING(mesh.RefineElement(p_first_element,new_point));
        // Instead of a element with nodes at 0 and 0.1
        // there should be an element with nodes at 0 and 0.01 and
        // an element with nodes at 0.01 and 0.1. Other elements should stay the same

        const Node<1>* p_first_node=p_first_element->GetNode(0);
        const Node<1>* p_second_node=p_first_element->GetNode(1);

        TS_ASSERT_EQUALS(p_first_node->GetPoint().rGetLocation()(0), 0);
        TS_ASSERT_EQUALS(p_second_node->GetPoint().rGetLocation()(0), 0.01);

        // test second element

        Element<1,1> *p_second_element=mesh.GetElement(1);

        p_first_node=p_second_element->GetNode(0);
        p_second_node=p_second_element->GetNode(1);

        TS_ASSERT_EQUALS(p_first_node->GetPoint().rGetLocation()(0), 0.1);
        TS_ASSERT_EQUALS(p_second_node->GetPoint().rGetLocation()(0), 0.2);

        // test last element

        Element<1,1>* p_last_element=mesh.GetElement(10);

        p_first_node=p_last_element->GetNode(0);
        p_second_node=p_last_element->GetNode(1);

        TS_ASSERT_EQUALS(p_first_node->GetPoint().rGetLocation()(0), 0.01);
        TS_ASSERT_EQUALS(p_second_node->GetPoint().rGetLocation()(0), 0.1);

        // test jacobians
        c_matrix<double,1,1> jacobian;
        double det;
        p_first_element->CalculateJacobian(jacobian,det);
        TS_ASSERT_EQUALS(det, 0.01);
        p_second_element->CalculateJacobian(jacobian,det);
        TS_ASSERT_EQUALS(det, 0.1);
        p_last_element->CalculateJacobian(jacobian,det);
        TS_ASSERT_DELTA(det, 0.09,1e-6);

        // test mesh length
        TS_ASSERT_DELTA(mesh.CalculateVolume(), 1.0, 1e-6);
    }

    void Test1DRefineElementWithPointTooFarRightFails() throw (Exception)
    {
        TrianglesMeshReader<1,1> mesh_reader("mesh/test/data/1D_0_to_1_10_elements");

        MutableMesh<1,1> mesh;

        mesh.ConstructFromMeshReader(mesh_reader);

        ChastePoint<1> new_point(0.11);
        Element<1,1>* p_first_element=mesh.GetElement(0);

        // Trying to add Point(0.11) to Element(0)
        // This point is contained in Element(1)
        TS_ASSERT_THROWS_ANYTHING(mesh.RefineElement(p_first_element, new_point));

    }

    void Test1DRefineElementWithPointTooFarLeftFails() throw (Exception)
    {
        TrianglesMeshReader<1,1> mesh_reader("mesh/test/data/1D_0_to_1_10_elements");

        MutableMesh<1,1> mesh;

        mesh.ConstructFromMeshReader(mesh_reader);

        ChastePoint<1> new_point(-0.1);
        Element<1,1>* p_first_element=mesh.GetElement(0);

        // Trying to add Point(-0.1) to Element(0)
        // This point is to the left of Element(0)
        TS_ASSERT_THROWS_ANYTHING(mesh.RefineElement(p_first_element, new_point));

    }

    void Test1DRefineElementManyNodes() throw (Exception)
    {
        TrianglesMeshReader<1,1> mesh_reader("mesh/test/data/1D_0_to_1_10_elements");

        MutableMesh<1,1> mesh;

        mesh.ConstructFromMeshReader(mesh_reader);

        Element<1,1>* p_first_element=mesh.GetElement(0);

        //There's space on the node vector for 10 new points
        // but more than 10 should still work
        for (int i=1; i<=20; i++)
        {
            ChastePoint<1> new_point(0.1 - i*0.0005);
            mesh.RefineElement(p_first_element,new_point);
        }

    }

    void Test2DRefineElement() throw (Exception)
    {
        TrianglesMeshReader<2,2> mesh_reader("mesh/test/data/2D_0_to_1mm_200_elements");

        MutableMesh<2,2> mesh;

        mesh.ConstructFromMeshReader(mesh_reader);

        TS_ASSERT_DELTA(mesh.CalculateVolume(), 0.01,1e-6);
        TS_ASSERT_DELTA(mesh.CalculateSurfaceArea(), 0.4,1e-6);


        // Refine an element in the bottom right corner
        Element<2,2>* p_corner_element=mesh.GetElement(18);
        
        c_matrix<double,2,2> jacobian;
        double det;
        p_corner_element->CalculateJacobian(jacobian,det);
        TS_ASSERT_DELTA(det,0.0001 , 1e-6);

        // Point to be inserted in the bottom right corner
        ChastePoint<2> new_point(0.095,0.003);

        TS_ASSERT_THROWS_NOTHING(mesh.RefineElement(p_corner_element,new_point));

        //Testing the JacobianDeterminant of the element that has changed
        p_corner_element->CalculateJacobian(jacobian,det);
        TS_ASSERT_DELTA(det,3e-05 , 1e-6);

        //Testing invariants
        TS_ASSERT_DELTA(mesh.CalculateVolume(), 0.01,1e-6);
        TS_ASSERT_DELTA(mesh.CalculateSurfaceArea(), 0.4,1e-6);


        // Refine an element in the middle of the mesh
        Element<2,2>* p_middle_element=mesh.GetElement(108);

        p_middle_element->CalculateJacobian(jacobian,det);
        TS_ASSERT_DELTA(det,0.0001 , 1e-6);

        // Point to be inserted in the middle of the mesh
        ChastePoint<2> new_point1(0.045,0.053);

        TS_ASSERT_THROWS_NOTHING(mesh.RefineElement(p_middle_element,new_point1));

        //Testing the JacobianDeterminant of the element that has changed
        p_middle_element->CalculateJacobian(jacobian,det);
        TS_ASSERT_DELTA(det,3e-05 , 1e-6);

        //Testing invariants
        TS_ASSERT_DELTA(mesh.CalculateVolume(), 0.01,1e-6);
        TS_ASSERT_DELTA(mesh.CalculateSurfaceArea(), 0.4,1e-6);
    }

    void Test2DRefineElementFails() throw (Exception)
    {
        TrianglesMeshReader<2,2> mesh_reader("mesh/test/data/2D_0_to_1mm_200_elements");

        MutableMesh<2,2> mesh;

        mesh.ConstructFromMeshReader(mesh_reader);

        // Refine an element in the bottom right corner
        Element<2,2>* p_corner_element=mesh.GetElement(18);

        // Point to be inserted on the edge of the element
        ChastePoint<2> new_point(0.095,0.005);

        //Shouldn't be able to insert this point at the edge of the element
        TS_ASSERT_THROWS_ANYTHING(mesh.RefineElement(p_corner_element,new_point));

    }

    void Test3DRefineElement() throw (Exception)
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");

        MutableMesh<3,3> mesh;

        mesh.ConstructFromMeshReader(mesh_reader);

        TS_ASSERT_DELTA(mesh.CalculateVolume(), 1.0, 1e-6);
        TS_ASSERT_DELTA(mesh.CalculateSurfaceArea(), 6.0, 1e-6);


        // Refine an element in the top corner (1, 1, 1)
        Element<3,3>* p_corner_element=mesh.GetElement(64);
        c_matrix<double,3,3> jacobian;
        double det;
        p_corner_element->CalculateJacobian(jacobian,det);
        TS_ASSERT_DELTA(det,0.03125 , 1e-6);

        // Point to be inserted in the top corner
        ChastePoint<3> new_point(0.9,0.75,0.9);

        TS_ASSERT_THROWS_NOTHING(mesh.RefineElement(p_corner_element,new_point));

        //Testing the JacobianDeterminant of the element that has changed
        p_corner_element->CalculateJacobian(jacobian,det);        
        TS_ASSERT_DELTA(det, 0.0125 , 1e-6);

        //Testing invariants
        TS_ASSERT_DELTA(mesh.CalculateVolume(), 1.0, 1e-6);
        TS_ASSERT_DELTA(mesh.CalculateSurfaceArea(), 6.0, 1e-6);



        // Refine an element which includes the middle node
        Element<3,3>* p_middle_element=mesh.GetElement(49);
        p_middle_element->CalculateJacobian(jacobian,det);
        TS_ASSERT_DELTA(det,0.0625 , 1e-6);

        // Point to be inserted near node 22 (middle of cube)
        ChastePoint<3> new_point1(0.49, 0.47, 0.6);

        TS_ASSERT_THROWS_NOTHING(mesh.RefineElement(p_middle_element, new_point1));

        //Testing the JacobianDeterminant of the element that has changed
        p_middle_element->CalculateJacobian(jacobian,det);
        TS_ASSERT_DELTA(det, 0.01125 , 1e-6);

        //Testing invariants
        TS_ASSERT_DELTA(mesh.CalculateVolume(), 1.0, 1e-6);
        TS_ASSERT_DELTA(mesh.CalculateSurfaceArea(), 6.0, 1e-6);
    }

    void Test3DRefineElementFails() throw (Exception)
    {
        TrianglesMeshReader<3,3> mesh_reader("mesh/test/data/cube_136_elements");

        MutableMesh<3,3> mesh;

        mesh.ConstructFromMeshReader(mesh_reader);

        TS_ASSERT_DELTA(mesh.CalculateVolume(), 1.0, 1e-6);
        TS_ASSERT_DELTA(mesh.CalculateSurfaceArea(), 6.0, 1e-6);


        // Refine an element in the top corner (1, 1, 1)
        Element<3,3>* p_corner_element=mesh.GetElement(64);
        c_matrix<double,3,3> jacobian;
        double det;
        p_corner_element->CalculateJacobian(jacobian,det);
        TS_ASSERT_DELTA(det,0.03125,1e-6);

        // Point to be inserted in wrong place
        ChastePoint<3> new_point(0.9,0.75,1.0);

        TS_ASSERT_THROWS_ANYTHING(mesh.RefineElement(p_corner_element,new_point));
    }

    void TestGetStiffnessMatrixGlobalIndices ( void )
    {
        unsigned PROBLEM_DIM = 1;

        TrianglesMeshReader<2,2> mesh_reader("mesh/test/data/square_4_elements");
        TetrahedralMesh<2,2> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);

        Element<2,2>* p_element = mesh.GetElement(2);

        unsigned indices_1[3];
        // Element 2 in the mesh has global node indices: 1,2,4
        unsigned expected_indices_1[]={1u,2u,4u};
        p_element->GetStiffnessMatrixGlobalIndices(PROBLEM_DIM,indices_1);
        TS_ASSERT_SAME_DATA(indices_1, expected_indices_1, 3*sizeof(unsigned));

        PROBLEM_DIM=2;
        unsigned indices_2[6];
        unsigned expected_indices_2[]={2u,3u,4u,5u,8u,9u};

        p_element->GetStiffnessMatrixGlobalIndices(PROBLEM_DIM,indices_2);

        TS_ASSERT_SAME_DATA(indices_2, expected_indices_2, 6*sizeof(unsigned));
        
        p_element->SetRegion(3);
        TS_ASSERT_EQUALS(p_element->GetRegion(),3U);
    }

};

#endif //_TESTELEMENT_HPP_
