#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <vector>
#include <set>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <sys/wait.h>
#include <unistd.h>
#include "include/fedm/ReaderModule.h"
#include "include/fedm/UsbManager.h"

using namespace FEDM;
using namespace FEDM::TagHandler;
using namespace FEDM::Utility;
using namespace std;

// Constants
const double timeoutDuration = 300.0; // Maximum recording duration (in seconds)
const double reentryDelayDuration = 5.0; // Minimum time between two consecutive entry detections (in seconds)
const int anomalyThreshold = 5; // Maximum number of detections within the reentryDelayDuration

// Map tag IDs to patients (replace this with actual mapping)
// map<string, string> tagToPatient = {
//     {"tagID1_A", "patient1"},
//     {"tagID1_B", "patient1"},
//     {"tagID2_A", "patient2"},
//     {"tagID2_B", "patient2"},
//     // Add more tags and patients as needed
// };


void executePythonScript(const std::vector<std::string>& args) {
    std::vector<char*> c_args;
    for (const auto& arg : args) {
        c_args.push_back(const_cast<char*>(arg.c_str()));
    }
    c_args.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        execvp("python3", c_args.data());
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork");
    }
}

void start_recording(const std::string& tagID) {
    executePythonScript({"python3", "camera_controller.py", "start", tagID});
}

void stop_recording() {
    executePythonScript({"python3", "camera_controller.py", "stop"});
}

std::string extractPatientID(const std::string& tagID) {
    // Extract the patient ID from the tag ID (assuming the patient ID is a prefix)
    // You should replace this with your specific extraction method
    return tagID.substr(0, 3); // Change the numbers according to the length of the patient ID prefix
}



std::map<std::string, std::pair<bool, std::chrono::steady_clock::time_point>> recordingStatus; // <tagID, <isRecording, lastExitTimestamp>>
std::map<std::string, std::chrono::steady_clock::time_point> lastSeen; // <tagID, lastSeenTimestamp>
std::map<std::string, int> detectionCount; // <tagID, detectionCount>
std::map<std::string, int> patientEntryCount; // <patient, entryCount>

class Inventory_TagItem_withRSSI_Sample 
{
public:
    static std::map<std::string, std::string> patientTags;


    static void run(ReaderModule &reader) {
        InventoryParam inventoryParam;

        // Set antenna to ANT1, ANT2, ANT3, and ANT4
        inventoryParam.setAntennas((uint8_t)0x0F);

        // Open file for appending
        ofstream file;
        file.open("tag_data_rssi.csv", std::ios::app);

        // Get start time
        auto startTime = std::chrono::steady_clock::now();

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
                if (!tagItem.isValid()) { /* Add error handling */ cout << "Error: tag table empty"; return; }
                // Output IDD of TagItem
                string iddString = tagItem.iddToHexString();
                std::cout << "IDD: " << iddString << std::endl;

                // Get patient associated with the tag
                // string patient = tagToPatient[iddString];

                // If tagID is new, initialize the patientTags map with the patient ID extracted from the tagID
                if (patientTags.find(iddString) == patientTags.end()) {
                    string patientID = extractPatientID(iddString); // Implement a function to extract the patient ID from the tag ID
                    patientTags[iddString] = patientID;
                }



                // Check if the tag ID is in the recordingStatus map
                auto it = recordingStatus.find(iddString);
                if (it == recordingStatus.end())
                {
                    // If not in the map, add it and start recording
                    recordingStatus[iddString] = make_pair(true, currentTime);
                    lastSeen[iddString] = currentTime;
                    detectionCount[iddString] = 1;
                    patientEntryCount[patient]++;
                }
                else
                {
                    auto elapsedSinceLastSeen = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastSeen[iddString]).count();

                    if (elapsedSinceLastSeen >= reentryDelayDuration) {
                        detectionCount[iddString] = 1;
                    } 
                    else {
                        detectionCount[iddString]++;
                    }

                    if (detectionCount[iddString] <= anomalyThreshold) {
                        if (recordingStatus[iddString].first) {
                            // If in the map and recording is ongoing, check for timeout and stop recording if necessary
                            auto elapsedSinceStart = std::chrono::duration_cast<std::chrono::seconds>(currentTime - recordingStatus[iddString].second).count();
                            if (elapsedSinceStart >= timeoutDuration)
                            {
                                recordingStatus[iddString].first = false;
                                stop_recording();
                            }
                        }
                    }
                    else
                    {
                        // If the detection count is within the threshold, update the last seen time
                        lastSeen[iddString] = currentTime;

                        // Check if the patient has exited (if not standing in front of the entry sensor)
                        if (patientEntryCount[patient] % 2 == 0)
                        {
                            // Count as an exit, stop recording
                            recordingStatus[iddString].first = false;
                            recordingStatus[iddString].second = currentTime;
                            stop_recording();
                            patientEntryCount[patient]++;
                        }
                        else
                        {
                            // Count as an entry, start recording
                            recordingStatus[iddString].first = true;
                            recordingStatus[iddString].second = currentTime;
                            start_recording(iddString);
                            patientEntryCount[patient]++;
                        }
                    }
                }

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

        // identify tag type using tagHandler_EPC_Sample
        // Inventory_TagHandler_Sample::tagHandler_EPC_Sample(*reader);
        // Inventory_TagItem_withRSSI_Sample::run(*reader);

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

