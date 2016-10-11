#pragma once

#include "Manus.h"
#include <Shlobj.h>
#include "json.hpp"
using json = nlohmann::json;

typedef struct IK_SETTINGS {
	double shoulderLength;
	double upperArmLength;
	double lowerArmLength;
	double upperNeckLength;
	double lowerNeckLength;
	IK_VECTOR upperNeckOffset;

	int iterations;

	IK_SETTINGS() {
		shoulderLength = 0.4f;
		upperArmLength = 0.27f;
		lowerArmLength = 0.28f;
		upperNeckLength = 0.2f;
		lowerNeckLength = 0.17f;
		upperNeckOffset.x = 0.0f;
		upperNeckOffset.y = 0.05f;
		upperNeckOffset.z = 0.16f;

		iterations = 10;
	}

	IK_SETTINGS(json j) {
		shoulderLength = j["shoulderLength"];
		upperArmLength = j["upperArmLength"];
		lowerArmLength = j["lowerArmLength"];
		upperNeckLength = j["upperNeckLength"];
		lowerNeckLength = j["lowerNeckLength"];
		upperNeckOffset.x = j["upperNeckOffset"]["x"];
		upperNeckOffset.y = j["upperNeckOffset"]["y"];
		upperNeckOffset.z = j["upperNeckOffset"]["z"];
		iterations = j["iterations"];
	}

	json toJson() {
		json j = {
			{ "shoulderLength", shoulderLength },
			{ "upperArmLength", upperArmLength },
			{ "lowerArmLength", lowerArmLength },
			{ "upperNeckLength", upperNeckLength },
			{ "lowerNeckLength", lowerNeckLength },
			{ "upperNeckOffset",{
				{ "x", upperNeckOffset.x },
				{ "y", upperNeckOffset.y },
				{ "z", upperNeckOffset.z }
			} },
			{ "iterations", iterations }
		};

		return j;
	}

} IK_SETTINGS;

class SettingsManager
{
private:
	// vars
	static IK_SETTINGS *ik_settings;
	static bool initialized;

public:
	// functions
	SettingsManager();
	~SettingsManager();

	void loadSettings();
	static IK_SETTINGS getSettings();
};
