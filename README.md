# Introduction

This is a price scraper of various Magic: the Gathering superstores based in Japan. It converts the prices of all detected cards into USD, compares them to US market prices, ad spits out a tab-separated .CSV with all the best deals.

Program is written in C++ using curl.

# Disclaimer

* This program is merely a tool and not professional financial advice. Any purchases made using this tool do not guarantee any profit. **Check each card yourself before confirming a purchase.**
* Current USD market prices are pulled from EchoMTG.com. While non-foil prices tend to be fairly accurate to TCGPlayer Low, foils are often inflated.

# How To Use

* Run "MtG_PriceComparisons_Japan.exe."
* Input your Expected USD Profit and Expected ROI values.
  * To compensate for international shipping and third party resale fees, the program recommends values of 5 and 200 respectively. Depending on your needs, you are free to input whatever values you'd like.
* Allow the program to download any price updates (this can be very large).
* To remove any sets, delete the set's line in the "files/sets.txt" file.

Once completed, it will export a .csv to "files/output/output.csv"

![image](https://user-images.githubusercontent.com/127451072/227806523-bde604ba-68d1-4808-9976-7f39bd2c8322.png)

# Features
This section explains the data contained in the exported .csv.
* **Store Name**: Current list of stores is Aki Aki, Hareruya, and Single Star. More will be added in the future.
* **Product Link**: URL to the entry's item listing.
* **Set Code**: Three-letter code corresponding to the entry's set.
* **Card Name**: This includes any modifiers like "Borderless," "Showcase," "Foil Etched," etc.
* **Foil Status**
* **Card Price**: In USD
* **Stock**
* **Expected USD Profit**
* **Expected ROI**
* **Weight**: The product of Expected USD Profit and Expected ROI. Useful when sorting for quickly identifying the best deals.
* **Tiers**: An organization method for a card's estimated value. Entries with a weight ≤ 0 are not included.
  * A tier: The listing's price is ≥ your Expected USD Profit, AND the calculated ROI is ≥ your Expected ROI.
  * B tier: The listing's price is < your Expected USD Profit, BUT the calculated ROI is ≥ your Expected ROI.
  * C tier: The listing's price is ≥ your Expected USD Profit, BUT the calculated ROI is < your Expected ROI.
  * D tier: The listing's price is < your Expected USD Profit, AND the calculated ROI is < your Expected ROI. Both values are still greater than 0.
