#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <vector>
#include <set>
#include <chrono>
#include <thread>
#include <deque>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include "include/fedm/ReaderModule.h"
#include "include/fedm/UsbManager.h"

using namespace FEDM;
using namespace FEDM::TagHandler;
using namespace FEDM::Utility;
using namespace std;

// **********************
// MIN 2 tags per patient
// 60fps - video split - config file
// for fplit -- change the cmd to remote pi
// update PC
// filename: tagID_ANT1_ANT0_date_time_cam1.h264
// keep log file just in case
// move PC, test
//  
// **********************

void start_recording(const std::string &tagID)
{
    std::string command = "python3 AMBIENT_vid.py start " + tagID;
    system(command.c_str());
}

void stop_recording()
{
    std::string command = "python3 AMBIENT_vid.py stop ";
    system(command.c_str());
}

void split_file(const std::string &tagID, const std::string &tag2_start, const std::string &tag1_duration, const std::string &tag2_duration)
{
    std::string command = "python3 AMBIENT_vid.py split " + tagID + tag2_start + tag1_duration + tag2_duration;
    system(command.c_str());
}

void send_file()
{
    std::string command = "python3 AMBIENT_vid.py send ";
    system(command.c_str());
}


// std::map<std::string, bool> recordingStatus;
std::map<std::string, std::pair<bool, int>> recordingStatus;

// define params
// int tag1_start, tag2_start;
// int tag1_duration, tag2_duration;
// tag1_start = 0;

// initialize tag1_start
int64_t tag1_start = 0;
int64_t tag2_start = 0;
// duration
int64_t tag1_duration = 0;
int64_t tag2_duration = 0;

auto two_tag = false;

// auto end_time = std::chrono::steady_clock::now();

class Inventory_Queue_Sample
{
public:
    static void run(ReaderModule &reader)
    {
        InventoryParam inventoryParam;

        // Set antenna to ANT1, ANT2, ANT3, and ANT4
        inventoryParam.setAntennas((uint8_t)0x0F);

        reader.hm().setUsageMode(FEDM::Hm::UsageMode::UseQueue);

        int state = reader.hm().inventory(true, inventoryParam);
        // cout << "inventory: " << reader.lastErrorStatusText() << endl;
        if (state != ErrorCode::Ok) { /* Add error-handling... */ }

        // Number of read tags
        size_t count = reader.hm().queueItemCount();
        // cout << "No. of tags: " << count << endl;

        // Create TagItem for each tag and output IDD
        while (reader.hm().queueItemCount() > 0)
        {
            unique_ptr<const TagItem> tagItem = reader.hm().popItem();

            // Output IDD of TagItem
            string iddString = tagItem->iddToHexString();
            cout << "IDD: " << iddString << endl;

            // Check if the tag ID is in the recordingStatus map
            auto it = recordingStatus.find(iddString);

            if (it == recordingStatus.end())
            {
                // If not in the map, add it and start recording
                recordingStatus[iddString] = {true, 1}; // {recording: bool, scan_count: int}
                // start recording only if no other tag is recording
                if (recordingStatus.size() == 1)
                {
                    start_recording(iddString);
                    // start time in sec fset to 0
                    auto start_time = std::chrono::steady_clock::now();
                    // start of tag1 is time 0
                    tag1_start = std::chrono::duration_cast<std::chrono::seconds>(start_time.time_since_epoch()).count();
                    // tag1_start = start_time;

                }
            }
            else
            {
                // Increment the scan count
                recordingStatus[iddString].second++;

                // Check if any other tag is recording
                bool otherTagActive = false;
                for (const auto &status : recordingStatus)
                {
                    if (status.first != iddString && status.second.first) // If another tag is active
                    {
                        otherTagActive = true;
                        // tag2_start
                        tag2_start = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
                        break;
                    }
                }

                // If the tag is already recording and its scan count is less than 2, continue recording
                if (recordingStatus[iddString].first && recordingStatus[iddString].second < 2)
                {
                    continue;
                }

                // // If there is another active tag and this tag has been scanned twice, stop recording
                // if both tags have been scanned twice
                if (otherTagActive && recordingStatus[iddString].second == 2)
                {
                    // time until now
                    tag1_duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - tag1_start;
                    // check if the other tag has been scanned twice
                    for (const auto &status : recordingStatus)
                    {
                        if (status.first != iddString && status.second.second == 2)
                        {
                            recordingStatus[iddString].first = false;
                            stop_recording();
                            // time until now for tag2
                            tag2_duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - tag2_start;
                            two_tag = true;
                            break;
                        }
                    }
                    // recordingStatus[iddString].first = false;

                    // stop_recording();
                    continue;
                }

                // if (otherTagActive && recordingStatus[iddString].second == 2)
                // {
                //     recordingStatus[iddString].first = false;
                //     stop_recording();
                //     continue;
                // }
                // *******************************************************
                // otherTag is not active, single tag case ****TEST IT****
                // *******************************************************
                else if (recordingStatus[iddString].second == 2)
                {
                    recordingStatus[iddString].first = false;
                    stop_recording();
                    // time until tag1 end
                    tag1_duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - tag1_start;
                } // *****************************************************
                // If in the map and recording is not ongoing, start recording
                else
                {
                    recordingStatus[iddString] = {true, 1};
                    start_recording(iddString);
                }
                
            }

            // Create List for RSSI values of TagItem
            vector<RssiItem> rssiList = tagItem->rssiValues();
            vector<RssiItem>::iterator itor;

            for (RssiItem rssiItem : rssiList)
            {
                // Antenna Number
                int antennaNumber = rssiItem.antennaNumber();
                std::cout << "Ant: " << antennaNumber;
            }

            // send and split if needed
            if (two_tag)
            {
                split_file(iddString, tag2_start, tag1_duration, tag2_duration);
                send_file();
            }
            else
            {
                send_file();
            }

        }

        // Save tags and antennas to a logfile
        // ofstream logfile("logfile.txt", ios::out | ios::app);
        // if (!logfile)
        // {
        //     cerr << "Unable to open logfile." << endl;
        //     return;
        // }

        // for (const auto &status : recordingStatus)
        // {
        //     // Retrieve the IDD and scan count
        //     string idd = status.first;
        //     int scanCount = status.second.second;

        //     // Log the IDD, antenna, and time of the scan
        //     logfile << "IDD: " << idd << ", Antenna: " << antennaNumber << ", Scans: " << scanCount << ", Time: " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count() << endl;
        // }

        // logfile.close();
    }
};


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
    // Start USB Discover
    usbManager.startDiscover(this);
    cout << "Waiting for Connection with first USB Reader...." << endl;;

    // Wait for first BLE device
    while (usbManager.readerCount() == 0)
    {
    }

    // Get the Connector by BLE Manager
    UsbScanInfo usbScanInfo = usbManager.popDiscover();
    usbManager.stopDiscover();

    if (!usbScanInfo.isValid())
    {
        cout << "Error: Invalid usb scan info" << endl;
        return;
    }

    Connector connector = usbScanInfo.connector();
    if (connector.isValid())
    {
        // Create Reader Module with Request Mode UniDirectional = Advanced Protocol
        ReaderModule* reader = new ReaderModule(RequestMode::UniDirectional);
        cout << "Start connection with Reader: " << usbScanInfo.readerTypeToString() << " (" << connector.usbDeviceIdToHexString() << ")..." << endl;
        // Connect
        int state = reader->connect(connector);

        cout << "Connect: " << reader->lastErrorStatusText() << endl;
        // Error handling
        if (state != ErrorCode::Ok)
        {
            return;
        }

        set<string> uniqueTags;

        // config antennas
        
        int scanNum = 0;
        // perform scans
        while (uniqueTags.size() == 0) {
            scanNum++;
            // Inventory tags
            state = reader->hm().inventory();
            // cout << "inventory: " << reader->lastErrorStatusText() << endl;
            if (state != ErrorCode::Ok) { /* Add error-handling... */ }

            // Get tag count
            size_t count = reader->hm().itemCount();

            // Output number of tags found
            // std::cout << "No. of tags found in scan " << scanNum << ": " << count << std::endl;

            // Save data to uniqueTags set
            for (size_t itemIndex = 0; itemIndex < count; itemIndex++)
            {
                // Create TagItem
                TagItem tagItem = reader->hm().tagItem(itemIndex);
                if (!tagItem.isValid()) { /* Add error handling */ cout << "Error: tag table empty"; return; }

                // Get IDD of TagItem
                string iddString = tagItem.iddToHexString();

                // Insert IDD to uniqueTags set
                uniqueTags.insert(iddString);
            }
        }

        // Output unique tags found
        // std::cout << "No. of unique tags found: " << uniqueTags.size() << std::endl;

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

                // save data to CSV file, with time in EST
                time_t now = time(0);
                tm *gmtm = gmtime(&now);
                gmtm = localtime(&now);
                file << *it << "," << asctime(gmtm) << endl;
                
            }

            // Close file
            file.close();
        }



        // TT
        string last_scanned_tag = "";
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = std::chrono::steady_clock::now();
        std::chrono::minutes run_duration(2);

        while (std::chrono::duration_cast<std::chrono::minutes>(end_time - start_time) < run_duration)
        {
            // sleep for 1 second
            std::this_thread::sleep_for(std::chrono::seconds(1));


            // Inventory_TagItem_withRSSI_Sample::run(*reader); //RSSI
            // Inventory_TagItem_Sample::run(*reader); //single TAG
            Inventory_Queue_Sample::run(*reader); //QUEUE

            // Update the end_time
            end_time = std::chrono::steady_clock::now();
        }

        // Disconnect Reader
        reader->disconnect();

        if (reader->lastError() == ErrorCode::Ok)
        {
            cout << "\n" << reader->info().readerTypeToString() << " disconnected." << endl;
        }
        delete reader;
        }
    }


    void onUsbEvent()
    {
        // Not used
    }
};

UsbManager UsbManagerConnect::usbManager;

int main()
{
    UsbManagerConnect::SampleMain();

    return 0;
}


// dont read same for 4 sec - edit this part in main code
// *********** Could try this piece in code *************
// void run(ReaderModule &reader)
// {
//     InventoryParam inventoryParam;
//     inventoryParam.setAntennas((uint8_t)0x0F);
//     reader.hm().setUsageMode(FEDM::Hm::UsageMode::UseQueue);

//     int state = reader.hm().inventory(true, inventoryParam);
//     cout << "inventory: " << reader.lastErrorStatusText() << endl;
//     if (state != ErrorCode::Ok) { /* Add error-handling... */ }

//     size_t count = reader.hm().queueItemCount();
//     cout << "No. of tags: " << count << endl;

//     static std::map<std::string, std::chrono::steady_clock::time_point> last_scanned;

//     while (reader.hm().queueItemCount() > 0)
//     {
//         unique_ptr<const TagItem> tagItem = reader.hm().popItem();
//         string iddString = tagItem->iddToHexString();

//         auto now = std::chrono::steady_clock::now();
//         auto it = last_scanned.find(iddString);
//         if (it != last_scanned.end())
//         {
//             // Skip the tag if it was read less than 4 seconds ago
//             if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count() < 4)
//             {
//                 continue;
//             }
//         }

//         last_scanned[iddString] = now;

//         // Rest of the logic

//         vector<RssiItem> rssiList = tagItem->rssiValues();
//         vector<RssiItem>::iterator itor;

//         for (RssiItem rssiItem : rssiList)
//         {
//             int antennaNumber = rssiItem.antennaNumber();
//             std::cout << "Ant: " << antennaNumber;
//         }
//     }

//     // Sleep for 2 seconds before the next scan
//     std::this_thread::sleep_for(std::chrono::seconds(2));
// }

