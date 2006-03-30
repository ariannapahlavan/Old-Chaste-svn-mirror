#ifndef _CONFORMINGTETRAHEDRALMESH_CPP_
#define _CONFORMINGTETRAHEDRALMESH_CPP_

#include "ConformingTetrahedralMesh.hpp"
#include "Exception.hpp"

#include <vector>
#include <map>
#include <set>
#include <algorithm>

template<int ELEMENT_DIM, int SPACE_DIM>
ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::ConformingTetrahedralMesh()
{
}

template<int ELEMENT_DIM, int SPACE_DIM>
ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::ConformingTetrahedralMesh(long numElements)
{
    mElements.reserve(numElements);
}



template<int ELEMENT_DIM, int SPACE_DIM>
void ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::ConstructFromMeshReader(AbstractMeshReader &rMeshReader, int orderOfBasisFunctions)
{
	// Check dimension matches the data	
	if (SPACE_DIM != rMeshReader.GetDimension())
	{		
		throw Exception("Mesh and MeshReader dimensions do not agree.");
	}
	
	// We only use linear or quadratic basis functions
	assert(orderOfBasisFunctions == 1 || orderOfBasisFunctions == 2);
	
	// Record number of corner nodes
	mNumCornerNodes = rMeshReader.GetNumNodes();

	// Reserve memory for nodes, so we don't have problems with pointers stored in
	// elements becoming invalid.
	// For quadratic basis functions we need to go through and count them first
	
	if (orderOfBasisFunctions == 1)
	{
		mNodes.reserve(mNumCornerNodes);
	}
	else if (orderOfBasisFunctions == 2)
	{
		typename std::map<std::pair<int,int>,int>::const_iterator iterator;
		std::map<std::pair<int,int>,int> internal_nodes_map;
		
		std::vector<Node<SPACE_DIM> > temp_nodes;
		temp_nodes.reserve(mNumCornerNodes);
		
		// Add corner nodes to temporary nodes vector
		std::vector<double> coords;
		for (int i=0; i < mNumCornerNodes; i++)
		{
			coords = rMeshReader.GetNextNode();
			temp_nodes.push_back(Node<SPACE_DIM>(i, Point<SPACE_DIM>(coords), false));
		}
		
		int new_node_index = mNumCornerNodes;		
		
		// Create the map of internal node indices
		for (int i=0; i < rMeshReader.GetNumElements(); i++)
		{
			std::vector<int> node_indices = rMeshReader.GetNextElement();
			std::vector<const Node<SPACE_DIM>*> nodes;
		    
		    //Debugging code
		    int nodes_size = node_indices.size();
		    
			for (int j=0; j < nodes_size; j++)
			{
				assert(node_indices[j] < (int) temp_nodes.size());
				nodes.push_back(&temp_nodes[node_indices[j]]);
			}
						
			for (int j=0; j < ELEMENT_DIM + 1; j++)
			{
				for (int k=j+1; k < ELEMENT_DIM + 1; k++)
				{
					assert(j < nodes_size && k < nodes_size);					
					int node_i = nodes[j]->GetIndex();
					int node_j = nodes[k]->GetIndex();
					if (node_j < node_i)
					{
						int temp = node_i;
						node_i = node_j;
						node_j = temp;
					}
					iterator = internal_nodes_map.find(std::pair<int,int>(node_i, node_j));
					if (iterator == internal_nodes_map.end())
					{
						// add node to map
						internal_nodes_map[(std::pair<int,int>(node_i, node_j))] = new_node_index;
						new_node_index++;
					}
				}
			}	
		}		
		mNodes.reserve(new_node_index);				
	}
	
	rMeshReader.Reset();	
	
	typename std::map<std::pair<int,int>,int>::const_iterator iterator;
	std::map<std::pair<int,int>,int> internal_nodes_map;
	
	// Add corner nodes
	std::vector<double> coords;
	for (int i=0; i < mNumCornerNodes; i++)
	{
		coords = rMeshReader.GetNextNode();
		mNodes.push_back(Node<SPACE_DIM>(i, Point<SPACE_DIM>(coords), false));
	}
	
    int new_node_index = mNumCornerNodes;
	
	if (orderOfBasisFunctions == 2)
	{
		for (int i=0; i < rMeshReader.GetNumElements(); i++)
		{
			std::vector<int> node_indices = rMeshReader.GetNextElement();
			std::vector<const Node<SPACE_DIM>*> nodes;
		    
		    //Debugging code
		    int nodes_size = node_indices.size();
		    
			for (int j=0; j < nodes_size; j++)
			{
				assert(node_indices[j] < mNodes.size());
				nodes.push_back(&mNodes[node_indices[j]]);
			}
						
			for (int j=0; j < ELEMENT_DIM + 1; j++)
			{
				for (int k=j+1; k < ELEMENT_DIM + 1; k++)
				{
					assert(j < nodes_size && k < nodes_size);					
					int node_i = nodes[j]->GetIndex();
					int node_j = nodes[k]->GetIndex();
					if (node_j < node_i)
					{
						int temp = node_i;
						node_i = node_j;
						node_j = temp;
					}
					iterator = internal_nodes_map.find(std::pair<int,int>(node_i, node_j));
					if (iterator == internal_nodes_map.end())
					{
						// add node to map
						internal_nodes_map[(std::pair<int,int>(node_i, node_j))] = new_node_index;
						// add node to mesh
       					const Node<SPACE_DIM>* node1 = GetNodeAt(node_i);
		    			const Node<SPACE_DIM>* node2 = GetNodeAt(node_j);
			    		Node<SPACE_DIM> new_node(new_node_index,
											 node1->GetPoint().MidPoint(node2->GetPoint()),
											 node1->IsBoundaryNode() && node2->IsBoundaryNode());
				    	mNodes.push_back(new_node);
						new_node_index++;
					}
				}
			}	
		}		
	}			
	rMeshReader.Reset();
	// Add elements
	new_node_index = mNumCornerNodes;
    mElements.reserve(rMeshReader.GetNumElements());
	
	for (int i=0; i < rMeshReader.GetNumElements(); i++)
	{
		std::vector<int> node_indices = rMeshReader.GetNextElement();
		std::vector<Node<SPACE_DIM>*> nodes;
		int nodes_size = node_indices.size();
		for (int j=0; j<nodes_size; j++)
		{
			assert(node_indices[j] < mNodes.size());
			nodes.push_back(&mNodes[node_indices[j]]);
		}
        
		if (orderOfBasisFunctions == 2)
		{
			for (int j=0; j < ELEMENT_DIM + 1; j++)
			{
				for (int k=j+1; k < ELEMENT_DIM + 1; k++)
				{
					assert(j < nodes_size && k < nodes_size);
					int node_i = nodes[j]->GetIndex();
					int node_j = nodes[k]->GetIndex();
					if (node_j < node_i)
					{
						int temp = node_i;
						node_i = node_j;
						node_j = temp;
					}
					iterator = internal_nodes_map.find(std::pair<int,int>(node_i, node_j));
					assert(iterator != internal_nodes_map.end());										
					// add node to element
					nodes.push_back(this->GetNodeAt(iterator->second));
					new_node_index++;
				}
			}
		}
		mElements.push_back(Element<ELEMENT_DIM,SPACE_DIM>(nodes,orderOfBasisFunctions));
	}
	
	// Add boundary elements & nodes
	for (int i=0; i<rMeshReader.GetNumFaces(); i++)
	{
		std::vector<int> node_indices = rMeshReader.GetNextFace();
        
        // Determine if this is a boundary face
        std::set<const void *> containing_elements; // Elements that contain this face
		std::vector<Node<SPACE_DIM>*> nodes;
		for (int j=0; j<node_indices.size(); j++)
		{
			assert(node_indices[j] < mNodes.size());
			// Add Node pointer to list for creating an element
			nodes.push_back(&mNodes[node_indices[j]]);
            
//            // Debugging
//            std::cout << " " << node_indices[j] << ":[";
//            std::set<const void *> s = nodes[j]->rGetContainingElements();
//            for (std::set<const void *>::iterator it = s.begin(); it != s.end(); it++)
//            {
//                const Element<ELEMENT_DIM, SPACE_DIM> *e =
//                    (const Element<ELEMENT_DIM, SPACE_DIM> *) *it;
//                std::cout << "(";
//                for (int n=0; n<e->GetNumNodes(); n++)
//                {
//                    const Node<SPACE_DIM> *node = e->GetNode(n);
//                    if (n != 0) std::cout << ",";
//                    std::cout << node->GetIndex();
//                }
//                std::cout << ")";
//            }
//            std::cout << "]";
            
            // Work out what elements contain this face, by taking the intersection
            // of the sets of elements containing each node in the face.
            if (j == 0)
            {
                containing_elements = nodes[j]->rGetContainingElements();
            } else {
                std::set<const void *> temp;
                std::set_intersection(nodes[j]->rGetContainingElements().begin(),
                                      nodes[j]->rGetContainingElements().end(),
                                      containing_elements.begin(), containing_elements.end(),
                                      std::inserter(temp, temp.begin()));
                containing_elements = temp;
            }
		}
        
//        std::cout << " " << containing_elements.size() << std::endl;
        if (containing_elements.size() == 1)
        {
            // This is a boundary face
            // Ensure all its nodes are marked as boundary nodes
            for (int j=0; j<nodes.size(); j++)
            {
                if (!nodes[j]->IsBoundaryNode())
                {
                    nodes[j]->SetAsBoundaryNode();
                    mBoundaryNodes.push_back(nodes[j]);
                }
            }

		
    		if (orderOfBasisFunctions == 2)
    		{
    			int nodes_size = node_indices.size();
    			for (int j=0; j < ELEMENT_DIM; j++)
    			{
    				for (int k=j+1; k < ELEMENT_DIM; k++)
    				{
    					assert(j < nodes_size && k < nodes_size);
    					int node_i = nodes[j]->GetIndex();
    					int node_j = nodes[k]->GetIndex();
    					if (node_j < node_i)
    					{
    						int temp = node_i;
    						node_i = node_j;
    						node_j = temp;
    					}
    					iterator = internal_nodes_map.find(std::pair<int,int>(node_i, node_j));
    					assert(iterator != internal_nodes_map.end());
    					// add node to element
    					nodes.push_back(this->GetNodeAt(iterator->second));
    				}
    			}
    		}
    		
    		// The added elements will be deleted in our destructor
    		mBoundaryElements.push_back(new Element<ELEMENT_DIM-1,SPACE_DIM>(nodes,orderOfBasisFunctions));
    	}
    }
}

template<int ELEMENT_DIM, int SPACE_DIM>
ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::~ConformingTetrahedralMesh()
{
	// Iterate over boundary elements and free the memory
	BoundaryElementIterator it = GetBoundaryElementIteratorBegin();
	while (it != GetBoundaryElementIteratorEnd())
	{
		delete (*it);
		it++;
	}
}

template<int ELEMENT_DIM, int SPACE_DIM>
void ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::AddElement(Element<ELEMENT_DIM, SPACE_DIM> newElement)
{
    mElements.push_back(newElement);
}

template<int ELEMENT_DIM, int SPACE_DIM>
void ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::AddNode(Node<SPACE_DIM> newNode)
{
	newNode.SetIndex(mNodes.size());
    mNodes.push_back(newNode);
}

//template<int ELEMENT_DIM, int SPACE_DIM>
//void ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::AddSurfaceElement(const Element<ELEMENT_DIM-1, SPACE_DIM> *pNewElement)
//{
//	mBoundaryElements.push_back(pNewElement);
//}

/**
 * Get a node reference from the mesh.
 * 
 * Note that this may become invalid if nodes are subsequently added to the mesh.
 */
template<int ELEMENT_DIM, int SPACE_DIM>
Node<SPACE_DIM> *ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::GetNodeAt(long index)
{
	assert(index < (long) mNodes.size());
    return &(mNodes[index]);
}

// Keep this for the moment (does exactly the same as GetNumAllNodes)
template<int ELEMENT_DIM, int SPACE_DIM>
long ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::GetNumNodes()
{
    return mNodes.size();
}

template<int ELEMENT_DIM, int SPACE_DIM>
long ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::GetNumElements()
{
    return mElements.size();
}

template<int ELEMENT_DIM, int SPACE_DIM>
long ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::GetNumBoundaryNodes()
{
    return mBoundaryNodes.size();
}

template<int ELEMENT_DIM, int SPACE_DIM>
long ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::GetNumBoundaryElements()
{
    return mBoundaryElements.size();
}

template<int ELEMENT_DIM, int SPACE_DIM>
long ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::GetNumCornerNodes()
{
    return mNumCornerNodes;
}

template<int ELEMENT_DIM, int SPACE_DIM>
long ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::GetNumAllNodes()
{
    return mNodes.size();
}

template<int ELEMENT_DIM, int SPACE_DIM>
void ConformingTetrahedralMesh<ELEMENT_DIM, SPACE_DIM>::RescaleMeshFromBoundaryNode(Point<1> updatedPoint, int boundaryNodeIndex)
{
    
    double scaleFactor = updatedPoint[0] / GetNodeAt(boundaryNodeIndex)->GetPoint()[0];
    double temp;
    for(int i=0; i < boundaryNodeIndex+1; i++)
    {
        temp = scaleFactor * mNodes[i].GetPoint()[0];
        Point<1> newPoint(temp);
        mNodes[i].SetPoint(newPoint);
    }
    
}


#endif // _CONFORMINGTETRAHEDRALMESH_CPP_
