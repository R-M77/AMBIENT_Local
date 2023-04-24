#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <thread>
#include <chrono>
#include <map>
#include <unordered_map>
#include <FedmTagItem.h>
#include <vector>
#include <set>
// #include <Python.h>

#include "include/fedm/ReaderModule.h"
#include "include/fedm/UsbManager.h"
#include "include/fedm/FedmConst.h"

using namespace FEDM;
using namespace FEDM::TagHandler;
using namespace FEDM::Utility;
using namespace std;

class Inventory_TagHandler
{
public:
    static void tagHandler_EPC(ReaderModule &reader)
    {
        reader.hm().setUsageMode(FEDM::Hm::UsageMode::UseQueue);

        int state = reader.hm().inventory();
        cout << "inventory: " << reader.lastErrorStatusText() << endl;
        if (state != ErrorCode::Ok) { /* Add error-handling... */ }

        // Number of read tags
        size_t count = reader.hm().queueItemCount();
        cout << "No. of tags: " << count << endl;

        // Create TagItem for each tag and output IDD
        while (reader.hm().queueItemCount() > 0)
        {
            unique_ptr <const TagItem> tagItem = reader.hm().popItem();
            // cout << "Tag " << tagItem->iddToHexString() << " is declared: " << tagItem->transponderName() << endl;

            // Check if this tag has been seen before
            string tagID = tagItem->iddToHexString();
            if (uniqueTags.count(tagID) == 0)
            {
                // This is a new tag
                uniqueTags.insert(tagID);
                tagTimestamps[tagID] = time(nullptr);
                // tagAntennas[tagID] = tagItem->lastAntennaNumber();

                // Trigger camera recording
                // replace this with the appropriate commands for your Pi camera setup
                // system("ssh user@host 'python3 start_recording.py'");
                // run_python("start_recording.py", "");


                // run_python("camera_controller.py", "start " + tagID);

            }
            else
            {
                // This tag has been seen before
                // Check if the tag has moved to a different antenna
                // int currentAntenna = tagItem->lastAntennaNumber();
                // if (currentAntenna != tagAntennas[tagID])
                // {
                //     // The tag has moved to a different antenna
                //     tagAntennas[tagID] = currentAntenna;
                //     tagTimestamps[tagID] = time(nullptr);

                //     // Stop camera recording and save the video
                //     // replace this with the appropriate commands for your Pi camera setup
                //     ostringstream filename;
                //     filename << tagID << "_" << getDateTimeString() << ".h264";
                //     // string command = "ssh user@host 'python3 stop_recording.py " + filename.str() + "'";
                //     // system(command.c_str());


                //     // run_python("camera_controller.py", "stop " + tagID);

                // }
            }
        }

        // Check if any tags have stopped moving and stop recording
        time_t currentTime = time(nullptr);
        for (auto it = tagTimestamps.begin(); it != tagTimestamps.end(); ++it)
        {
            if (currentTime - it->second >= TIMEOUT_SECONDS)
            {
                // This tag hasn't moved for a while, stop recording
                // replace this with the appropriate commands for your Pi camera setup
                ostringstream filename;
                filename << it->first << "_" << getDateTimeString() << ".h264";
                // string command = "ssh user@host 'python3 stop_recording.py " + filename.str() + "'";


                // run_python("stop_recording.py", filename.str());
                // system(command.c_str());

                // Remove the tag from the list
                uniqueTags.erase(it->first);
                // tagTimestamps
                tagAntennas.erase(it->first);
                tagTimestamps.erase(it);
            }
        }
    }

    static string getDateTimeString()
    {
        time_t now = time(nullptr);
        struct tm tstruct;
        char buf[80];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%m%d%Y_%H%M", &tstruct);
        return buf;
    }

private:
    static set<string> uniqueTags;
    static unordered_map<string, time_t> tagTimestamps;
    static unordered_map<string, int> tagAntennas;
    static const int TIMEOUT_SECONDS = 5;
};

set<string> Inventory_TagHandler::uniqueTags;
unordered_map<string, time_t> Inventory_TagHandler::tagTimestamps;
unordered_map<string, int> Inventory_TagHandler::tagAntennas;

class UsbManagerConnect : IUsbListener
{
public:
    static UsbManager usbManager;

    static void StartMain()
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

            // Set antenna to ANT1 and ANT2
            // Antenna must be set to set mode to "Request with antenna number" for RSSI
            InventoryParam inventoryParam;
            inventoryParam.setAntennas((uint8_t)0x03);

            // Loop continuously and handle RFID tag scanning and camera recording
            while (true)
            {
                // Inventory tags
                state = reader->hm().inventory(true, inventoryParam);
                cout << "inventory: " << reader->lastErrorStatusText() << endl;
                if (state != ErrorCode::Ok) { /* Add error-handling... */ }

                // Call tagHandler to check if any new tags have been detected or if any tags have stopped moving
                Inventory_TagHandler::tagHandler_EPC(*reader);

                // Sleep for a short period to reduce CPU usage
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

// // function to run python scripts from C++
// void run_python(const std::string &script_name, const std::string &arg = "")
// {
//     Py_Initialize();

//     PyObject *pName = PyUnicode_DecodeFSDefault(script_name.c_str());
//     PyObject *pModule = PyImport_Import(pName);
//     Py_DECREF(pName);

//     if (pModule != nullptr)
//     {
//         PyObject *pFunc = PyObject_GetAttrString(pModule, "main");

//         if (pFunc && PyCallable_Check(pFunc))
//         {
//             PyObject *pArgs = PyTuple_New(arg.empty() ? 0 : 1);
//             if (!arg.empty())
//             {
//                 PyObject *pValue = PyUnicode_FromString(arg.c_str());
//                 PyTuple_SetItem(pArgs, 0, pValue);
//             }

//             PyObject *pRetVal = PyObject_CallObject(pFunc, pArgs);
//             Py_DECREF(pArgs);
//             Py_XDECREF(pRetVal);
//         }
//         Py_XDECREF(pFunc);
//         Py_DECREF(pModule);
//     }
//     else
//     {
//         PyErr_Print();
//         fprintf(stderr, "Failed to load \"%s\"\n", script_name.c_str());
//     }

//     Py_Finalize();
// }

UsbManager UsbManagerConnect::usbManager;

// UsbManager UsbManagerConnect
// UsbManagerConnect::usbManager;

int main()
{
    // Start the program
    UsbManagerConnect::StartMain();

    return 0;
}
