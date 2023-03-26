#pragma once
#include <string>
#include <filesystem>


std::string translateJPNameToEN(std::string, std::string);
std::string getEnglishNameFromCollectorNo(std::string, std::string);
std::string translateModifiers(std::string);
std::string getFoilStatus(std::string);
std::string convertUTF8ToANSI(std::filesystem::path, std::string);

void extractAkiAki(std::filesystem::path);
void extractHareruya(std::filesystem::path);
void extractSingleStar(std::filesystem::path);
void compileAkiAki(std::filesystem::path, float);
void compileHareruya(std::filesystem::path, float);
void compileSingleStar(std::filesystem::path, float);

void extractAndCompile(std::string, std::string, std::string, std::string);