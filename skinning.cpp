#include "skinning.h"
#include "vec3d.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
using namespace std;

// CSCI 520 Computer Animation and Simulation
// Spring 2019
// Jernej Barbic and Yijing Li
// Avishkar Kolahalu

Skinning::Skinning(int numMeshVertices, const double * restMeshVertexPositions, const std::string & meshSkinningWeightsFilename)
{
	this->numMeshVertices = numMeshVertices;
	this->restMeshVertexPositions = restMeshVertexPositions;

	cout << "Loading skinning weights..." << endl;
	ifstream fin(meshSkinningWeightsFilename.c_str());
	assert(fin);
	int numWeightMatrixRows = 0, numWeightMatrixCols = 0;
	fin >> numWeightMatrixRows >> numWeightMatrixCols;
	assert(fin.fail() == false);
	assert(numWeightMatrixRows == numMeshVertices);
	int numJoints = numWeightMatrixCols;

	vector<vector<int>> weightMatrixColumnIndices(numWeightMatrixRows);
	vector<vector<double>> weightMatrixEntries(numWeightMatrixRows);
	fin >> ws;
	while(fin.eof() == false)
	{
		int rowID = 0, colID = 0;
		double w = 0.0;
		fin >> rowID >> colID >> w;
		weightMatrixColumnIndices[rowID].push_back(colID);
		weightMatrixEntries[rowID].push_back(w);
		assert(fin.fail() == false);
		fin >> ws;
	}
	fin.close();

	// Build skinning joints and weights.
	numJointsInfluencingEachVertex = 0;
	for (int i = 0; i < numMeshVertices; i++)
	numJointsInfluencingEachVertex = std::max(numJointsInfluencingEachVertex, (int)weightMatrixEntries[i].size());
	assert(numJointsInfluencingEachVertex >= 2);

	// Copy skinning weights from SparseMatrix into meshSkinningJoints and meshSkinningWeights.
	meshSkinningJoints.assign(numJointsInfluencingEachVertex * numMeshVertices, 0);
	meshSkinningWeights.assign(numJointsInfluencingEachVertex * numMeshVertices, 0.0);
	for (int vtxID = 0; vtxID < numMeshVertices; vtxID++)
	{
		vector<pair<double, int>> sortBuffer(numJointsInfluencingEachVertex);
		for (size_t j = 0; j < weightMatrixEntries[vtxID].size(); j++)
		{
			int frameID = weightMatrixColumnIndices[vtxID][j];
			double weight = weightMatrixEntries[vtxID][j];
			sortBuffer[j] = make_pair(weight, frameID);
		}
		sortBuffer.resize(weightMatrixEntries[vtxID].size());
		assert(sortBuffer.size() > 0);
		sort(sortBuffer.rbegin(), sortBuffer.rend()); // sort in descending order using reverse_iterators
		for(size_t i = 0; i < sortBuffer.size(); i++)
		{
			meshSkinningJoints[vtxID * numJointsInfluencingEachVertex + i] = sortBuffer[i].second;
			meshSkinningWeights[vtxID * numJointsInfluencingEachVertex + i] = sortBuffer[i].first;
		}

		// Note: When the number of joints used on this vertex is smaller than numJointsInfluencingEachVertex,
		// the remaining empty entries are initialized to zero due to vector::assign(XX, 0.0) .
	}
}

void Skinning::applySkinning(const RigidTransform4d * jointSkinTransforms, double * newMeshVertexPositions) const
{
	// Create & initialise new and rest position vectors.
	vector<Vec3d> newPositions(numMeshVertices), restPositions(numMeshVertices);
	for (int i = 0; i < numMeshVertices; ++i)
	{
		newPositions[i] = Vec3d(0, 0, 0);
		restPositions[i] = Vec3d(restMeshVertexPositions[3 * i], restMeshVertexPositions[3 * i + 1], restMeshVertexPositions[3 * i + 2]);
	}

	// Skinning:
	for(int i = 0; i < numMeshVertices; ++i)
	{
		for (size_t j = 0; j < numJointsInfluencingEachVertex; ++j)
			// Summation: wj Mj pbar_i:
			newPositions[i] += meshSkinningWeights[i * numJointsInfluencingEachVertex + j] *
				jointSkinTransforms[meshSkinningJoints[i * numJointsInfluencingEachVertex + j]].transformPoint(restPositions[i]);
		

		// Assign the new positions to newMeshVertexPositions :

		newMeshVertexPositions[3 * i] = newPositions[i][0];
		newMeshVertexPositions[3 * i + 1] = newPositions[i][1];
		newMeshVertexPositions[3 * i + 2] = newPositions[i][2];
	}
}

