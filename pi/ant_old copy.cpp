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

void start_recording(string tagID, int tag_scan_count) {
    if (tag_scan_count == 1) {
        // Add the tag ID to the recordingTags list
        recordingTags.push_back(tagID);
        // Set the start time for the tag
        tag_start_times[tagID] = chrono::steady_clock::now();
        // Start recording for both cameras
        string cmd1 = "ssh pi@" + camera_pi1 + " raspivid -o " + tagID + "_cam1.h264 -t 40000";
        string cmd2 = "ssh pi@" + camera_pi2 + " raspivid -o " + tagID + "_cam2.h264 -t 40000";
        system(cmd1.c_str());
        system(cmd2.c_str());
        cout << "Started recording for tag " << tagID << endl;
    }
}

void stop_recording(string tagID, int tag_scan_count) {
    if (tag_scan_count == 2) {
        // Remove the tag ID from the recordingTags list
        recordingTags.remove(tagID);
        // Stop recording for both cameras
        string cmd = "ssh pi@" + camera_pi1 + " pkill raspivid; ssh pi@" + camera_pi2 + " pkill raspivid";
        system(cmd.c_str());
        // Send the video files to the PC
        send_to_PC(tagID);
        cout << "Stopped recording for tag " << tagID << endl;
    }
}


// void start_recording(const std::string &tagID)
// {
//     std::string command = "python3 vid_old.py start " + tagID;
//     system(command.c_str());
// }

// void stop_recording(const std::string &tagID)
// {
//     std::string command = "python3 vid_old.py stop " + tagID;
//     system(command.c_str());
// }



// vector<string> tagList;
// void start_recording(const std::string &tagID)
// {
//     std::string command = "python3 vid_old.py start " + tagID;
//     system(command.c_str());
// }

// void stop_recording(const std::string &tagID)
// {
//     std::string command = "python3 vid_old.py stop" + tagID;
//     system(command.c_str());
// }

std::map<std::string, bool> recordingStatus;


class Inventory_TagItem_withRSSI_Sample
{
public:
    static void run(ReaderModule &reader)
    {
        InventoryParam inventoryParam;

        // Set antenna to ANT1, ANT2, ANT3, and ANT4
        inventoryParam.setAntennas((uint8_t)0x0F);

        // Open file for appending
        ofstream file;
        file.open("tag_data_rssi.csv", std::ios::app);

        // Get start time
        auto startTime = std::chrono::steady_clock::now();
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsedSeconds;

        // Add a deque to store the order of unique scanned tags
        std::deque<string> scannedTagsOrder;
        

        // Loop for 180 seconds
        while (elapsedSeconds.count() < 180.0)
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
                if (!tagItem.isValid()) { /* Add error handling */ cout << "Error: tag table empty"; return; }
                // Output IDD of TagItem
                string iddString = tagItem.iddToHexString();
                std::cout << "IDD: " << iddString << std::endl;

                // Check if the tag ID is in the recordingStatus map
                auto it = recordingStatus.find(iddString);

                if (it == recordingStatus.end())
                {
                    // If not in the map, add it and start recording
                    recordingStatus[iddString] = true;
                    start_recording(iddString);

                    // Add the tag to the deque to remember the order
                    scannedTagsOrder.push_back(iddString);
                }
                else
                {
                    // If in the map and recording is ongoing,
                    // stop recording when the same unique tag is scanned for the second time
                    if (recordingStatus[iddString])
                    {
                        recordingStatus[iddString] = false;
                        stop_recording(iddString);
                    }
                    // If in the map and recording is not ongoing, start recording
                    else
                    {
                        recordingStatus[iddString] = true;
                        start_recording(iddString);
                    }
                }

                // // Check if the tag ID is in the recordingStatus map
                // auto it = recordingStatus.find(iddString);

                // std::cout << "recording status_: " << recordingStatus[iddString] << std::endl;
                // // std::cout << "rr" << it << std::endl;
                
                // if (it == recordingStatus.end())
                // {
                //     // If not in the map, add it and start recording
                //     recordingStatus[iddString] = true;
                //     start_recording(iddString);
                // }
                // else
                // {
                //     // If in the map and recording is ongoing, stop recording
                //     if (recordingStatus[iddString])
                //     {
                //         recordingStatus[iddString] = false;
                //         stop_recording(iddString);
                //     }
                //     else
                //     {
                //         // If in the map and recording is not ongoing, start recording
                //         recordingStatus[iddString] = true;
                //         start_recording(iddString);
                //     }
                // }

                // Create List for RSSI values of TagItem
                vector<RssiItem> rssiListe = tagItem.rssiValues();
                vector<RssiItem>::iterator itor;

                for (RssiItem rssiItem : rssiListe)
                {
                    // Antenna Number
                    int antennaNumber = rssiItem.antennaNumber();
                    std::cout << "Ant: " << antennaNumber;

                    // Save tagID and antenna number to CSV file
                    file << "TagID:" << iddString << ", ANT:" << antennaNumber << std::endl;
                }
            }

        //     // Update elapsed time
        //     currentTime = std::chrono::steady_clock::now();
        //     elapsedSeconds = currentTime - startTime;
        // }

        // // Get start time
        // auto startTime = std::chrono::steady_clock::now();
        // auto currentTime = std::chrono::steady_clock::now();
        // std::chrono::duration<double> elapsedSeconds;

        // // Add a deque to store the order of unique scanned tags
        // std::deque<string> scannedTagsOrder;

        // // Add a dictionary to keep track of the scan count for each tag
        // std::unordered_map<string, int> tagScanCount;

        // // Add a dictionary to keep track of the time of the last scan for each tag
        // std::unordered_map<string, std::chrono::steady_clock::time_point> tagLastScanTime;

        // // Loop for 180 seconds
        // while (elapsedSeconds.count() < 180.0)
        // {
        //     int state = reader.hm().inventory(true, inventoryParam);
        //     if (state != ErrorCode::Ok) { /* Add error-handling... */ }

        //     // Number of read tags
        //     size_t count = reader.hm().itemCount();

        //     // Create TagItem for each tag and output IDD and RSSI values
        //     for (size_t itemIndex = 0; itemIndex < count; itemIndex++)
        //     {
        //         // Create TagItem
        //         TagItem tagItem = reader.hm().tagItem(itemIndex);
        //         if (!tagItem.isValid()) { /* Add error handling */ cout << "Error: tag table empty"; return; }
        //         // Output IDD of TagItem
        //         string iddString = tagItem.iddToHexString();
        //         std::cout << "IDD: " << iddString << std::endl;

        //         // Check if the tag ID is in the recordingStatus map
        //         auto it = tagScanCount.find(iddString);

        //         if (it == tagScanCount.end())
        //         {
        //             // If not in the map, add it and start recording
        //             tagScanCount[iddString] = 1;
        //             tagLastScanTime[iddString] = currentTime;
        //             start_recording(iddString);

        //             // Add the tag to the deque to remember the order
        //             scannedTagsOrder.push_back(iddString);
        //         }
        //         else
        //         {
        //             // If in the map and the last scan was less than 4 seconds ago, don't do anything
        //             if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - tagLastScanTime[iddString]).count() < 4)
        //             {
        //                 continue;
        //             }
        //             // If in the map and recording is ongoing,
        //             // stop recording when the same unique tag is scanned for the second time
        //             else if (tagScanCount[iddString] == 2)
        //             {
        //                 tagScanCount[iddString] = 0;
        //                 stop_recording(iddString);
        //             }
        //             // If in the map and recording is not ongoing, start recording
        //             else
        //             {
        //                 tagScanCount[iddString]++;
        //                 tagLastScanTime[iddString] = currentTime;
        //                 start_recording(iddString);
        //             }
        //         }

        //         // Create List for RSSI values of TagItem
        //         vector<RssiItem> rssiListe = tagItem.rssiValues();
        //         vector<RssiItem>::iterator itor;

        //         for (RssiItem rssiItem : rssiListe)
        //         {
        //             // Antenna Number
        //             int antennaNumber = rssiItem.antennaNumber();
        //             std::cout << "Ant: " << antennaNumber;

        //             // Save tagID and antenna number to CSV file
        //             file << "TagID:" << iddString << ", ANT:" << antennaNumber << std::endl;
        //         }
        //     }

            // Update elapsed time
            currentTime = std::chrono::steady_clock::now();
            elapsedSeconds = currentTime - startTime;
        }





        // Close file
        file.close();
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
                if (!tagItem.isValid()) { /* Add error handling */ cout << "Error: tag table empty"; return; }

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

                // save data to CSV file, with time in EST
                time_t now = time(0);
                tm *gmtm = gmtime(&now);
                gmtm = localtime(&now);
                file << *it << "," << asctime(gmtm) << endl;
                
            }

            // Close file
            file.close();
        }


        // TTTT
        // *****************
        // CAMERA TEST
        string last_scanned_tag = "";
        auto start_time = std::chrono::steady_clock::now();

        while (true)
        {
            Inventory_TagItem_withRSSI_Sample::run(*reader);
           
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





// #include <iostream>
// #include <fstream>
// #include <sstream>
// #include <string>
// #include <string.h>
// #include <vector>
// #include <set>
// #include <chrono>
// #include <thread>
// #include <list>
// #include <unordered_map>
// #include <deque>
// #include <cstdlib>
// #include <sys/wait.h>
// #include <unistd.h>
// #include "include/fedm/ReaderModule.h"
// #include "include/fedm/UsbManager.h"

// using namespace FEDM;
// using namespace FEDM::TagHandler;
// using namespace FEDM::Utility;
// using namespace std;

// std::unordered_map<string, std::chrono::steady_clock::time_point> tagLastScanTime;

// std::list<std::string> recordingTags;
// std::unordered_map<std::string, std::chrono::steady_clock::time_point> tag_start_times;

// #include <ctime> // Include ctime library for strftime function

// void start_recording(string tagID) {
//     string camera_pi1 = "10.42.0.57";
//     string camera_pi2 = "10.42.0.197";
    
//     // Get current date and time for file naming
//     time_t now = time(0);
//     struct tm* local_time = localtime(&now);
//     char time_str[80];
//     strftime(time_str, 80, "%Y%m%d_%H%M%S", local_time);

//     // Add the tag ID to the recordingTags list
//     recordingTags.push_back(tagID);
//     // Set the start time for the tag
//     tag_start_times[tagID] = chrono::steady_clock::now();
//     // Start recording for both cameras
//     string cmd1 = "ssh pi@" + camera_pi1 + " raspivid -o " + tagID + "_" + time_str + "_cam1.h264 -t 40000";
//     string cmd2 = "ssh pi@" + camera_pi2 + " raspivid -o " + tagID + "_" + time_str + "_cam2.h264 -t 40000";
//     system(cmd1.c_str());
//     system(cmd2.c_str());
//     cout << "Started recording for tag " << tagID << endl;
// }


// int send_to_PC(const string& tagID) {
//     const string camera_pi1 = "10.42.0.57";
//     const string camera_pi2 = "10.42.0.197";
//     const string pc_addr = "10.42.0.1:/home/rm/";

//     // Get the current time
//     const auto currentTime = chrono::system_clock::now();
//     const time_t now_c = chrono::system_clock::to_time_t(currentTime);
//     const tm now_tm = *localtime(&now_c);

//     // Get the start time for the tag
//     const auto startTime = tag_start_times[tagID];

//     // Calculate the duration of the recording
//     const chrono::duration<double> elapsedSeconds = currentTime - startTime;
//     const int duration = elapsedSeconds.count() * 1000;

//     // Generate the video file names
//     char cam1_file[50], cam2_file[50];
//     strftime(cam1_file, 50, "%Y%m%d-%H%M%S", &now_tm);
//     strftime(cam2_file, 50, "%Y%m%d-%H%M%S", &now_tm);

//     const string file1 = tagID + "_" + cam1_file + "_cam1_" + to_string(duration) + ".h264";
//     const string file2 = tagID + "_" + cam2_file + "_cam2_" + to_string(duration) + ".h264";

//     // Send the video files to the PC
//     try {
//         const string cmd1 = "ssh pi@" + camera_pi1 + " scp " + tagID + "_cam1.h264" + " rm@" + pc_addr + file1;
//         const string cmd2 = "ssh pi@" + camera_pi2 + " scp " + tagID + "_cam2.h264" + " rm@" + pc_addr + file2;

//         if (system(cmd1.c_str()) != 0 || system(cmd2.c_str()) != 0) {
//             cerr << "Error: failed to send video files to PC" << endl;
//             return -1;
//         }

//         cout << "Video files for tag " << tagID << " sent to PC." << endl;
//         return 0;
//     }
//     catch (const std::exception& ex) {
//         cerr << "Exception while sending video files to PC: " << ex.what() << endl;
//         return -1;
//     }
// }


// void stop_recording(string tagID) {
//     string camera_pi1 = "10.42.0.57";
//     string camera_pi2 = "10.42.0.197";
    
//     // Remove the tag ID from the recordingTags list
//     recordingTags.remove(tagID);
//     // Stop recording for both cameras
//     string cmd = "ssh pi@" + camera_pi1 + " pkill raspivid; ssh pi@" + camera_pi2 + " pkill raspivid";
//     system(cmd.c_str());
//     // Send the video files to the PC
//     // send_to_PC(tagID);
//     cout << "Stopped recording for tag " << tagID << endl;
// }





// std::map<std::string, bool> recordingStatus;


// class Inventory_TagItem_withRSSI_Sample
// {
// public:
//     static void run(ReaderModule &reader)
//     {
//         InventoryParam inventoryParam;

//         // Set antenna to ANT1, ANT2, ANT3, and ANT4
//         inventoryParam.setAntennas((uint8_t)0x0F);

//         // Open file for appending
//         ofstream file;
//         file.open("tag_data_rssi.csv", std::ios::app);

//         // Get start time
//         auto startTime = std::chrono::steady_clock::now();
//         auto currentTime = std::chrono::steady_clock::now();
//         std::chrono::duration<double> elapsedSeconds;

//         // Add a deque to store the order of unique scanned tags
//         std::deque<string> scannedTagsOrder;

//         // Add a dictionary to keep track of the scan count for each tag
//         std::unordered_map<string, int> tagScanCount;

//         // Add a dictionary to keep track of the time of the last scan for each tag
//         std::unordered_map<string, std::chrono::steady_clock::time_point> tagLastScanTime;

//         // Loop for 180 seconds
//         while (elapsedSeconds.count() < 180.0)
//         {
//             int state = reader.hm().inventory(true, inventoryParam);
//             if (state != ErrorCode::Ok) { /* Add error-handling... */ }

//             // Number of read tags
//             size_t count = reader.hm().itemCount();

//             // Create TagItem for each tag and output IDD and RSSI values
//             for (size_t itemIndex = 0; itemIndex < count; itemIndex++)
//             {
//                 // Create TagItem
//                 TagItem tagItem = reader.hm().tagItem(itemIndex);
//                 if (!tagItem.isValid()) { /* Add error handling */ cout << "Error: tag table empty"; return; }
//                 // Output IDD of TagItem
//                 string iddString = tagItem.iddToHexString();
//                 std::cout << "IDD: " << iddString << std::endl;

//                 // Check if the tag ID is in the recordingStatus map
//                 auto it = tagScanCount.find(iddString);

//                 if (it == tagScanCount.end())
//                 {
//                     // If not in the map, add it and start recording
//                     tagScanCount[iddString] = 1;
//                     tagLastScanTime[iddString] = currentTime;
//                     start_recording(iddString);

//                     // Add the tag to the deque to remember the order
//                     scannedTagsOrder.push_back(iddString);
//                 }
//                 else
//                 {
//                     // If in the map and the last scan was less than 4 seconds ago, don't do anything
//                     if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - tagLastScanTime[iddString]).count() < 4)
//                     {
//                         continue;
//                     }
//                     // If in the map and recording is ongoing,
//                     // stop recording when the same unique tag is scanned for the second time
//                     else if (tagScanCount[iddString] == 2)
//                     {
//                         tagScanCount[iddString] = 0;
//                         stop_recording(iddString);
//                     }
//                     // If in the map and recording is not ongoing, start recording
//                     else
//                     {
//                         tagScanCount[iddString]++;
//                         tagLastScanTime[iddString] = currentTime;
//                         start_recording(iddString);
//                     }
//                 }

//                 // Create List for RSSI values of TagItem
//                 vector<RssiItem> rssiListe = tagItem.rssiValues();
//                 vector<RssiItem>::iterator itor;

//                 for (RssiItem rssiItem : rssiListe)
//                 {
//                     // Antenna Number
//                     int antennaNumber = rssiItem.antennaNumber();
//                     std::cout << "Ant: " << antennaNumber;

//                     // Save tagID and antenna number to CSV file
//                     file << "TagID:" << iddString << ", ANT:" << antennaNumber << std::endl;
//                 }
//             }

//             // Update elapsed time
//             currentTime = std::chrono::steady_clock::now();
//             elapsedSeconds = currentTime - startTime;
//         }
//         // Close file
//         file.close();
//     }

// };


// class UsbManagerConnect : IUsbListener
// {
// public:
//     static UsbManager usbManager;

//     static void SampleMain()
//     {
//         UsbManagerConnect usbManager;
//         usbManager.Run();
//     }

//     void Run()
// {
//     // Start USB Discover
//     usbManager.startDiscover(this);
//     cout << "Waiting for Connection with first USB Reader...." << endl;;

//     // Wait for first BLE device
//     while (usbManager.readerCount() == 0)
//     {
//     }

//     // Get the Connector by BLE Manager
//     UsbScanInfo usbScanInfo = usbManager.popDiscover();
//     usbManager.stopDiscover();

//     if (!usbScanInfo.isValid())
//     {
//         cout << "Error: Invalid usb scan info" << endl;
//         return;
//     }

//     Connector connector = usbScanInfo.connector();
//     if (connector.isValid())
//     {
//         // Create Reader Module with Request Mode UniDirectional = Advanced Protocol
//         ReaderModule* reader = new ReaderModule(RequestMode::UniDirectional);
//         cout << "Start connection with Reader: " << usbScanInfo.readerTypeToString() << " (" << connector.usbDeviceIdToHexString() << ")..." << endl;
//         // Connect
//         int state = reader->connect(connector);

//         cout << "Connect: " << reader->lastErrorStatusText() << endl;
//         // Error handling
//         if (state != ErrorCode::Ok)
//         {
//             return;
//         }

//         set<string> uniqueTags;

//         // config antennas
        
//         int scanNum = 0;
//         // perform scans
//         while (uniqueTags.size() == 0) {
//             scanNum++;
//             // Inventory tags
//             state = reader->hm().inventory();
//             cout << "inventory: " << reader->lastErrorStatusText() << endl;
//             if (state != ErrorCode::Ok) { /* Add error-handling... */ }

//             // Get tag count
//             size_t count = reader->hm().itemCount();

//             // Output number of tags found
//             std::cout << "No. of tags found in scan " << scanNum << ": " << count << std::endl;

//             // Save data to uniqueTags set
//             for (size_t itemIndex = 0; itemIndex < count; itemIndex++)
//             {
//                 // Create TagItem
//                 TagItem tagItem = reader->hm().tagItem(itemIndex);
//                 if (!tagItem.isValid()) { /* Add error handling */ cout << "Error: tag table empty"; return; }

//                 // Get IDD of TagItem
//                 string iddString = tagItem.iddToHexString();

//                 // Insert IDD to uniqueTags set
//                 uniqueTags.insert(iddString);
//             }
//         }

//         // Output unique tags found
//         std::cout << "No. of unique tags found: " << uniqueTags.size() << std::endl;

//         // Save data to CSV file
//         if (!uniqueTags.empty()) {
//             // Open file for appending
//             ofstream file;
//             file.open("tag_data_rssi.csv", std::ios::app);

//             // Create TagItem for each unique tag and output IDD
//             for (auto it = uniqueTags.begin(); it != uniqueTags.end(); ++it)
//             {
//                 // Display IDD on screen
//                 std::cout << "IDD: " << *it << std::endl;

//                 // save data to CSV file, with time in EST
//                 time_t now = time(0);
//                 tm *gmtm = gmtime(&now);
//                 gmtm = localtime(&now);
//                 file << *it << "," << asctime(gmtm) << endl;
                
//             }

//             // Close file
//             file.close();
//         }


//         // TTTT
//         // *****************
//         // CAMERA TEST
//         string last_scanned_tag = "";
//         auto start_time = std::chrono::steady_clock::now();

//         while (true)
//         {
//             Inventory_TagItem_withRSSI_Sample::run(*reader);
           
//         }


//         // Disconnect Reader
//         reader->disconnect();

//         if (reader->lastError() == ErrorCode::Ok)
//         {
//             cout << "\n" << reader->info().readerTypeToString() << " disconnected." << endl;
//         }
//         delete reader;
//         }
//     }


//     void onUsbEvent()
//     {
//         // Not used
//     }
// };

// UsbManager UsbManagerConnect::usbManager;

// int main()
// {
//     UsbManagerConnect::SampleMain();
//     // Add a timer to terminate the program after 3 hours
//     // std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now() + std::chrono::minutes(4);
//     // while (std::chrono::system_clock::now() < endTime)
//     // {
//     //     std::this_thread::sleep_for(std::chrono::seconds(1));
//     // }

//     // wait 3 minutes
//     std::this_thread::sleep_for(std::chrono::minutes(3));
    
//     // Exit the program
//     exit(0);
//     return 0;
// }