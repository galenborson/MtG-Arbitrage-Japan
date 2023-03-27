#define CURL_STATICLIB
#include "records.h"
#include "extractcompile.h"
#include <fstream>
#include <regex>
#include <filesystem>
#include <chrono>
#include <curl/curl.h>
#include <Windows.h>

AllRecords fullRecords;
float alphaDesired;
float betaDesired;

// Downloads a given URL (inputURL) to the destination (filename).
static size_t my_write(void* buffer, size_t size, size_t nmemb, void* param) {
    std::string& text = *static_cast<std::string*>(param);
    size_t totalsize = size * nmemb;
    text.append(static_cast<char*>(buffer), totalsize);
    return totalsize;
}
void curlDownload(std::string inputURL, std::string filename) {
    std::string htmlcontent;
    std::string file = filename;
    const char* url = inputURL.c_str();
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

// Derives the current USD to JPY exchange rate.
// Pulled from finance.yahoo.com.
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

// Compares the most recent write time of a given file (inputfile) with an amount of time (timediff) in days.
int fileUpdateCheck(std::filesystem::path inputfile, float timediff) {
    std::filesystem::file_time_type filetime;
    try {
        filetime = std::filesystem::last_write_time(inputfile);
        auto fileClock = std::chrono::clock_cast<std::chrono::system_clock>(filetime);
        auto fileEpoch = std::chrono::system_clock::to_time_t(fileClock);
        std::time_t nowt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        float difference = difftime(nowt, fileEpoch);
        if (difference / 86400 >= timediff) {
            return 1;
        }
        else {
            return 0;
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        return 2;
    }
}
void resetMasterTSVs() {
    for (Store& store : fullRecords.getAllStores()) {
        for (Set& set : fullRecords.getAllSets()) {
            std::string masterTSVDirectory = "files/stores_databases/" + store.getStoreName() + "/" + set.getSetCode() + ".tsv";
            std::filesystem::remove(masterTSVDirectory);
        }
    }
}


// Converts market prices for a given set identified by (code). Exports as a .tsv.
void extractMarketPrices(std::string code) {
    std::string inputhtml = "files/market_prices_html/" + code + ".html";
    std::ifstream pricesInput;
    std::ofstream pricesOutput;
    std::string data;

    // Extracts the price information line
    pricesInput.open(inputhtml);
    int lineCount = 0;
    while (lineCount < 768) {
        getline(pricesInput, data);
        ++lineCount;
    }
    pricesInput.close();

    // Overwrites html file with just information line
    pricesOutput.open(inputhtml);
    std::string thing = regex_replace(data, std::regex("<\/tr>"), "<\/tr>\n");
    pricesOutput << thing << std::endl;
    pricesOutput.close();
    std::cout << " Done.\n";

    // Extracts individual card info to .tsv
    std::cout << "Extracting prices for " + code + " to " + code + ".csv... ";
    pricesInput.open(inputhtml);
    pricesOutput.open("files/price_databases/" + code + ".csv");
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
    pricesInput.close();
    // std::filesystem::remove(inputhtml);
    pricesOutput.close();
}

// Assigns each store a list of url slugs that it uses to iterate through multiple pages of a single set.
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

// For each store, check to see if their card prices are up to date.
// Any store or set not updated within the last 24 hours is updated.
void downloadStoreLinks() {
    for (Store& store : fullRecords.getAllStores()) {
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
                    switch (fileUpdateCheck(existingCSV, 1)) {
                    case 0: {
                        std::cout << "File " << existingCSV << " is up to date.\n";
                        break;
                    }
                    case 1: {
                        std::cout << "File " << existingCSV << " is out of date. Updating now...\n";
                        curlDownload(urlpages, outputhtml);
                        extractAndCompile(outputhtml, csvnameorigin, storename, currentset);
                        break;
                    }
                    case 2: {
                        std::cout << "File " << existingCSV << " not found. Downloading now...\n";
                        curlDownload(urlpages, outputhtml);
                        extractAndCompile(outputhtml, csvnameorigin, storename, currentset);
                        std::filesystem::remove(outputhtml);
                        break;
                    }
                    }
                    ++currentpage;
                }
            }
        }
    }
}

// Compares all store card prices to the current market rate and exports the best deals to a .csv.
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
            std::string filename = entry.path().filename().string();
            if (filename.substr(filename.length()-3, 3) == "tsv") {
                std::ifstream setStore(entry.path().string());
                std::string setCode = filename.substr(0, 3);
                std::string setMarketPath = "files/price_databases/" + setCode + ".csv";
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
                                    // std::cout << storeName << "\t" << link << "\t" << setCode << "\t" << cardName << "\t" << isFoil << "\t" << lang << "\t" << priceDisplay << "\t" << stock << "\t" << alpha << "\t" << beta << "\t" << gamma << "\t" << tier << "\n";
                                    std::cout << "Found " << tier << " tier listing in " << storeName << ": " << setCode << " "; 
                                    if (isFoil == "Foil") std::cout << "*" << isFoil << "* ";
                                    std::cout << cardName << " for $" << priceDisplay << "\n";
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

// Iterates through every store to compile their card prices into one document per set.
void extractAndCompile(std::string inputhtml, std::string inputcsvorigin, std::string storename, std::string currentset) {
    std::filesystem::path outputhtml(inputhtml);
    std::filesystem::path outputcsv("files/stores_databases/" + storename + "/" + currentset + ".csv");
    std::string referenceCSV = "files/stores_databases/" + storename + "/" + inputcsvorigin.substr(0, inputcsvorigin.length() - 5) + ".csv";
    if (storename == "Aki Aki") {
        extractAkiAki(outputhtml);
        compileAkiAki(referenceCSV, fullRecords.getConversion());
    }
    else if (storename == "Hareruya") {
        extractHareruya(outputhtml);
        compileHareruya(referenceCSV, fullRecords.getConversion());
    }
    else if (storename == "Single Star") {
        extractSingleStar(outputhtml);
        compileSingleStar(referenceCSV, fullRecords.getConversion());
    }
}

// For each set, check to see if their market prices are up to date.
// Any set not updated within the past hour is updated.
void downloadMarketPrices() {
    std::ifstream fileOfSets;
    std::string currentSet;
    std::string fileString;
    std::string setTag;
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
        std::string csvFilename = "files/price_databases/" + std::string(code) + ".csv";
        std::string temporaryCSV = "files/market_prices_html/" + std::string(code) + ".csv";
        switch (fileUpdateCheck(csvFilename, .04167)) {
        case 0: {
            std::cout << "Prices for " << code << " are up to date.\n";
            fullRecords.addSet(code, name, temporaryCSV, allCodes);
            break;
        }
        case 1: {
            std::cout << "Prices for " << code << " are out of date. Updating now...\n";
            curlDownload(marketURL, htmlFilename);
            extractMarketPrices(code);
            fullRecords.addSet(code, name, temporaryCSV, allCodes);
            std::cout << "Done\n";
            break;
        }
        case 2: {
            std::cout << "No prices for " << code << " found. Downloading now...\n";
            curlDownload(marketURL, htmlFilename);
            extractMarketPrices(code);
            fullRecords.addSet(code, name, temporaryCSV, allCodes);
            std::cout << "Done\n";
            break;
        }
        }
    }
    std::cout << "\n";
    fileOfSets.close();
}


// The program's main execution call.
int main() {
    std::cout << "Input your desired minimum profit in USD as a float (5.00 recommended): ";
    std::cin >> alphaDesired;
    std::cout << "Input your desired minimum ROI percentage as a float (200 recommended): ";
    std::cin >> betaDesired;
    std::cout << "\n========UPDATING USD TO JPY CONVERSION========\n";
    fullRecords.setConversion(convertUSDToJPY());
    std::cout << "$1.00 USD = " << fullRecords.getConversion() << " JPY.\n";

    // Assigns stores their URL slugs.
    std::cout << "\n========POPULATING LIST OF STORES========\n";
    fullRecords.populateStores();

    // Deletes old files.
    resetMasterTSVs();
    std::cout << "Done\n";

    // If prices are out of date, downloads all USD market prices as html.
    std::cout << "\n========DOWNLOADING USD MARKET PRICES========\n";
    downloadMarketPrices();

    // For each set in "files/sets.txt," assigns each store the respective list of url slugs & number of pages per set.
    assignCodesToStores();

    // In this step, we convert the store codes to urls and download them all.
    downloadStoreLinks();

    // Compares all store listings with their US market prices and exports results to a final csv.
    std::cout << "========GENERATING FINAL OUTPUT TO \"files/output/output.csv\"--------\n";
    generateFinalOutput();
    std::cout << "\n**Congratulations!**\n";
    std::cout << "Your file has been successfully generated in \"files/output/output.csv\"\n";
    std::cout << "Now you can open it in your favorite spreadsheet editor to sort through all available listings.\n\n";
    system("pause");
    return 0;
}