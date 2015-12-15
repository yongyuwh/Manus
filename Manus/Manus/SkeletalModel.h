/*
   Copyright 2015 Manus VR

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#pragma once

#include "Manus.h"
#include "Glove.h"
#include <fbxsdk.h>

class SkeletalModel
{
private:
	FbxManager* m_sdk_manager;
	FbxScene* m_scene[2];
	FbxNode* m_bone_nodes[2][GLOVE_FINGERS][4];
	
	GLOVE_POSE ToGlovePose(FbxAMatrix mat, GLOVE_QUATERNION &Quat);
	

public:
	SkeletalModel();
	~SkeletalModel();

	bool InitializeScene();
	bool Simulate(const GLOVE_DATA data, GLOVE_SKELETAL* model, GLOVE_HAND hand, bool OSVR_Compat = false);
};