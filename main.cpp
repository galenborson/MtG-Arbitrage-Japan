#define CURL_STATICLIB
#include "header.h"
#include <string>
#include <iostream>
#include <fstream>
#include <regex>
#include <filesystem>
#include <chrono>
#include <curl/curl.h>
#include <Windows.h>
#include <stdio.h>

AllRecords fullRecords;
float alphaDesired;
float betaDesired;
float conversionUSDToJPY;

static size_t my_write(void* buffer, size_t size, size_t nmemb, void* param)
{
    std::string& text = *static_cast<std::string*>(param);
    size_t totalsize = size * nmemb;
    text.append(static_cast<char*>(buffer), totalsize);
    return totalsize;
}
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

void pullListOfStores() {
    std::cout << "========PULLING LIST OF STORES========\n\n";
    // Aki Aki's pagenum is calculated by roundup(number_of_products / 100). Number of products is identified at "登録アイテム数: 160件"
    fullRecords.addStore("Aki Aki", "https://www.aki-aki.net/product-list/<<urltag>>?page=<<pagenum>>");
    // Hareruya's pagenum is calculated by roundup(number_of_products / 60). Number of products is identified at "商品数: 1105 点"
    fullRecords.addStore("Hareruya", "https://www.hareruyamtg.com/ja/products/search?cardset=<<urltag>>&rarity%5B0%5D=1&rarity%5B1%5D=2&rarity%5B2%5D=3&rarity%5B3%5D=4&foilFlg%5B0%5D=0&foilFlg%5B1%5D=1&stock=1&page=<<pagenum>>");
    // Hareruya's pagenum is calculated by roundup(number_of_products / 60). Number of products cannot be calculated, so the sets file will need to contain a workaround."
    fullRecords.addStore("Single Star", "https://www.singlestar.jp/<<urltag>>/0/normal?page=<<pagenum>>");
}
void curlDownload(std::string inputUrl, std::string filename) {
    std::string htmlcontent;
    std::string file = filename;
    const char* url = inputUrl.c_str();
    std::ofstream outputFile(file);
    CURL* curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &htmlcontent);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (CURLE_OK != res) {
            std::cerr << "CURL error: " << res << '\n';
        }
        else {
            std::cout << "Starting download for " + filename + "... ";
        }
    }
    curl_global_cleanup();
    outputFile << htmlcontent;
    outputFile.close();
    std::cout << "Done\n";
}
void extractMarketPrices(std::string code, std::string name) {
    std::string filename = code + " - " + name;
    std::ifstream pricesInput;
    std::ofstream pricesOutput;
    std::string data;

    // Extracts the price information line
    pricesInput.open("files/market_prices_html/" + code + ".html");
    int lineCount = 0;
    while (lineCount < 768) {
        getline(pricesInput, data);
        ++lineCount;
    }
    pricesInput.close();

    // Overwrites html file with just information line
    pricesOutput.open("files/market_prices_html/" + code + ".html");
    std::string thing = regex_replace(data, std::regex("<\/tr>"), "<\/tr>\n");
    pricesOutput << thing << std::endl;
    pricesOutput.close();
    std::cout << " Done.\n";

    // Extracts individual card info
    std::cout << "Extracting prices for " + code + " to " + code + ".csv... ";
    pricesInput.open("files/market_prices_html/" + code + ".html");
    pricesOutput.open("files/price_databases/" + code + ".tsv");
    std::string card;
    std::string cell;
    std::smatch match;
    while (!pricesInput.eof()) {
        getline(pricesInput, card);
        if (regex_search(card, match, std::regex("<tr class(.*?)/\">([A-Za-z0-9, \.'\:\(\)\-\/]*)"))) {
            cell = match[2];
            pricesOutput << cell << "\t";
        }
        else pricesOutput << "\t";
        if (regex_search(card, match, std::regex("<tr class(.*?)price price_low\"> / .([0-9.,]*)"))) {
            cell = match[2];
            pricesOutput << cell << "\t";
        }
        else pricesOutput << "\t";
        if (regex_search(card, match, std::regex("<tr class(.*?)foil\">\.([0-9.,]*)"))) {
            cell = match[2];
            pricesOutput << cell << "\n";
        }
        else pricesOutput << "\n";
    }
    std::cout << "Done.\n";
    pricesOutput.close();
}
void assignCodesToStores() {
    for (Set& set : fullRecords.getAllSets()) {
        std::string codes = set.getCodes();
        std::string setname = set.getSetCode();
        std::string storename;
        std::smatch match;
        // Something happening between the end of this loop and the start of a new one is causing each store's set-code mapping to completely reset.
        for (Store& store : fullRecords.getAllStores()) {
            if (store.getStoreName() == "Aki Aki") {
                regex_search(codes, match, std::regex("^(.*?),.*"));
                fullRecords.addCodesByStore(0,set.getSetCode(), match[1]);
            }
            else if (store.getStoreName() == "Hareruya") {
                regex_search(codes, match, std::regex("^.*?,(.*?),.*"));
                fullRecords.addCodesByStore(1, set.getSetCode(), match[1]);
            }
            else if (store.getStoreName() == "Single Star") {
                regex_search(codes, match, std::regex("^.*?,.*?,(.*)"));
                fullRecords.addCodesByStore(2, set.getSetCode(), match[1]);
            }
        }
    }
    std::cout << "\n";
}
float convertUSDToJPY() {
    curlDownload("https://finance.yahoo.com/quote/JPY=X/", "files/usd_to_jpy.html");
    std::ifstream oldfile("files/usd_to_jpy.html");
    std::string temp;
    int line = 1;
    while (line <= 18) {
        getline(oldfile, temp);
        ++line;
    }
    std::smatch match;
    std::regex_search(temp, match, std::regex("^.*active=\"\">([0-9]{2,3}\.[0-9]{4})"));
    oldfile.close();
    std::filesystem::remove("files/usd_to_jpy.html");
    return std::stof(match[1]);
}

std::string getFoilStatus(std::string text) {
    std::smatch match;
    std::string status;
    std::regex_search(text, match, std::regex("^.*?([FOILfoil]{4}|【S&amp;C・F】)"));
    if (match[1] != "") status = "Foil";
    return status;
}

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
    while(!inputfile.eof()) {
        getline(inputfile, temp);
        std::smatch match;
        // if (std::regex_search(temp, match, std::regex("^.*model_number\">.*?([0-9]{1,5})"))) {
        if (std::regex_search(temp, match, std::regex("^.*?href=\"(.*?)\".*?model_number\">.*?([0-9]{3,5})"))) {
            cleanedhtml << match[1] << "\t" << match[2] << "\t";
            std::string isFoil = getFoilStatus(temp);
            cleanedhtml << isFoil << "\t";
            if (std::regex_search(temp, match, std::regex("^.*?\\(日"))) {
                cleanedhtml << "JP";
            }
            else cleanedhtml << "EN";
            cleanedhtml << "\t";
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            std::regex_search(temp, match, std::regex("^.*?>(.*?)円"));
            cleanedhtml << match[1] << "\t";
            getline(inputfile, temp);
            getline(inputfile, temp);
            getline(inputfile, temp);
            std::regex_search(temp, match, std::regex("^.*?数 ([0-9,]*?)点"));
            cleanedhtml << match[1] << "\n";
        }
    }
    inputfile.close();
    cleanedhtml.close();
    std::filesystem::remove(ansihtmlfile);
}
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
    while(!inputfile.eof()) {
        getline(inputfile, temp);
        std::smatch match;
        if (std::regex_search(temp, match, std::regex("^.*?<a href=.*?\/ja\/products\/detail\/(.*)"))) {
            std::string link = "https://www.hareruyamtg.com/ja/products/detail/" + std::string(match[1]);
            cleanedhtml << link << "\t";
            getline(inputfile, temp);
            getline(inputfile, temp);
            std::regex_search(temp, match, std::regex("^.*?(【.*?)<"));
            cleanedhtml << match[1] << "\t";
            std::string isFoil = getFoilStatus(temp);
            cleanedhtml << isFoil << "\t";
            if (std::regex_search(temp, match, std::regex("^.*?【JP】"))) {
                cleanedhtml << "JP";
            }
            else cleanedhtml << "EN";
            cleanedhtml << "\t";
            getline(inputfile, temp);
            getline(inputfile, temp);
            std::regex_search(temp, match, std::regex("^.*?<p class.*? ([0-9\,]*?)<"));
            cleanedhtml << match[1] << "\t";
            getline(inputfile, temp);
            std::regex_search(temp, match, std::regex("^.*?在庫:([0-9]*)"));
            cleanedhtml << match[1] << "\n";
        }
    }
    inputfile.close();
    cleanedhtml.close();
    std::filesystem::remove(ansihtmlfile);
}
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
            std::regex_search(temp, match, std::regex("^.*\"goods_name\">.*?\/(.*?)【(.*?)】"));
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
            std::regex_search(temp, match, std::regex("^.*数 ([0-9,]{1,10})"));
            cleanedhtml << match[1] << "\n";
        }
    }
    inputfile.close();
    cleanedhtml.close();
    std::filesystem::remove(ansihtmlfile);
}

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
std::string translateModifiers(std::string modifiers) {
    std::string modifierReturn;
    std::smatch inputmatch;
    std::ifstream modifierMap("files/translations/modifiers.tsv");
    std::string temp;
    bool found = false;
    while (!modifierMap.eof() && !found) {
        getline(modifierMap,temp);
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
        regex_search(line, match1, std::regex("^(.*?)\t.*?】(.*?)《(.*?)\/(.*?)\t(.*?)\t(.*?)\t(.*?)\t(.*)"));
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
                if (setcode == "BRR" && (match1[2] == "■旧枠■" || match1[2] == "【Foil】■旧枠■")) {
                }
                else if (setcode == "DMU" && std::string(match1[2]).substr(0,match1[2].length()-3) == "【テクスチャー・Foil】") {
                    nameEN += " (Textured Foil)";
                }
                else if (setcode == "DMU" && match1[2] == ""){
                } else {
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
            else if (std::regex_search(namecell, match2, std::regex("^(.*?) ([\\(|●].*?\\))"))) {
                nameEN = match2[1];
                if (setcode == "BRR" && (match2[2] == "(旧枠)" || match2[2] == "(コレクターブースター版、旧枠)" || match2[2] == "● (ドラフト/セットブースター版、旧枠)")) {

                } else nameEN += translateModifiers(match2[2]);
            }
            else {
                std::regex_search(namecell, match2, std::regex("^(.*?) $"));
                nameEN = match2[1];
            }
            if (lang == "英語版") lang = "EN";
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

void extractAndCompile(std::string inputhtml, std::string inputcsvorigin, std::string storename, std::string currentset) {
    std::filesystem::path outputhtml(inputhtml);
    std::filesystem::path outputcsv("files/stores_databases/" + storename + "/" + currentset + ".csv");
    std::string referenceCSV = "files/stores_databases/" + storename + "/" + inputcsvorigin.substr(0,inputcsvorigin.length()-5) + ".csv";
    if (storename == "Aki Aki") {
        extractAkiAki(outputhtml);
        compileAkiAki(referenceCSV, conversionUSDToJPY);
    }
    else if (storename == "Hareruya") {
        extractHareruya(outputhtml);
        compileHareruya(referenceCSV, conversionUSDToJPY);
    }
    else if (storename == "Single Star") {
        extractSingleStar(outputhtml);
        compileSingleStar(referenceCSV, conversionUSDToJPY);
    }
}

void downloadStoreLinks(Store& store) {
    const auto& tagsMap = store.separateURLTags();
    std::map<std::string, std::string> urlTagMap = store.getURLTags();
    std::string result;
    std::regex urltag("<<urltag>>");
    std::regex pagenum("<<pagenum>>");
    for (const auto& x : tagsMap) {
        std::string currentset = x.first;
        std::vector<std::string> currentsetcodes = x.second;
        for (const auto& y : currentsetcodes) {
            std::smatch match;
            std::regex splitter("([^;]*);([^\s]*)");
            regex_search(y, match, splitter);
            std::string tagstring = match[1];
            std::string pagesmax = match[2];
            int currentpage = 1;
            int maxint = stoi(pagesmax);
            while (currentpage <= maxint) {
                std::string urlbase = store.getURLConstructor();
                urlbase = std::regex_replace(urlbase, urltag, tagstring);
                std::string urlpages = std::regex_replace(urlbase, pagenum, std::to_string(currentpage));
                std::string tagstringdownloadable = regex_replace(tagstring, std::regex("\/"), "_");
                std::string storename = store.getStoreName();
                std::string outputhtml = "files/stores_html/" + storename + "/" + currentset + " " + tagstringdownloadable + " - " + std::to_string(currentpage) + ".html";
                // If the file doesn't already exist or has been modified within the past 24 hours, (re)download it
                std::filesystem::path mypath = outputhtml;
                std::filesystem::path existingCSV = "files/stores_databases/" + storename + "/" + currentset + " " + tagstringdownloadable + " - " + std::to_string(currentpage) + ".csv";
                std::string csvnameorigin = mypath.filename().string();
                std::filesystem::file_time_type filetime;
                try {
                    filetime = std::filesystem::last_write_time(existingCSV);
                    auto fileClock = std::chrono::clock_cast<std::chrono::system_clock>(filetime);
                    auto fileEpoch = std::chrono::system_clock::to_time_t(fileClock);
                    std::time_t nowt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                    float difference = difftime(nowt, fileEpoch);
                    if (difference / 86400 >= 5) {
                        std::cout << "File " << existingCSV << " last updated " << difference / 86400 << " days ago. Updating now...\n";
                        curlDownload(urlpages, outputhtml);
                        extractAndCompile(outputhtml, csvnameorigin, storename, currentset);
                    }
                    else {
                        std::cout << "File " << existingCSV << " last updated " << difference / 86400 << " days ago. Will not update.\n";
                    }
                }
                catch (const std::filesystem::filesystem_error& e) {
                    std::cout << "File " << existingCSV << " not found. Downloading now...\n";
                    curlDownload(urlpages, outputhtml);
                    extractAndCompile(outputhtml, csvnameorigin, storename, currentset);
                    std::filesystem::remove(outputhtml);
                }
                ++currentpage;
            }
        }
    }
}

void fetchStorePrices(Store& store) {
    std::string storename = store.getStoreName();
    std::filesystem::path inputdir = "files/stores_html/" + storename;
    // For each file in a particularly directory, extract just the price information and delete the rest.
    for (const auto& entry : std::filesystem::directory_iterator(inputdir)) {
        if (storename == "Aki Aki") {
            extractAkiAki(entry);
        }
        else if (storename == "Hareruya") {
            extractHareruya(entry);
        }
        else if (storename == "Single Star") {
            extractSingleStar(entry);
        }
    }
}
void compileStorePrices(Store& store, float conversion) {
    std::string storename = store.getStoreName();
    std::cout << "Compiling lists for " << storename << "\n";
    std::filesystem::path inputdir = "files/stores_databases/" + storename + "/";
    for (const auto& entry : std::filesystem::directory_iterator(inputdir)) {
        std::string csvFilename = entry.path().string();
        if (csvFilename.substr(csvFilename.find_last_of(".") + 1) == "csv") {
            if (storename == "Aki Aki") {
                compileAkiAki(entry, conversion);
            }
            else if (storename == "Hareruya") {
                compileHareruya(entry, conversion);
            }
            else if (storename == "Single Star") {
                compileSingleStar(entry, conversion);
            }
        }
    }
}

void generateFinalOutput() {
    std::ifstream market_prices;
    time_t now = time(0);
    std::string dt = ctime(&now);
    std::string outputString = "files/output/output.csv";
    std::filesystem::remove(outputString);
    std::ofstream masterOutput(outputString, std::ios_base::app);
    masterOutput << "Store Name" << "\t" << "Product Link" << "\t" << "Set Code" << "\t" << "Card Name" << "\t" << "Foil" << "\t" << "Lang" << "\t" << "Card Price" << "\t" << "Stock" << "\t" << "Expected USD Profit" << "\t" << "Expected ROI" << "\t" << "Weight" << "\t" << "Tier" << "\n";
    for (Store& store : fullRecords.getAllStores()) {
        std::string storeName = store.getStoreName();
        std::filesystem::path inputdir = "files/stores_databases/" + store.getStoreName() + "/";
        for (const auto& entry : std::filesystem::directory_iterator(inputdir)) {
            if (entry.path().filename().string().substr(0, 6) != ".~lock") {
                std::ifstream setStore(entry.path().string());
                std::string setCode = entry.path().filename().string().substr(0, 3);
                std::string setMarketPath = "files/price_databases/" + setCode + ".tsv";
                std::string entryStore;
                // 1: Link; 2: Card Name; 3: Foil Status; 4: Language; 5: Card Price; 6: Stock
                std::regex entryStore_re("^(.*?)\t(.*?)\t(.*?)\t(.*?)\t(.*?)\t(.*)");
                std::smatch matchStore;
                // 1: Card Name; 2: Nonfoil Price; 3: Foil Price
                std::regex entryMarket_re("^(.*?)\t(.*?)\t(.*)");
                while (!setStore.eof()) {
                    getline(setStore, entryStore);
                    std::regex_search(entryStore, matchStore, entryStore_re);
                    if (matchStore[2] != "" && matchStore[6] != "0" && matchStore[6] != "") {
                        std::ifstream setMarket(setMarketPath);
                        std::string entryMarket;
                        std::smatch matchMarket;
                        bool found = false;
                        while (!setMarket.eof() && !found) {
                            getline(setMarket, entryMarket);
                            std::regex_search(entryMarket, matchMarket, entryMarket_re);
                            if (matchMarket[1] == matchStore[2]) {
                                std::string link = matchStore[1];
                                std::string cardName = matchStore[2];
                                std::string isFoil = matchStore[3];
                                std::string lang = matchStore[4];
                                std::string priceDisplay = matchStore[5];
                                float priceStore = std::stof(matchStore[5]);
                                std::string stock = matchStore[6];
                                float priceMarket;
                                if (isFoil != "" && matchMarket[3] != "") priceMarket = std::stof(matchMarket[3]);
                                else { 
                                    priceMarket = std::stof(matchMarket[2]);
                                }
                                float alpha = priceMarket - priceStore;
                                float beta = ((priceMarket / priceStore)-1) * 100;
                                float gamma = alpha * beta;
                                char tier;
                                if (alpha >= alphaDesired && beta >= betaDesired) {
                                    tier = 'A';
                                }
                                else if (alpha >= alphaDesired && beta > 0) {
                                    tier = 'C';
                                }
                                else if (alpha > 0 && beta >= betaDesired) {
                                    tier = 'B';
                                }
                                else if (alpha > 0 && beta > 0) {
                                    tier = 'D';
                                }
                                else tier = 'F';
                                if (tier == 'A' || tier == 'B' || tier == 'C' || tier == 'D') {
                                    std::cout << storeName << "\t" << link << "\t" << setCode << "\t" << cardName << "\t" << isFoil << "\t" << lang << "\t" << priceDisplay << "\t" << stock << "\t" << alpha << "\t" << beta << "\t" << gamma << "\t" << tier << "\n";
                                    masterOutput << storeName << "\t" << link << "\t" << setCode << "\t" << cardName << "\t" << isFoil << "\t" << lang << "\t" << priceDisplay << "\t" << stock << "\t" << alpha << "\t" << beta << "\t" << gamma << "\t" << tier << "\n";
                                }
                                found = true;
                            }
                        }
                        setMarket.close();
                    }
                }
                setStore.close();
            }
        }
    }
    masterOutput.close();
}

void resetMasterTSVs() {
    for (Store& store : fullRecords.getAllStores()) {
        for (Set& set : fullRecords.getAllSets()) {
            std::string masterTSVDirectory = "files/stores_databases/" + store.getStoreName() + "/" + set.getSetCode() + ".tsv";
            std::filesystem::remove(masterTSVDirectory);
        }
    }
}

int main() {
    std::ifstream fileOfSets;
    std::string currentSet;
    std::string fileString;
    std::string setTag;
    std::cout << "Input your desired minimum profit in USD as a float (5.00 recommended): ";
    std::cin >> alphaDesired;
    std::cout << "Input your desired minimum ROI percentage as a float (200 recommended): ";
    std::cin >> betaDesired;
    std::cout << "\n========UPDATING USD TO JPY CONVERSION========\n";
    conversionUSDToJPY = convertUSDToJPY();

    std::cout << "========ASSEMBLING LIST OF STORES========\n";
    pullListOfStores();
    resetMasterTSVs();

    // Downloads all USD market prices as html
    std::cout << "========DOWNLOADING USD MARKET PRICES========\n";
    fileOfSets.open("files/sets.txt");
    std::vector<Store> allStores = fullRecords.getAllStores();
    while (!fileOfSets.eof()) {
        getline(fileOfSets, currentSet);
        std::smatch setMatch;
        // This method for breaking apart the sets file into a map is 
        regex_search(currentSet, setMatch, std::regex("^(.*?),(.*?),(.*?),(.*)"));
        std::string code = setMatch[1];
        std::string name = setMatch[2];
        std::string marketURL = setMatch[3];
        std::string allCodes = setMatch[4];
        std::string htmlFilename = "files/market_prices_html/" + std::string(code) + ".html";
        curlDownload(marketURL, htmlFilename);
        std::string csvfile = "files/market_prices_html/" + std::string(code) + ".csv";
        fullRecords.addSet(code, name, csvfile, allCodes);
        std::cout << "Done\n";
    }
    std::cout << "\n";
    fileOfSets.close();

    assignCodesToStores();

    // Converts market price htmls to csv
    std::cout << "========EXTRACTING PRICES TO CSVS========\n";
    fileOfSets.open("files/sets.txt");
    while (!fileOfSets.eof()) {
        getline(fileOfSets, currentSet);
        std::smatch setMatch;
        regex_search(currentSet, setMatch, std::regex("^(.*?),(.*?),(.*?)"));
        std::string setcode = setMatch[1];
        std::string setname = setMatch[2];
        extractMarketPrices(setcode, setname);
    }
    
    // In this step, we convert the store codes to urls and download them all.
    for (Store& store : fullRecords.getAllStores()) {
        downloadStoreLinks(store);
    }

    std::cout << "========GENERATING FINAL OUTPUT TO \"files/output/output.csv\"--------\n";
    generateFinalOutput();
    return 0;
}