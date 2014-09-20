/*Copyright (c) 2014 The Paradox Game Converters Project

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/



#include <fstream>
#include <stdexcept>
#include <sys/stat.h>
#include <stdio.h>

#include <boost/filesystem.hpp>

#include "Configuration.h"
#include "Log.h"
#include "Parsers/Parser.h"
#include "EU4World/EU4World.h"
#include "EU4World/EU4Religion.h"
#include "EU4World/EU4Localisation.h"
#include "V2World/V2World.h"
#include "V2World/V2Factory.h"
#include "V2World/V2TechSchools.h"
#include "V2World/V2LeaderTraits.h"
#include "WinUtils.h"

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#ifndef WIN32 
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),(mode)))==NULL
#endif

bool CopyDir(boost::filesystem::path const & source, boost::filesystem::path const & destination);

// Converts the given EU4 save into a V2 mod.
// Returns 0 on success or a non-zero failure code on error.
int ConvertEU4ToV2(const std::string& EU4SaveFileName)
{
	Object*	obj;					// generic object

	Configuration::getInstance();

	std::string curDir = boost::filesystem::current_path().generic_string();
	LOG(LogLevel::Debug) << "Current directory is " << curDir;

	// Get V2 install location
	LOG(LogLevel::Info) << "Get V2 Install Path";
	string V2Loc = Configuration::getV2Path();	// the V2 install location as stated in the configuration file
	if (!boost::filesystem::is_directory(V2Loc))
	{
		LOG(LogLevel::Error) << "No Victoria 2 path was specified in configuration.txt, or the path was invalid";
		return (-1);
	}
	else
	{
		LOG(LogLevel::Debug) << "Victoria 2 install path is " << V2Loc;
	}

	// Get V2 Documents Directory
	LOG(LogLevel::Debug) << "Get V2 Documents directory";
	string V2DocLoc = Configuration::getV2DocumentsPath();	// the V2 My Documents location as stated in the configuration file
	if (!boost::filesystem::is_directory(V2DocLoc))
	{
		LOG(LogLevel::Error) << "No Victoria 2 documents directory was specified in configuration.txt, or the path was invalid";
		return (-1);
	}
	else
	{
		LOG(LogLevel::Debug) << "Victoria 2 documents directory is " << V2DocLoc;
	}

	// Get EU4 install location
	LOG(LogLevel::Debug) << "Get EU4 Install Path";
	string EU4Loc = Configuration::getEU4Path();	// the EU4 install location as stated in the configuration file
	if (!boost::filesystem::is_directory(EU4Loc))
	{
		LOG(LogLevel::Error) << "No Europa Universalis 4 path was specified in configuration.txt, or the path was invalid";
		return (-1);
	}
	else
	{
		LOG(LogLevel::Debug) << "EU4 install path is " << EU4Loc;
	}

	// Get EU4 Mod directory
	map<string, string> possibleMods; // name, path
	LOG(LogLevel::Debug) << "Get EU4 Mod Directory";
	string EU4DocumentsLoc = Configuration::getEU4DocumentsPath();	// the EU4 My Documents location as stated in the configuration file
	if (!boost::filesystem::is_directory(EU4DocumentsLoc))
	{
		LOG(LogLevel::Error) << "No Europa Universalis 4 documents directory was specified in configuration.txt, or the path was invalid";
		return (-1);
	}
	else
	{
		LOG(LogLevel::Debug) << "EU4 Documents directory is " << EU4DocumentsLoc;
		set<string> fileNames;
		WinUtils::GetAllFilesInFolder(EU4DocumentsLoc + "/mod", fileNames);
		for (set<string>::iterator itr = fileNames.begin(); itr != fileNames.end(); itr++)
		{
			const int pos = itr->find_last_of('.');	// the position of the last period in the filename
			if (itr->substr(pos, itr->length()) == ".mod")
			{
				Object* modObj = doParseFile((EU4DocumentsLoc + "\\mod\\" + *itr).c_str());	// the parsed mod file
				string name = modObj->getLeaf("name");														// the name of the mod

				string path;	// the path of the mod
				vector<Object*> dirObjs = modObj->getValue("path");	// the possible paths of the mod
				if (dirObjs.size() > 0)
				{
					path = dirObjs[0]->getLeaf();
				}
				else
				{
					vector<Object*> dirObjs = modObj->getValue("archive");	// the other possible paths of the mod (if its zipped)
					if (dirObjs.size() > 0)
					{
						path = dirObjs[0]->getLeaf();
					}
				}

				if (path != "")
				{
					possibleMods.insert(make_pair(name, EU4DocumentsLoc + "\\" + path));
					Log(LogLevel::Debug) << "\t\tFound a mod named " << name << " claiming to be at " << EU4DocumentsLoc << "\\" << path;
				}
			}
		}
	}

	// Get CK2 Export directory
	LOG(LogLevel::Debug) << "Get CK2 Export Directory";
	string CK2ExportLoc = Configuration::getCK2ExportPath();		// the CK2 converted mods location as stated in the configuration file
	if (!boost::filesystem::is_directory(CK2ExportLoc))
	{
		LOG(LogLevel::Warning) << "No Crusader Kings 2 mod directory was specified in configuration.txt, or the path was invalid - this will cause problems with CK2 converted saves";
	}
	else
	{
		LOG(LogLevel::Debug) << "CK2 export directory is " << CK2ExportLoc;
		set<string> fileNames;
		WinUtils::GetAllFilesInFolder(CK2ExportLoc, fileNames);
		for (set<string>::iterator itr = fileNames.begin(); itr != fileNames.end(); itr++)
		{
			const int pos = itr->find_last_of('.');	// the last period in the filename
			if (itr->substr(pos, itr->length()) == ".mod")
			{
				Object* modObj = doParseFile((CK2ExportLoc + "\\" + *itr).c_str());	// the parsed mod file
				string name = modObj->getLeaf("name");											// the name ofthe mod

				string path;	// the path of the mod
				vector<Object*> dirObjs = modObj->getValue("user_dir");	// the possible paths for the mod
				if (dirObjs.size() > 0)
				{
					path = dirObjs[0]->getLeaf();
				}
				else
				{
					vector<Object*> dirObjs = modObj->getValue("archive");	// the other possible paths for the mod (if it's zipped)
					if (dirObjs.size() > 0)
					{
						path = dirObjs[0]->getLeaf();
					}
				}

				if (path != "")
				{
					possibleMods.insert(make_pair(name, CK2ExportLoc + "\\" + path));
				}
			}
		}
	}

	//get output name
	boost::filesystem::path outputPath(EU4SaveFileName);
	string outputName = outputPath.filename().generic_string();

	int dash = outputName.find_first_of('-');									// the first (if any) dask in the output name
	while (dash != string::npos)
	{
		outputName.replace(dash, 1, "_");
		dash = outputName.find_first_of('-');
	}
	int space = outputName.find_first_of(' ');	// the first space (if any) in the output name
	while (space != string::npos)
	{
		outputName.replace(space, 1, "_");
		space = outputName.find_first_of(' ');
	}
	Configuration::setOutputName(outputName);
	LOG(LogLevel::Info) << "Using output name " << outputName;

	LOG(LogLevel::Info) << "* Importing EU4 save *";

	//	Parse EU4 Save
	LOG(LogLevel::Info) << "Parsing save";
	obj = doParseFile(EU4SaveFileName.c_str());
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse file " << EU4SaveFileName;
		exit(-1);
	}

	// Get EU4 Mod
	LOG(LogLevel::Debug) << "Get EU4 Mod";
	vector<string> fullModPaths;	// the full pathnames for used mods
	vector<Object*> modObj = obj->getValue("mod_enabled");	// the used mods
	if (modObj.size() > 0)
	{
		string modString = modObj[0]->getLeaf();	// the names of all the mods
		while (modString != "")
		{
			string newMod;	// the corrected name of the mod
			const int firstQuote = modString.find("\"");	// the location of the first quote, defining the start of a mod name
			if (firstQuote == std::string::npos)
			{
				newMod.clear();
				modString.clear();
			}
			else
			{
				const int secondQuote = modString.find("\"", firstQuote + 1);	// the location of the second quote, defining the end of a mod name
				if (secondQuote == std::string::npos)
				{
					newMod.clear();
					modString.clear();
				}
				else
				{
					newMod = modString.substr(firstQuote + 1, secondQuote - firstQuote - 1);
					modString = modString.substr(secondQuote + 1, modString.size());
				}
			}

			if (newMod != "")
			{
				map<string, string>::iterator modItr = possibleMods.find(newMod);
				if (modItr != possibleMods.end())
				{
					string newModPath = modItr->second;	// the path for this mod
					if (boost::filesystem::exists(newModPath))
					{
						LOG(LogLevel::Error) << newMod << " could not be found in the specified mod directory - a valid mod directory must be specified. Tried " << newModPath;
					}
					else
					{
						LOG(LogLevel::Debug) << "EU4 Mod is at " << newModPath;
						fullModPaths.push_back(newModPath);
					}
				}
				else
				{
					LOG(LogLevel::Error) << "No path could be found for " << newMod;
				}
			}
		}
	}

	// Read all localisations.
	LOG(LogLevel::Info) << "Reading localisation";
	EU4Localisation localisation;
	localisation.ReadFromAllFilesInFolder( boost::filesystem::path(Configuration::getEU4Path() + "\\localisation").generic_string() );
	for (vector<string>::iterator itr = fullModPaths.begin(); itr != fullModPaths.end(); itr++)
	{
		LOG(LogLevel::Debug) << "Reading mod localisation";
		boost::filesystem::path ItrPath(*itr + "\\localisation");
		localisation.ReadFromAllFilesInFolder(ItrPath.generic_string());
	}

	// Read Idea Effects
	LOG(LogLevel::Info) << "Getting idea effects";
	Object* ideaObj = doParseFile("idea_effects.txt");
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse file idea_effects.txt";
		exit(-1);
	}
	map<string, int>					armyInvIdeas;
	map<string, int>					commerceInvIdeas;
	map<string, int>					cultureInvIdeas;
	map<string, int>					industryInvIdeas;
	map<string, int>					navyInvIdeas;
	map<string, double>				UHLiberalIdeas;
	map<string, double>				UHReactionaryIdeas;
	vector< pair<string, int> >	literacyIdeas;
	map<string, int>					orderIdeas;
	map<string, int>					libertyIdeas;
	map<string, int>					equalityIdeas;
	initIdeaEffects(ideaObj, armyInvIdeas, commerceInvIdeas, cultureInvIdeas, industryInvIdeas, navyInvIdeas, UHLiberalIdeas, UHReactionaryIdeas, literacyIdeas, orderIdeas, libertyIdeas, equalityIdeas);

	// Construct world from EU4 save.
	LOG(LogLevel::Info) << "Building world";
	EU4World sourceWorld(obj, armyInvIdeas, commerceInvIdeas, cultureInvIdeas, industryInvIdeas, navyInvIdeas);

	// Read EU4 common\countries
	LOG(LogLevel::Info) << "Reading EU4 common\\countries";
	
	boost::filesystem::path CommonCountriesPath(Configuration::getEU4Path());
	CommonCountriesPath /= "common";
	CommonCountriesPath /= "country_tags";
	CommonCountriesPath /= "00_countries.txt";
	ifstream commonCountries(CommonCountriesPath.c_str());
	sourceWorld.readCommonCountries(commonCountries, Configuration::getEU4Path());
	for (vector<string>::iterator itr = fullModPaths.begin(); itr != fullModPaths.end(); itr++)
	{
		set<string> fileNames;
		WinUtils::GetAllFilesInFolder(*itr + "\\common\\country_tags\\", fileNames);
		boost::filesystem::path convertedCommonCountriesPath(Configuration::getEU4Path());
		convertedCommonCountriesPath /= "common";
		convertedCommonCountriesPath /= "country_tags";
		convertedCommonCountriesPath /= "converted_countries.txt";
		ifstream convertedCommonCountries(convertedCommonCountriesPath.c_str());
			sourceWorld.readCommonCountries(convertedCommonCountries, *itr);
		boost::filesystem::path specialCommonCountriesPath(Configuration::getEU4Path());
		specialCommonCountriesPath /= "common";
		specialCommonCountriesPath /= "country_tags";
		specialCommonCountriesPath /= "01_special_tags.txt";
		ifstream specialCommonCountries(specialCommonCountriesPath.c_str());
	}
	
	sourceWorld.setLocalisations(localisation);

	// Resolve unit types
	/*log("Resolving unit types.\n");
	printf("Resolving unit types.\n");
	RegimentTypeMap rtm;
	read.open("unit_strength.txt");
	if (read.is_open())
	{
		read.close();
		read.clear();
		log("\tReading unit strengths from unit_strength.txt\n");
		obj = doParseFile("unit_strength.txt");
		if (obj == NULL)
		{
			log("Could not parse file unit_strength.txt\n");
			exit(-1);
		}
		for (int i = 0; i < num_reg_categories; ++i)
		{
			AddCategoryToRegimentTypeMap(obj, (RegimentCategory)i, RegimentCategoryNames[i], rtm);
		}
	}
	else
	{
		log("	Reading unit strengths from EU4 installation folder\n");
		struct _finddata_t unitFileData;
		intptr_t fileListing;
		if ( (fileListing = _findfirst( boost::filesystem::path(EU4Loc + "\\common\\units\\*.txt").generic_string().c_str(), &unitFileData)) == -1L)
		{
			log("	Could not open units directory.\n");
			return -1;
		}
		do
		{
			if (strcmp(unitFileData.name, ".") == 0 || strcmp(unitFileData.name, "..") == 0 )
			{
				continue;
			}
			string unitFilename = unitFileData.name;
			string unitName = unitFilename.substr(0, unitFilename.find_first_of('.'));
			AddUnitFileToRegimentTypeMap(boost::filesystem::path(EU4Loc + "\\common\\units").generic_string().c_str(), unitName, rtm);
		} while(_findnext(fileListing, &unitFileData) == 0);
		_findclose(fileListing);
	}
	read.close();
	read.clear();
	sourceWorld.resolveRegimentTypes(rtm);*/


	// Merge nations
	LOG(LogLevel::Info) << "Merging nations";
	obj = doParseFile("merge_nations.txt");
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse file merge_nations.txt";
		exit(-1);
	}
	mergeNations(sourceWorld, obj);

	// Parse V2 input file
	LOG(LogLevel::Info) << "Parsing Vicky2 data";
	V2World destWorld;


	//// Construct factory factory
	LOG(LogLevel::Info) << "Determining factory allocation rules.";
	V2FactoryFactory factoryBuilder;

	// Parse province mappings
	LOG(LogLevel::Info) << "Parsing province mappings";
	obj = doParseFile("province_mappings.txt");
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse file province_mappings.txt";
		exit(-1);
	}
	provinceMapping			provinceMap;
	inverseProvinceMapping	inverseProvinceMap;
	resettableMap				resettableProvinces;
	initProvinceMap(obj, sourceWorld.getVersion(), provinceMap, inverseProvinceMap, resettableProvinces);
	sourceWorld.checkAllProvincesMapped(inverseProvinceMap);

	// Get country mappings
	removeEmptyNations(sourceWorld);
	if (Configuration::getRemovetype() == "dead")
	{
		removeDeadLandlessNations(sourceWorld);
	}
	else if (Configuration::getRemovetype() == "all")
	{
		removeLandlessNations(sourceWorld);
	}
	CountryMapping countryMap;
	countryMap.ReadRules("country_mappings.txt");

	// Get adjacencies
	LOG(LogLevel::Info) << "Importing adjacencies";
	adjacencyMapping adjacencyMap = initAdjacencyMap();

	// Generate continent mapping. Only the one from the last listed mod will be used
	continentMapping continentMap;
	for (vector<string>::iterator itr = fullModPaths.begin(); itr != fullModPaths.end(); itr++)
	{
		boost::filesystem::path continentFile = *itr + "\\map\\continent.txt";
		if (!boost::filesystem::exists(continentFile))
		{
			obj = doParseFile(continentFile.generic_string().c_str());
			if ((obj != NULL) && (obj->getLeaves().size() > 0))
			{
				initContinentMap(obj, continentMap);
			}
		}
	}
	if (continentMap.size() == 0)
	{
		obj = doParseFile(boost::filesystem::path(EU4Loc + "\\map\\continent.txt").generic_string().c_str());
		if (obj == NULL)
		{
			LOG(LogLevel::Error) << "Could not parse file " << EU4Loc << "\\map\\continent.txt";
			exit(-1);
		}
		if (obj->getLeaves().size() < 1)
		{
			LOG(LogLevel::Error) << "Failed to parse continent.txt";
			return 1;
		}
		initContinentMap(obj, continentMap);
	}
	if (continentMap.size() == 0)
	{
		LOG(LogLevel::Warning) << "No continent mappings found - may lead to problems later";
	}
	
	// Generate region mapping
	LOG(LogLevel::Info) << "Parsing region structure";
	if (!boost::filesystem::exists(".\\blankMod\\output\\map\\region.txt"))
	{
		obj = doParseFile(boost::filesystem::path(".\\blankMod\\output\\map\\region.txt").generic_string().c_str());
		if (obj == NULL)
		{
			LOG(LogLevel::Error) << "Could not parse file .\\blankMod\\output\\map\\region.txt";
			exit(-1);
		}
	}
	else
	{
		obj = doParseFile( boost::filesystem::path(V2Loc + "\\map\\region.txt").generic_string().c_str() );
		if (obj == NULL)
		{
			LOG(LogLevel::Error) << "Could not parse file " << V2Loc << "\\map\\region.txt";
			exit(-1);
		}
	}
	if (obj->getLeaves().size() < 1)
	{
		LOG(LogLevel::Error) << "Could not parse region.txt";
		return 1;
	}
	stateMapping		stateMap;
	stateIndexMapping	stateIndexMap;
	initStateMap(obj, stateMap, stateIndexMap);
	countryMap.readV2Regions(obj);


	// Parse Culture Mappings
	LOG(LogLevel::Info) << "Parsing culture mappings";
	obj = doParseFile("cultureMap.txt");
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse file cultureMap.txt";
		exit(-1);
	}
	if (obj->getLeaves().size() < 1)
	{
		LOG(LogLevel::Error) << "Failed to parse cultureMap.txt";
		return 1;
	}
	cultureMapping cultureMap;
	cultureMap = initCultureMap(obj->getLeaves()[0]);

	// find culture groups
	unionCulturesMap			unionCultures;
	inverseUnionCulturesMap	inverseUnionCultures;
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse file " << EU4Loc << "\\common\\cultures\\00_cultures.txt";
		exit(-1);
	}
	if (obj->getLeaves().size() < 1)
	{
		LOG(LogLevel::Error) << "Failed to parse 00_cultures.txt";
		return 1;
	}
	initUnionCultures(obj, unionCultures, inverseUnionCultures);
	for (vector<string>::iterator itr = fullModPaths.begin(); itr != fullModPaths.end(); itr++)
	{
		boost::filesystem::path directory(*itr + "\\common\\cultures");
		if (boost::filesystem::exists(directory) && boost::filesystem::is_directory(directory))
		{
			for (boost::filesystem::directory_iterator directoryItr(directory);
				directoryItr != boost::filesystem::directory_iterator();
				++directoryItr)
			{
				if (boost::filesystem::is_regular_file(*directoryItr))
				{
					obj = doParseFile((*directoryItr).path().generic_string().c_str());
					if (obj == NULL)
					{
						LOG(LogLevel::Error) << "Could not parse file " << (*directoryItr).path().generic_string();
						exit(-1);
					}
					if (obj->getLeaves().size() < 1)
					{
						LOG(LogLevel::Error) << "Failed to parse cultures file";
						return 1;
					}
					initUnionCultures(obj, unionCultures, inverseUnionCultures);
				}
			}
		}
	}

		/*
		if ((fileListing = _findfirst(boost::filesystem::path(*itr + "\\common\\cultures\\*").generic_string().c_str(), &fileData)) != -1L)
		{
			do
			{
				if (strcmp(fileData.name, ".") == 0 || strcmp(fileData.name, "..") == 0)
				{
					continue;
				}
				else if (fileData.attrib & _A_SUBDIR)
				{
					continue;
				}
				else
				{
					boost::filesystem::path modCultureFile(*itr + "\\common\\cultures\\" + fileData.name);
					obj = doParseFile(modCultureFile.generic_string().c_str());
					if (obj == NULL)
					{
						LOG(LogLevel::Error) << "Could not parse file " << modCultureFile;
						exit(-1);
					}
					initUnionCultures(obj, unionCultures, inverseUnionCultures);
				}
			} while (_findnext(fileListing, &fileData) == 0);
			_findclose(fileListing);
		}
	}*/

	// Parse EU4 Religions
	LOG(LogLevel::Info) << "Parsing EU4 religions";
	obj = doParseFile(boost::filesystem::path(EU4Loc + "\\common\\religions\\00_religion.txt").generic_string().c_str());
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse file " << EU4Loc << "\\common\\religions\\00_religion.txt";
		exit(-1);
	}
	if (obj->getLeaves().size() < 1)
	{
		LOG(LogLevel::Error) << "Failed to parse 00_religion.txt";
		return 1;
	}
	EU4Religion::parseReligions(obj);
	for (vector<string>::iterator itr = fullModPaths.begin(); itr != fullModPaths.end(); itr++)
	{
		boost::filesystem::path directory(*itr + "\\common\\religions");
		if (boost::filesystem::exists(directory) && boost::filesystem::is_directory(directory))
		{
			for (boost::filesystem::directory_iterator directoryItr(directory);
				directoryItr != boost::filesystem::directory_iterator();
				++directoryItr)
			{
				if (boost::filesystem::is_regular_file(*directoryItr))
				{
					obj = doParseFile((*directoryItr).path().generic_string().c_str());
					if (obj == NULL)
					{
						LOG(LogLevel::Error) << "Could not parse file " << (*directoryItr).path().generic_string();
						exit(-1);
					}
					if (obj->getLeaves().size() < 1)
					{
						LOG(LogLevel::Error) << "Failed to parse cultures file";
						return 1;
					}
					EU4Religion::parseReligions(obj);
				}
			}
		}

		/*
		if ((fileListing = _findfirst(boost::filesystem::path(*itr + "\\common\\religions\\*").generic_string().c_str(), &fileData)) != -1L)
		{
			do
			{
				if (strcmp(fileData.name, ".") == 0 || strcmp(fileData.name, "..") == 0)
				{
					continue;
				}
				else if (fileData.attrib & _A_SUBDIR)
				{
					continue;
				}
				else
				{
					boost::filesystem::path modReligionFile(*itr + "\\common\\religions\\" + fileData.name);
					if ((_stat(modReligionFile.generic_string().c_str(), &st) == 0))
					{
						obj = doParseFile(modReligionFile.generic_string().c_str());
						if (obj == NULL)
						{
							LOG(LogLevel::Error) << "Could not parse file " << modReligionFile;
							exit(-1);
						}
						EU4Religion::parseReligions(obj);
					}
				}
			} while (_findnext(fileListing, &fileData) == 0);
			_findclose(fileListing);
		}*/
	}

	// Parse Religion Mappings
	LOG(LogLevel::Info) << "Parsing religion mappings";
	obj = doParseFile("religionMap.txt");
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse file religionMap.txt";
		exit(-1);
	}
	if (obj->getLeaves().size() < 1)
	{
		LOG(LogLevel::Error) << "Failed to parse religionMap.txt";
		return 1;
	}
	religionMapping religionMap;
	religionMap = initReligionMap(obj->getLeaves()[0]);


	// Parse unions mapping
	LOG(LogLevel::Info) << "Parsing union mappings";
	obj = doParseFile("unions.txt");
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse file unions.txt";
		exit(-1);
	}
	if (obj->getLeaves().size() < 1)
	{
		LOG(LogLevel::Error) << "Failed to parse unions.txt";
		return 1;
	}
	unionMapping unionMap;
	unionMap = initUnionMap(obj->getLeaves()[0]);


	// Parse government mapping
	LOG(LogLevel::Info) << "Parsing governments mappings";
	initParser();
	obj = doParseFile("governmentMapping.txt");
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse file governmentMapping.txt";
		exit(-1);
	}
	governmentMapping governmentMap;
	governmentMap = initGovernmentMap(obj->getLeaves()[0]);


	// Parse tech schools
	LOG(LogLevel::Info) << "Parsing tech schools.";
	initParser();
	obj = doParseFile("blocked_tech_schools.txt");
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse file blocked_tech_schools.txt";
		exit(-1);
	}
	vector<string> blockedTechSchools;	// the list of disallowed tech schools
	blockedTechSchools = initBlockedTechSchools(obj);
	initParser();
	obj = doParseFile( (V2Loc + "\\common\\technology.txt").c_str() );
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse file " << V2Loc << "\\common\\technology.txt";
		exit(-1);
	}
	vector<techSchool> techSchools;
	techSchools = initTechSchools(obj, blockedTechSchools);


	// Get Leader traits
	LOG(LogLevel::Info) << "Getting leader traits";
	const V2LeaderTraits lt;	// the V2 leader traits
	map<int, int> leaderIDMap; // <EU4, V2>


	// Parse colony rules
	LOG(LogLevel::Info) << "Parsing colony naming rules.";
	initParser();
	obj = doParseFile("colonial.txt");
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse colonial.txt";
		exit(-1);
	}
	colonyMapping colonyMap = initColonyMap(obj);

	// Get EU4 colonial regions
	LOG(LogLevel::Info) << "Parsing EU4 colonial regions";
	obj = doParseFile((EU4Loc + "\\common\\colonial_regions\\00_colonial_regions.txt").c_str());
	if (obj == NULL)
	{
		LOG(LogLevel::Error) << "Could not parse file " << EU4Loc << "\\common\\colonial_regions\\00_colonial_regions.txt";
		exit(-1);
	}
	if (obj->getLeaves().size() < 1)
	{
		LOG(LogLevel::Error) << "Failed to parse 00_colonial_regions.txt";
		return 1;
	}
	countryMap.readEU4Regions(obj);
	for (vector<string>::iterator itr = fullModPaths.begin(); itr != fullModPaths.end(); itr++)
	{
		for (boost::filesystem::directory_iterator directoryItr(*itr + "\\common\\colonial_regions\\");
			directoryItr != boost::filesystem::directory_iterator();
			++directoryItr)
		{
			if (false == boost::filesystem::is_regular_file(*directoryItr))
			{
				continue;
			}

			string modRegionsFile(directoryItr->path().string());	// the path and name of the colonial regions file in this mod
			obj = doParseFile(modRegionsFile.c_str());
			if (obj == NULL)
			{
				LOG(LogLevel::Error) << "Could not parse file " << modRegionsFile;
				exit(-1);
			}
			countryMap.readEU4Regions(obj);
		}
	}

	// Create country mapping
	countryMap.CreateMapping(sourceWorld, destWorld, colonyMap, inverseProvinceMap, provinceMap, inverseUnionCultures);


	// Convert
	LOG(LogLevel::Info) << "Converting countries";
	destWorld.convertCountries(sourceWorld, countryMap, cultureMap, unionCultures, religionMap, governmentMap, inverseProvinceMap, techSchools, leaderIDMap, lt, UHLiberalIdeas, UHReactionaryIdeas, literacyIdeas, orderIdeas, libertyIdeas, equalityIdeas);
	LOG(LogLevel::Info) << "Converting provinces";
	destWorld.convertProvinces(sourceWorld, provinceMap, resettableProvinces, countryMap, cultureMap, religionMap, stateIndexMap);
	LOG(LogLevel::Info) << "Converting diplomacy";
	destWorld.convertDiplomacy(sourceWorld, countryMap);
	LOG(LogLevel::Info) << "Setting colonies";
	destWorld.setupColonies(adjacencyMap, continentMap);
	LOG(LogLevel::Info) << "Creating states";
	destWorld.setupStates(stateMap);
	LOG(LogLevel::Info) << "Setting unciv reforms";
	destWorld.convertUncivReforms();
	/*LOG(LogLevel::Info) << "Creating pops";
	destWorld.setupPops(sourceWorld);*/
	LOG(LogLevel::Info) << "Adding unions";
	destWorld.addUnions(unionMap);
	/*LOG(LogLevel::Info) << "Converting armies and navies";
	destWorld.convertArmies(sourceWorld, inverseProvinceMap, leaderIDMap);*/
	LOG(LogLevel::Info) << "Converting techs";
	destWorld.convertTechs(sourceWorld);
	LOG(LogLevel::Info) << "Allocating starting factories";
	destWorld.allocateFactories(sourceWorld, factoryBuilder);

	// Output results
	LOG(LogLevel::Info) << "Outputting mod";
	CopyDir("./blankMod", "./output");
	FILE* modFile;	// the .mod file for this mod
	if (fopen_s(&modFile, boost::filesystem::path("Output\\" + Configuration::getOutputName() + ".mod").generic_string().c_str(), "w") != 0)
	{
		LOG(LogLevel::Error) << "Could not create .mod file";
		exit(-1);
	}
	fprintf(modFile, "name = \"Converted - %s\"\n", Configuration::getOutputName().c_str());
	fprintf(modFile, "path = \"mod/%s\"\n", Configuration::getOutputName().c_str());
	fprintf(modFile, "replace = \"history/provinces\"\n");
	fprintf(modFile, "replace = \"history/countries\"\n");
	fprintf(modFile, "replace = \"history/diplomacy\"\n");
	fprintf(modFile, "replace = \"history/units\"\n");
	fprintf(modFile, "replace = \"common/religion.txt\"\n");
	fprintf(modFile, "replace = \"common/cultures.txt\"\n");
	fprintf(modFile, "replace = \"gfx/interface/icon_religion.dds\"\n");
	fclose(modFile);
	boost::filesystem::rename("output\\output", "output\\" + Configuration::getOutputName());
	destWorld.output();

	LOG(LogLevel::Info) << "* Conversion complete *";
	return 0;
}


int main(const int argc, const char * argv[])
{
	try
	{
		const char* const defaultEU4SaveFileName = "input.eu4";	// the default name for a save to convert
		string EU4SaveFileName;												// the actual name for the save to convert
		if (argc >= 2)
		{
			EU4SaveFileName = argv[1];
			LOG(LogLevel::Info) << "Using input file " << EU4SaveFileName;
		}
		else
		{
			EU4SaveFileName = defaultEU4SaveFileName;
			LOG(LogLevel::Info) << "No input file given, defaulting to " << defaultEU4SaveFileName;
		}
		return ConvertEU4ToV2(EU4SaveFileName);
	}
	catch (const std::exception& e)	// catch any errors
	{
		LOG(LogLevel::Error) << e.what();
		return -1;
	}
}

bool CopyDir(
	boost::filesystem::path const & source,
	boost::filesystem::path const & destination
	)
{
	namespace fs = boost::filesystem;
	try
	{
		// Check whether the function call is valid
		if (
			!fs::exists(source) ||
			!fs::is_directory(source)
			)
		{
			std::cerr << "Source directory " << source.string()
				<< " does not exist or is not a directory." << '\n'
				;
			return false;
		}
		if (fs::exists(destination))
		{
			std::cerr << "Destination directory " << destination.string()
				<< " already exists." << '\n'
				;
			return false;
		}
		// Create the destination directory
		if (!fs::create_directory(destination))
		{
			std::cerr << "Unable to create destination directory"
				<< destination.string() << '\n'
				;
			return false;
		}
	}
	catch (fs::filesystem_error const & e)
	{
		std::cerr << e.what() << '\n';
		return false;
	}
	// Iterate through the source directory
	for (
		fs::directory_iterator file(source);
		file != fs::directory_iterator(); ++file
		)
	{
		try
		{
			fs::path current(file->path());
			if (fs::is_directory(current))
			{
				// Found directory: Recursion
				if (
					!CopyDir(
					current,
					destination / current.filename()
					)
					)
				{
					return false;
				}
			}
			else
			{
				// Found file: Copy
				fs::copy_file(
					current,
					destination / current.filename()
					);
			}
		}
		catch (fs::filesystem_error const & e)
		{
			std::cerr << e.what() << '\n';
		}
	}
	return true;
}
