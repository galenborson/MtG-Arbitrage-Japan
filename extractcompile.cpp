#pragma once
#include "extractcompile.h"
#include <iostream>
#include <fstream>
#include <regex>
#include <Windows.h>

// Converts raw html files from UTF8 into ANSI.
// Necessary for the .csv conversion step.
std::string convertUTF8ToANSI(std::filesystem::path inputfile, std::string outputfile) {
    // Open the input file in binary mode
    std::string filestring = inputfile.string();
    // std::cout << "Converting " << filestring << " from UTF-8 to ANSI...\n";
    std::ifstream input(filestring, std::ios::binary);

    // Read the entire content of the input file into a string object
    std::string input_str((std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());

    // Convert the input string from UTF-8 encoding to wide character encoding
    int wstr_size = MultiByteToWideChar(CP_UTF8, 0, input_str.c_str(), -1, NULL, 0);
    wchar_t* wstr = new wchar_t[wstr_size];
    MultiByteToWideChar(CP_UTF8, 0, input_str.c_str(), -1, wstr, wstr_size);

    // Convert the wide character string to a new string in ANSI encoding
    int str_size = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* str = new char[str_size];
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, str_size, NULL, NULL);

    // Open the output file in binary mode
    std::ofstream output(outputfile + inputfile.filename().string(), std::ios::binary);

    // Write the new ANSI string to the output file
    output.write(str, str_size);

    // Clean up the allocated memory
    delete[] wstr;
    delete[] str;
    return inputfile.filename().string();
}

// Takes the Japanese name of a card and matches it to its English equivalent.
// This step does not append modifiers.
std::string translateJPNameToEN(std::string nameJP, std::string setcode) {
    std::string nameEN;
    std::ifstream setfile("files/translations/" + setcode + ".tsv");
    std::string currentline;
    bool found = false;
    while (!setfile.eof() && !found) {
        getline(setfile, currentline);
        std::smatch match;
        std::regex_search(currentline, match, std::regex("^.*?\t(.*?)\t(.*)"));
        if (match[1] == nameJP) {
            nameEN = match[2];
            found = true;
        }
    }
    if (!found) {
        std::cout << "Could not find English name for " << nameJP << " in set " << setcode << ". Skipping.\n";
    }
    setfile.close();
    return nameEN;
}

// Takes a card's collector number and converts it to the full English name.
// This step appends modifiers.
std::string getEnglishNameFromCollectorNo(std::string collno, std::string setcode) {
    std::string nameEN;
    std::ifstream setfile("files/translations/" + setcode + ".tsv");
    std::string currentline;
    bool found = false;
    while (!setfile.eof() && !found) {
        getline(setfile, currentline);
        std::smatch match;
        std::regex_search(currentline, match, std::regex("^(.*?)\t.*?\t(.*)"));
        if (match[1] == collno) {
            nameEN = match[2];
            found = true;
        }
    }
    setfile.close();
    return nameEN;
}

// If a card has modifiers ("Borderless," "Showcase," "Extended Art," etc), converts the modifiers to their English equivalent.
std::string translateModifiers(std::string modifiers) {
    std::string modifierReturn;
    std::smatch inputmatch;
    std::ifstream modifierMap("files/translations/modifiers.tsv");
    std::string temp;
    bool found = false;
    while (!modifierMap.eof() && !found) {
        getline(modifierMap, temp);
        std::smatch match;
        std::regex_search(temp, match, std::regex("^(.*?)\t(.*?)$"));
        std::string currentmod = match[1];
        if (modifiers == currentmod) {
            modifierReturn += " (" + std::string(match[2]) + ")";
        }
    }
    modifierMap.close();
    return modifierReturn;
}

// Detects if an entry is foil or not.
std::string getFoilStatus(std::string text) {
    std::smatch match;
    std::string status;
    std::regex_search(text, match, std::regex("^.*?([FOILfoil]{4}|ÅyS&amp;CÅEFÅz)"));
    if (match[1] != "") status = "Foil";
    return status;
}


// Converts the raw html file of an Aki Aki page into a .csv.
void extractAkiAki(std::filesystem::path filedir) {
    std::ifstream inputfile;
    std::string filename = filedir.filename().string();
    std::string convertedfile = convertUTF8ToANSI(filedir, "files/stores_databases/Aki Aki/");
    std::string ansihtmlfile = "files/stores_databases/Aki Aki/" + convertedfile;
    std::filesystem::path outputdir = "files/stores_databases/Aki Aki/" + filename.substr(0, filename.size() - 5) + ".csv";
    std::ofstream cleanedhtml;
    std::string temp;
    inputfile.open(ansihtmlfile);
    cleanedhtml.open(outputdir);
    while (!inputfile.eof()) {
        getline(inputfile, temp);
        std::smatch match;
        // if (std::regex_search(temp, match, std::regex("^.*model_number\">.*?([0-9]{1,5})"))) {
        if (std::regex_search(temp, match, std::regex("^.*?href=\"(.*?)\".*?model_number\">.*?([0-9]{3,5})"))) {
            cleanedhtml << match[1] << "\t" << match[2] << "\t";
            std::string isFoil = getFoilStatus(temp);
            cleanedhtml << isFoil << "\t";
            if (std::regex_search(temp, match, std::regex("^.*?\\(ì˙"))) {
                cleanedhtml << "JP";
            }
            else cleanedhtml << "EN";
            cleanedhtml << "\t";
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            std::regex_search(temp, match, std::regex("^.*?>(.*?)â~"));
            cleanedhtml << match[1] << "\t";
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            std::regex_search(temp, match, std::regex("^.*?êî ([0-9,]*?)ì_"));
            cleanedhtml << match[1] << "\n";
        }
    }
    inputfile.close();
    cleanedhtml.close();
    std::filesystem::remove(ansihtmlfile);
}

// Converts the raw html file of a Hareruya page into a .csv.
void extractHareruya(std::filesystem::path filedir) {
    std::ifstream inputfile;
    std::string filename = filedir.filename().string();
    std::string convertedfile = convertUTF8ToANSI(filedir, "files/stores_databases/Hareruya/");
    std::string ansihtmlfile = "files/stores_databases/Hareruya/" + convertedfile;
    std::filesystem::path outputdir = "files/stores_databases/Hareruya/" + filename.substr(0, filename.size() - 5) + ".csv";
    std::ofstream cleanedhtml;
    std::string temp;

    inputfile.open(ansihtmlfile);
    cleanedhtml.open(outputdir);
    while (!inputfile.eof()) {
        getline(inputfile, temp);
        std::smatch match;
        if (std::regex_search(temp, match, std::regex("^.*?<a href=.*?\/ja\/products\/detail\/(.*)"))) {
            std::string link = "https://www.hareruyamtg.com/ja/products/detail/" + std::string(match[1]);
            cleanedhtml << link << "\t";
            getline(inputfile, temp);
            getline(inputfile, temp);
            std::regex_search(temp, match, std::regex("^.*?(Åy.*?)<"));
            cleanedhtml << match[1] << "\t";
            std::string isFoil = getFoilStatus(temp);
            cleanedhtml << isFoil << "\t";
            if (std::regex_search(temp, match, std::regex("^.*?ÅyJPÅz"))) {
                cleanedhtml << "JP";
            }
            else cleanedhtml << "EN";
            cleanedhtml << "\t";
            getline(inputfile, temp);
            getline(inputfile, temp);
            std::regex_search(temp, match, std::regex("^.*?<p class.*? ([0-9\,]*?)<"));
            cleanedhtml << match[1] << "\t";
            getline(inputfile, temp);
            std::regex_search(temp, match, std::regex("^.*?ç›å…:([0-9]*)"));
            cleanedhtml << match[1] << "\n";
        }
    }
    inputfile.close();
    cleanedhtml.close();
    std::filesystem::remove(ansihtmlfile);
}

// Converts the raw html file of a Single Star page into a .csv.
void extractSingleStar(std::filesystem::path filedir) {
    std::ifstream inputfile;
    std::string filename = filedir.filename().string();
    std::string convertedfile = convertUTF8ToANSI(filedir, "files/stores_databases/Single Star/");
    std::string ansihtmlfile = "files/stores_databases/Single Star/" + convertedfile;
    std::filesystem::path outputdir = "files/stores_databases/Single Star/" + filename.substr(0, filename.size() - 5) + ".csv";
    std::ofstream cleanedhtml;
    std::string temp;

    inputfile.open(ansihtmlfile);
    cleanedhtml.open(outputdir);
    while (!inputfile.eof()) {
        std::smatch match;
        getline(inputfile, temp);
        if (std::regex_search(temp, match, std::regex("^.*?<a href=\"(https://www.singlestar.jp/product/.*?)\""))) {
            std::string link = match[1];
            cleanedhtml << link << "\t";
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            std::regex_search(temp, match, std::regex("^.*\"goods_name\">.*?\/(.*?)Åy(.*?)Åz"));
            cleanedhtml << match[1] << "\t" << match[2] << "\t";
            std::smatch foilmatch;
            if (std::regex_search(temp, foilmatch, std::regex("^.*?\\[FOIL"))) {
                cleanedhtml << "Foil";
            }
            cleanedhtml << "\t";
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            std::regex_search(temp, match, std::regex("^.*?([0-9,]{1,10})"));
            if (match[1] == "") {
                getline(inputfile, temp);
                std::regex_search(temp, match, std::regex("^.*?([0-9,]{1,10})"));
            }
            cleanedhtml << match[1] << "\t";
            getline(inputfile, temp);
            getline(inputfile, temp);
            std::regex_search(temp, match, std::regex("^.*êî ([0-9,]{1,10})"));
            cleanedhtml << match[1] << "\n";
        }
    }
    inputfile.close();
    cleanedhtml.close();
    std::filesystem::remove(ansihtmlfile);
}


// Appends the information of an Aki Aki set .csv into a master .tsv.
void compileAkiAki(std::filesystem::path inputcsv, float conversion) {
    std::string setcode = inputcsv.filename().string().substr(0, 3);
    std::string masterTSVString = "files/stores_databases/Aki Aki/" + setcode + ".tsv";
    // std::string filestring = inputcsv.filename().string();
    // std::string cleanedhtmlpath = "files/stores_databases/Aki Aki/" + filestring.substr(0,filestring.length()-5) + ".csv";
    std::ifstream filecontents;
    filecontents.open(inputcsv);
    std::ofstream masterTSV(masterTSVString, std::ios_base::app);
    std::string line;
    while (!filecontents.eof()) {
        getline(filecontents, line);
        std::smatch match;
        regex_search(line, match, std::regex("^(.*?)\t(.*?)\t(.*?)\t(.*?)\t(.*?)\t(.*)"));
        // std::string nameEN = translateJPNameToEN(match[1], setcode);
        std::string nameEN = getEnglishNameFromCollectorNo(match[2], setcode);
        if (nameEN != "") {
            // std::string modifiers = translateModifiers(match[3]);
            std::string link = match[1];
            std::string isFoil = match[3];
            std::string lang = match[4];
            std::smatch submatch;
            // std::string isFoil = getFoilStatus(match[2]);
            std::string stock;
            float priceJPY = std::stof(std::regex_replace(std::string(match[5]), std::regex("\,"), ""));
            int rounding = float(priceJPY / conversion) * 100;
            float priceUSD = float(rounding) / float(100);
            if (match[5] == "") stock = "0";
            else stock = match[6];
            masterTSV << link << "\t" << nameEN << "\t" << isFoil << "\t" << lang << "\t" << priceUSD << "\t" << stock << "\n";
        }
    }
    filecontents.close();
    // std::filesystem::remove(inputcsv);
    masterTSV.close();
}

// Appends the information of a Hareruya set .csv into a master .tsv.
void compileHareruya(std::filesystem::path filedir, float conversion) {
    std::string setcode = filedir.filename().string().substr(0, 3);
    std::string masterTSVString = "files/stores_databases/Hareruya/" + setcode + ".tsv";
    std::ifstream filecontents;
    filecontents.open(filedir.string());
    std::ofstream masterTSV(masterTSVString, std::ios_base::app);
    std::string line;
    while (!filecontents.eof()) {
        getline(filecontents, line);
        std::smatch match1;
        // 1: Link; 2: Modifiers; 3: Card Name JP; 4: Possible Collector Number; 5: Language; 6: Foil; 7: Price JPY; 8: Stock
        regex_search(line, match1, std::regex("^(.*?)\t.*?Åz(.*?)Ås(.*?)\/(.*?)\t(.*?)\t(.*?)\t(.*?)\t(.*)"));
        std::string link = match1[1];
        std::string collno = match1[4];
        std::string nameEN;
        std::smatch match2;
        if (match1[2] != "") {
            if (std::regex_search(collno, match2, std::regex("^.*?\\(([0-9,]*)"))) {
                nameEN = getEnglishNameFromCollectorNo(match2[1], setcode);
            }
            else {
                nameEN = translateJPNameToEN(match1[3], setcode);
                if (setcode == "BRR" && (match1[2] == "Å°ãåògÅ°" || match1[2] == "ÅyFoilÅzÅ°ãåògÅ°")) {
                }
                else if (setcode == "DMU" && std::string(match1[2]).substr(0, match1[2].length() - 3) == "ÅyÉeÉNÉXÉ`ÉÉÅ[ÅEFoilÅz") {
                    nameEN += " (Textured Foil)";
                }
                else if (setcode == "DMU" && match1[2] == "") {
                }
                else {
                    std::string modifiers = translateModifiers(match1[2]);
                    nameEN += modifiers;
                }
            }
            std::string isFoil = match1[5];
            std::string lang = match1[6];
            std::smatch submatch;
            std::string stock = match1[8];
            float priceJPY = std::stof(std::regex_replace(std::string(match1[7]), std::regex("\,"), ""));
            int rounding = float(priceJPY / conversion) * 100;
            float priceUSD = float(rounding) / float(100);
            masterTSV << link << "\t" << nameEN << "\t" << isFoil << "\t" << lang << "\t" << priceUSD << "\t" << stock << "\n";
        }
    }
    filecontents.close();
    // std::filesystem::remove(filedir.string());
    masterTSV.close();
}

// Appends the information of a Single Star set .csv into a master .tsv.
void compileSingleStar(std::filesystem::path filedir, float conversion) {
    std::string setcode = filedir.filename().string().substr(0, 3);
    std::string masterTSVString = "files/stores_databases/Single Star/" + setcode + ".tsv";
    std::ifstream filecontents;
    filecontents.open(filedir.string());
    std::ofstream masterTSV(masterTSVString, std::ios_base::app);
    std::string line;
    while (!filecontents.eof()) {
        getline(filecontents, line);
        std::smatch match1;
        // 1: Link; 2: Card Name EN + Possible Collector Number + Possible Modifiers; 3: Language; 4: Foil; 5: Price JPY; 6: Stock
        regex_search(line, match1, std::regex("^(.*?)\t(.*?)\t(.*?)\t(.*?)\t(.*?)\t(.*)"));
        if (match1[2] != "" && match1[5] != "") {
            std::string link = match1[1];
            std::string namecell = match1[2];
            std::string lang = match1[3];
            std::string isFoil = match1[4];
            float priceJPY = std::stof(match1[5]);
            std::string stock = match1[6];
            std::smatch submatch;
            std::string nameEN;
            std::smatch match2;
            if (std::regex_search(namecell, match2, std::regex("^(.*?) No\\.([0-9,]*)"))) {
                nameEN = getEnglishNameFromCollectorNo(match2[2], setcode);
            }
            else if (std::regex_search(namecell, match2, std::regex("^(.*?) ([\\(|Åú].*?\\))"))) {
                nameEN = match2[1];
                if (setcode == "BRR" && (match2[2] == "(ãåòg)" || match2[2] == "(ÉRÉåÉNÉ^Å[ÉuÅ[ÉXÉ^Å[î≈ÅAãåòg)" || match2[2] == "Åú (ÉhÉâÉtÉg/ÉZÉbÉgÉuÅ[ÉXÉ^Å[î≈ÅAãåòg)")) {

                }
                else nameEN += translateModifiers(match2[2]);
            }
            else {
                std::regex_search(namecell, match2, std::regex("^(.*?) $"));
                nameEN = match2[1];
            }
            if (lang == "âpåÍî≈") lang = "EN";
            else lang = "JP";
            priceJPY = std::stof(std::regex_replace(std::string(match1[5]), std::regex("\,"), ""));
            int rounding = float(priceJPY / conversion) * 100;
            float priceUSD = float(rounding) / float(100);
            masterTSV << link << "\t" << nameEN << "\t" << isFoil << "\t" << lang << "\t" << priceUSD << "\t" << stock << "\n";
        }
    }
    filecontents.close();
    // std::filesystem::remove(filedir.string());
    masterTSV.close();
}