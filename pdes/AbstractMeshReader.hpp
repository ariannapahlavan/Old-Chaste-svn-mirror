// AbstractMeshReader.hpp

/**
 * Abstract mesh reader class. Reads output generated by a mesh generator
 * and converts it to a standard format for use in constructing a finite
 * element mesh structure.
 * 
 * A derived class TrianglesMeshReader exists for reading meshes generated
 * by Triangles (in 2-d) and TetGen (in 3-d).
 * 
 * A derived class MemfemMeshReader reads 3D data from the Tulane University code
 * 
 * A derived class FemlabMeshReader reads 2D data from Femlab or Matlab PDEToolbox
 * 
 * The superclass "AbstractMeshReadWrite" contains <strong>all</strong>
 * data necessary to either read or write a mesh.
 */

#ifndef _ABSTRACTMESHREADER_HPP_
#define _ABSTRACTMESHREADER_HPP_


#include "AbstractMeshReadWrite.hpp"

class AbstractMeshReader : public AbstractMeshReadWrite
{
	protected:
		
		std::vector<std::string> GetRawDataFromFile(std::string fileName); /**< Reads an input file fileName, removes comments (indicated by a #) and blank lines */
		std::vector< std::vector<int> > CullInternalFaces(); /**< Remove internal faces and store the result in mBoundaryFaceData */
		
	public:
		
		int GetNumElements() const {return mNumElements;} /**< Returns the number of elements in the mesh */
		int GetNumNodes() const {return mNumNodes;} /**< Returns the number of nodes in the mesh */
		int GetNumFaces() const {return mNumFaces;} /**< Returns the number of faces in the mesh (synonym of GetNumEdges()) */
		int GetNumBoundaryFaces() const {return mNumBoundaryFaces;} /**< Returns the number of boundary faces in the mesh (synonym of GetNumBoundaryEdges()) */
		int GetNumEdges() const {return mNumFaces;}	/**< Returns the number of edges in the mesh (synonym of GetNumFaces()) */
		int GetNumBoundaryEdges() const {return mNumBoundaryFaces;}	/**< Returns the number of boundary edges in the mesh (synonym of GetNumBoundaryFaces()) */		
		int GetDimension() const {return mDimension;} /**< Returns the dimension of the system */
		
		int GetMaxNodeIndex(); /**< Returns the maximum node index */
		int GetMinNodeIndex(); /**< Returns the minimum node index */
		
		std::vector<double> GetNextNode(); /**< Returns a vector of the coordinates of each node in turn */
		std::vector<int> GetNextElement(); /**< Returns a vector of the nodes of each element in turn */
		std::vector<int> GetNextEdge(); /**< Returns a vector of the nodes of each edge in turn (synonym of GetNextFace()) */
		std::vector<int> GetNextBoundaryEdge(); /**< Returns a vector of the nodes of each boundary edge in turn (synonym of GetNextBoundaryFace()) */		
		std::vector<int> GetNextFace(); /**< Returns a vector of the nodes of each face in turn (synonym of GetNextEdge()) */
		std::vector<int> GetNextBoundaryFace(); /**< Returns a vector of the nodes of each boundary face in turn (synonym of GetNextBoundaryEdge()) */		

};
#endif //_ABSTRACTMESHREADER_HPP_
