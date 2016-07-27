#include "stdafx.h"
#include "SettingsManager.h"
#include <wchar.h>

#define MANUS_LOCATION L"/ManusVR"
#define MANUS_BODY_SETTINGS L"/body.json"

IK_SETTINGS *SettingsManager::ik_settings;
bool SettingsManager::initialized;

SettingsManager::SettingsManager()
{
	ik_settings = new IK_SETTINGS();

	this->loadSettings();
}

SettingsManager::~SettingsManager()
{
}

IK_SETTINGS SettingsManager::getSettings()
{
	if (!initialized)
	{
		return false;
	}
	else
	{
		// return the data that ik_settings points to
		return *ik_settings;
	}
}

void SettingsManager::loadSettings()
{
	wchar_t* localAppData;
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &localAppData);
	// concat the MANUS_LOCATION folder constant
	wcsncat(localAppData, MANUS_LOCATION, 30);
	// check if the MANUS_LOCATION folder exists, if not, create it
	DWORD ftyp = GetFileAttributes(localAppData);
	if (ftyp == INVALID_FILE_ATTRIBUTES) {
		// create the folder
		_wmkdir(localAppData);
	}
	// concat the MANUS_BODY_SETTINGS 
	wcsncat(localAppData, MANUS_BODY_SETTINGS, 30);

	printf("%S", localAppData);
	FILE *file = _wfopen(localAppData, L"r");
	if (file) {
		// read the file and load the settings
		std::string buffer;
		// get the file size 
		struct _stat fileinfo;
		_wstat(localAppData, &fileinfo);
		// resize the buffer
		buffer.resize(fileinfo.st_size);
		// read and resize the buffer
		size_t chars_read = fread(&(buffer.front()), sizeof(char), fileinfo.st_size, file);
		buffer.resize(chars_read);
		buffer.shrink_to_fit();
		fclose(file);

		// parse and load the settings file
		json settings = json::parse(buffer);
		ik_settings = new IK_SETTINGS(settings);
	}
	else
	{
		// file does not exist, create it, and save/load default settings
		file = _wfopen(localAppData, L"w");

		// put default settings in settings file
		ik_settings = new IK_SETTINGS();
		fputs(ik_settings->toJson().dump(4).c_str(), file);
		fclose(file);
	}
	initialized = true;
}  