#pragma once
#include <vector>
#include <iostream>
#include <string>
#include <windows.h>
#include <map>

class Set {
private:
	std::string setCode;
	std::string setName;
	std::string pricesUSD;
	std::string data_html;
	std::string data_csv;
	std::string codestring;
	std::map<std::string, std::string> codes;
public:
	Set(std::string, std::string, std::string, std::string);
	std::string getSetCode();
	std::string getSetName();
	std::string getCodes();
	std::string getHTML();
	std::string getCSV();
	void addHTML(std::string);
	void addCSV(std::string);
};

class Store {
private:
	std::string name;
	std::map<std::string, std::string> storecodes;
	std::string urlconstructor;
public:
	Store(std::string, std::string);
	std::string getStoreName();
	void addCodes(std::string, std::string);
	std::map<std::string, std::string> getURLTags();
	std::string getURLConstructor();
	std::map<std::string, std::vector<std::string>> separateURLTags();
};

class AllRecords {
private:
	std::vector<Set> allSets;
	std::vector<Store> allStores;
public:
	void addSet(std::string, std::string, std::string, std::string);
	void addStore(std::string, std::string);
	std::vector<Store> getAllStores();
	std::vector<Set> getAllSets();
	void addCodesByStore(int, std::string, std::string);
};