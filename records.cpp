#pragma once
#include "records.h"
#include <fstream>
#include <regex>
#include <map>

Set::Set(std::string inputCode, std::string inputName, std::string pricescsv, std::string storecodes) {
	setCode = inputCode;
	setName = inputName;
	data_csv = pricescsv;
	codestring = storecodes;
}
std::string Set::getSetCode() {
	return setCode;
}
std::string Set::getSetName() {
	return setName;
}
void Set::addHTML(std::string html) {
	data_html = html;
}
void Set::addCSV(std::string csv) {
	data_csv = csv;
}
std::string Set::getHTML() {
	return data_html;
}
std::string Set::getCSV() {
	return data_csv;
}
std::string Set::getCodes() {
	return codestring;
}

Store::Store(std::string storename, std::string constructor) {
	name = storename;
	urlconstructor = constructor;
}
std::string Store::getStoreName() {
	return name;
}
std::string Store::getURLConstructor() {
	return urlconstructor;
}
void Store::addCodes(std::string setcode, std::string urlcodes) {
	storecodes.insert({setcode, urlcodes});
}
std::map<std::string, std::string> Store::getURLTags() {
	return storecodes;
}
std::map<std::string, std::vector<std::string>> Store::separateURLTags() {
	std::map<std::string, std::vector<std::string>> outputmap;
	for (auto const& x : getURLTags()) {
		std::vector<std::string> codemap;
		std::string f = x.first;
		std::string s = x.second;
		std::string delimiter = " ";

		size_t pos = 0;
		int count = 0;
		std::string token;
		while ((pos = s.find(delimiter)) != std::string::npos) {
			token = s.substr(0, pos);
			codemap.push_back(token);
			s.erase(0, pos + delimiter.length());
			++count;
		}
		outputmap.insert({ f,codemap });
	}
	return outputmap;
}

void AllRecords::addStore(std::string storeName, std::string constructor) {
	allStores.push_back(Store(storeName, constructor));
}
void AllRecords::addSet(std::string setcode, std::string setname, std::string setpriceshtml, std::string storecodes) {
	allSets.push_back(Set(setcode, setname, setpriceshtml, storecodes));
}
std::vector<Store> AllRecords::getAllStores() {
	return allStores;
}
std::vector<Set> AllRecords::getAllSets() {
	return allSets;
}
void AllRecords::addCodesByStore(int storeindex, std::string setcode, std::string urlcodes) {
	allStores[storeindex].addCodes(setcode, urlcodes);
}
void AllRecords::populateStores() {
	addStore("Aki Aki", "https://www.aki-aki.net/product-list/<<urltag>>?page=<<pagenum>>");
	addStore("Hareruya", "https://www.hareruyamtg.com/ja/products/search?cardset=<<urltag>>&rarity%5B0%5D=1&rarity%5B1%5D=2&rarity%5B2%5D=3&rarity%5B3%5D=4&foilFlg%5B0%5D=0&foilFlg%5B1%5D=1&stock=1&page=<<pagenum>>");
	addStore("Single Star", "https://www.singlestar.jp/<<urltag>>/0/normal?page=<<pagenum>>");
}
void AllRecords::setConversion(float rate) {
	conversionRate = rate;
}
float AllRecords::getConversion() {
	return conversionRate;
}