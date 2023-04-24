
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <vector>
#include <set>
#include <map>
#include <chrono>
#include <thread>
#include <cstdlib>
#include "include/fedm/ReaderModule.h"
#include "include/fedm/UsbManager.h"

using namespace FEDM;
using namespace FEDM::TagHandler;
using namespace FEDM::Utility;
using namespace std;

time_t lastUniqueTagTime = 0;

class Inventory_TagItem_withRSSI_Sample
{
public:
    static void run(ReaderModule &reader)
    {
        InventoryParam inventoryParam;

        // Enable antenna 1 before turning on all antennas
        inventoryParam.setAntennas((uint8_t)0x01);
        int state = reader.hm().inventory(true, inventoryParam);
        if (state != ErrorCode::Ok) { /* Add error-handling... */ }

        // Set antennas to ANT1, ANT2, ANT3, and ANT4
        inventoryParam.setAntennas((uint8_t)0x0F);

        // Open file for appending
        ofstream file;
        file.open("tag_data_rssi.csv", std::ios::app);

        // Get start time
        auto startTime = std::chrono::steady_clock::now();
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsedSeconds;

        // Maintain a map of tag IDs and their last read time
        map<string, chrono::steady_clock::time_point> lastReadTime;

        // Loop for 40 seconds
        while (elapsedSeconds.count() < 40.0)
        {
            int state = reader.hm().inventory(true, inventoryParam);
            if (state != ErrorCode::Ok) { /* Add error-handling... */ }

            // Number of read tags
            size_t count = reader.hm().itemCount();

            // Create TagItem for each tag and output IDD and RSSI values
            for (size_t itemIndex = 0; itemIndex < count; itemIndex++)
            {
                // Create TagItem
                TagItem tagItem = reader.hm().tagItem(itemIndex);

                // Output IDD of TagItem
                string iddString = tagItem.iddToHexString();
                std::cout << "IDD: " << iddString << std::endl;

                // Check if the tag has been read within the last 5 seconds
                auto lastRead = lastReadTime.find(iddString);
                if (lastRead != lastReadTime.end())
                {
                    auto timeSinceLastRead = currentTime - lastRead->second;
                    if (chrono::duration_cast<chrono::seconds>(timeSinceLastRead).count() < 5)
                    {
                        continue;
                    }
                }

                // Update last read time for the tag
                lastReadTime[iddString] = currentTime;

                // Create List for RSSI values of TagItem
                vector<RssiItem> rssiListe = tagItem.rssiValues();
                vector<RssiItem>::iterator itor;

                for (RssiItem rssiItem : rssiListe)
                {
                    // Antenna Number
                    int antennaNumber = rssiItem.antennaNumber();
                    std::cout << "Ant: " << antennaNumber;

                    // Save tagID and antenna number to CSV file
                    file << "TagID:" << iddString << ", ANT:" << antennaNumber << ", Time:" << currentTime << ", RSSI:" << rssiItem.rssiValue() << std::endl;
                }
            }

            // Update elapsed time
            currentTime = std::chrono::steady_clock::now();
            elapsedSeconds = currentTime - startTime;
        }

        // Close file
        file.close();
    }
};


// Class for USB Manager Connection
class UsbManagerConnect : IUsbListener
{
    public:
    static UsbManager usbManager;

    static void SampleMain()
    {
        UsbManagerConnect usbManager;
        usbManager.Run();
    }

    void Run()
    {
        // Start USB discovery
        usbManager.startDiscover(this);
        cout << "Waiting for Connection with first USB Reader...." << endl;

        // Wait for first USB device
        while (usbManager.readerCount() == 0)
        {
        }

        // Get Connector information
        UsbScanInfo usbScanInfo = usbManager.popDiscover();
        usbManager.stopDiscover();

        // Check if scan info is valid
        if (!usbScanInfo.isValid())
        {
            cout << "Error: Invalid usb scan info" << endl;
            return;
        }

        Connector connector = usbScanInfo.connector();
        info.connector();
        if(connector.isValid())
        {
            // Create Reader Module with Request Mode UniDirectional = Advanced Protocol
            ReaderModule* reader = new ReaderModule(RequestMode::UniDirectional);
            cout << "Start connection with Reader: " << usbScanInfo.readerTypeToString() << " (" << connector.usbDeviceIdToHexString() << ")..." << endl;
            // Connect to the reader
            int state = reader->connect(connector);
                cout << "Connect: " << reader->lastErrorStatusText() << endl;
        // Error handling
        if (state != ErrorCode::Ok)
        {
            return;
        }

        set<string> uniqueTags;

        // Configure antennas

        // Perform scans until at least 2 tags are found. Print the scan number and the number of tags found
        int scanNum = 0;
        while (uniqueTags.size() == 0) {
            scanNum++;
            // Inventory tags
            state = reader->hm().inventory();
            cout << "inventory: " << reader->lastErrorStatusText() << endl;
            if (state != ErrorCode::Ok) { /* Add error-handling... */ }

            // Get tag count
            size_t count = reader->hm().itemCount();

            // Output number of tags found
            std::cout << "No. of tags found in scan " << scanNum << ": " << count << std::endl;

            // Save data to uniqueTags set
            for (size_t itemIndex = 0; itemIndex < count; itemIndex++)
            {
                // Create TagItem
                TagItem tagItem = reader->hm().tagItem(itemIndex);

                // Get IDD of TagItem
                string iddString = tagItem.iddToHexString();

                // Insert IDD to uniqueTags set
                uniqueTags.insert(iddString);
            }
        }

        // Output unique tags found
        std::cout << "No. of unique tags found: " << uniqueTags.size() << std::endl;

        // Save data to CSV file
        if (!uniqueTags.empty()) {
            // Open file for appending
            ofstream file;
            file.open("tag_data_rssi.csv", std::ios::app);

            // Create TagItem for each unique tag and output IDD
            for (auto it = uniqueTags.begin(); it != uniqueTags.end(); ++it)
            {
                // Display IDD on screen
                std::cout << "IDD: " << *it << std::endl;
            cout << "\n" << reader->info().readerTypeToString() << " disconnected." << endl;
            }
        }
        delete reader;
        
    }

    void onUsbEvent()
    {
        // Not used
    }
};
}

UsbManager UsbManagerConnect::usbManager;

int main()
{
    UsbManagerConnect::SampleMain();
    return 0;
}

