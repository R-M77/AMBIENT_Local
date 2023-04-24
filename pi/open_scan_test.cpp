
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
#include <sys/wait.h>
#include <unistd.h>
#include "include/fedm/ReaderModule.h"
#include "include/fedm/UsbManager.h"

using namespace FEDM;
using namespace FEDM::TagHandler;
using namespace FEDM::Utility;
using namespace std;

class UsbManagerConnect : IUsbListener
{
public:
    static UsbManager usbManager;
    static string targetTagID;
    static string currentTagID;
    static bool foundTargetTag;

    static void SampleMain()
    {
        UsbManagerConnect usbManager;
        usbManager.Run();
    }

    static string executeCommand(const string& command)
    {
        string output;
        char buffer[128];
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe)
        {
            cerr << "Error: Unable to execute command: " << command << endl;
            return output;
        }
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            output += buffer;
        }
        pclose(pipe);
        return output;
    }



    void Run()
    {
        // Start USB Discover
        usbManager.startDiscover(this);
        cout << "Waiting for Connection with first USB Reader...." << endl;

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

            bool startRecording = true;
            // Perform scans until the target tag is found
            while (true) {
                // Inventory tags
                state = reader->hm().inventory();
                cout << "inventory: " << reader->lastErrorStatusText() << endl;
                if (state != ErrorCode::Ok) { /* Add error-handling... */ }

                // Get tag count
                size_t count = reader->hm().itemCount();

                // Output number of tags found
                std::cout << "No. of tags found: " << count << std::endl;

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

                // Save data to CSV file
                if (!uniqueTags.empty()) {
                    // Open file for appending
                    ofstream file;
                    file.open("tag_data.csv", std::ios::app);

                    // Create TagItem for each unique tag and output IDD
                    for (auto it = uniqueTags.begin(); it != uniqueTags.end(); ++it)
                    {
                        // Display IDD on screen
                        std::cout << "IDD: " << *it << std::endl;

                        // Save data to CSV file, with time in EST
                        time_t now = time(0);
                        tm* gmtm = gmtime(&now);
                        gmtm = localtime(&now);
                        file << *it << "," << asctime(gmtm) << endl;

                        // Check if the target tag is found or not
                        if (!foundTargetTag) {
                            // Set the current tag ID
                            currentTagID = *it;

                            // Set the target tag ID if this is the first tag found
                            if (targetTagID.empty()) {
                                targetTagID = currentTagID;
                                // Start recording on the Raspberry Pi cameras
                                if (startRecording)
                                {
                                    string command = "python3 camera_controller.py start " + currentTagID;
                                    executeCommand(command);
                                    startRecording = false;
                                }
                                // Wait for 5 seconds before the next scan
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                            }
                            // Check if the current tag ID matches the target tag ID
                            else if (currentTagID == targetTagID) {
                                // Stop recording on the Raspberry Pi cameras and send the video file to the PC
                                string command = "python3 camera_controller.py stop";
                                executeCommand(command);

                                // Set foundTargetTag to true
                                foundTargetTag = true;
                            }
                        }

                        // Close the file
                        file.close();

                        // Stop scanning if the target tag is found
                        if (foundTargetTag) {
                            // Disconnect the reader
                            reader->disconnect();
                            if (reader->lastError() == ErrorCode::Ok)
                            {
                                cout << "\n" << reader->info().readerTypeToString() << " disconnected." << endl;
                            }
                            delete reader;
                            return;
                        }
                    }

                    // Clear uniqueTags set for the next scan
                    uniqueTags.clear();

                    
                }
            }
        }
    }

    void onUsbEvent()
    {
        // Not used
    }
};

UsbManager UsbManagerConnect::usbManager;
string UsbManagerConnect::targetTagID;
string UsbManagerConnect::currentTagID;
bool UsbManagerConnect::foundTargetTag = false;

int main()
{
    UsbManagerConnect::SampleMain();

    return 0;
}

