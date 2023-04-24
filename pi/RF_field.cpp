static void rfOn(ReaderModule &reader)
{
    // [0x6A] RF output ON
    int antennaNumber = 0x01;
    bool maintainHostMode = false;
    bool dcOn = false;
    int state = reader.rf().on(antennaNumber, maintainHostMode, dcOn);
    cout << "RF On: " << reader.lastErrorStatusText() << endl;
    if (state != ErrorCode::Ok) { /* Add error-handling... */ }
}

static void rfOff(ReaderModule &reader)
{
    // [0x6A] RF output OFF
    int state = reader.rf().off();
    cout << "RF Off: " << reader.lastErrorStatusText() << endl;
    if (state != ErrorCode::Ok) { /* Add error-handling... */ }
}

static void rfReset(ReaderModule &reader)
{
    // [0x69] RF Reset
    int state = reader.rf().reset();
    cout << "RF Reset: " << reader.lastErrorStatusText() << endl;
    if (state != ErrorCode::Ok) { /* Add error-handling... */ }
}