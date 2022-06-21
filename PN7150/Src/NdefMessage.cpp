#include <NdefMessage.h>

NdefMessage::NdefMessage(void)
{
    _recordCount = 0;
}

NdefMessage::NdefMessage(const uint8_t * data, const int numBytes)
{
    #ifdef NDEF_DEBUG
    Serial.print(F("Decoding "));Serial.print(numBytes);Serial.println(F(" bytes"));
    PrintHexChar(data, numBytes);
    //DumpHex(data, numBytes, 16);
    #endif

    _recordCount = 0;

    int index = 0;

    while (index <= numBytes)
    {

        // decode tnf - first byte is tnf with bit flags
        // see the NFDEF spec for more info
        uint8_t tnf_byte = data[index];
        bool mb = (tnf_byte & 0x80) != 0;
        bool me = (tnf_byte & 0x40) != 0;
        bool cf = (tnf_byte & 0x20) != 0;
        bool sr = (tnf_byte & 0x10) != 0;
        bool il = (tnf_byte & 0x8) != 0;
        uint8_t tnf = (tnf_byte & 0x7);

        NdefRecord record = NdefRecord();
        record.setTnf(tnf);

        index++;
        int typeLength = data[index];

        int payloadLength = 0;
        if (sr)
        {
            index++;
            payloadLength = data[index];
        }
        else
        {
            payloadLength =
		((0xFF & data[++index]) << 24)
		| ((0xFF & data[++index]) << 16)
		| ((0xFF & data[++index]) << 8)
		| (0xFF & data[++index]);
        }

        int idLength = 0;
        if (il)
        {
            index++;
            idLength = data[index];
        }

        index++;
        record.setType(&data[index], typeLength);
        index += typeLength;

        if (il)
        {
            record.setId(&data[index], idLength);
            index += idLength;
        }

        record.setPayload(&data[index], payloadLength);
        index += payloadLength;

        addRecord(record);

        if (me) break; // last message
    }

}

NdefMessage::NdefMessage(const NdefMessage& rhs)
{

    _recordCount = rhs._recordCount;
    for (int i = 0; i < _recordCount; i++)
    {
        _records[i] = rhs._records[i];
    }

}

NdefMessage::~NdefMessage()
{
}

NdefMessage& NdefMessage::operator=(const NdefMessage& rhs)
{

    if (this != &rhs)
    {

        // delete existing records
        for (int i = 0; i < _recordCount; i++)
        {
            // TODO Dave: is this the right way to delete existing records?
            _records[i] = NdefRecord();
        }

        _recordCount = rhs._recordCount;
        for (int i = 0; i < _recordCount; i++)
        {
            _records[i] = rhs._records[i];
        }
    }
    return *this;
}

unsigned int NdefMessage::getRecordCount()
{
    return _recordCount;
}

int NdefMessage::getEncodedSize()
{
    int size = 0;
    for (int i = 0; i < _recordCount; i++)
    {
        size += _records[i].getEncodedSize();
    }
    return size;
}

// TODO change this to return uint8_t*
void NdefMessage::encode(uint8_t* data)
{
    // assert sizeof(data) >= getEncodedSize()
    uint8_t* data_ptr = &data[0];

    for (int i = 0; i < _recordCount; i++)
    {
        _records[i].encode(data_ptr, i == 0, (i + 1) == _recordCount);
        // TODO can NdefRecord.encode return the record size?
        data_ptr += _records[i].getEncodedSize();
    }

}

bool NdefMessage::addRecord(NdefRecord& record)
{

    if (_recordCount < MAX_NDEF_RECORDS)
    {
        _records[_recordCount] = record;
        _recordCount++;
        return true;
    }
    else
    {
        printf("WARNING: Too many records. Increase MAX_NDEF_RECORDS.\n");
        return false;
    }
}

void NdefMessage::addMimeMediaRecord(std::string mimeType, std::string payload)
{
    addMimeMediaRecord(mimeType, (uint8_t *) payload.c_str(), payload.length());
}

void NdefMessage::addMimeMediaRecord(std::string mimeType, uint8_t* payload, int payloadLength)
{
    NdefRecord r = NdefRecord();
    r.setTnf(TNF_MIME_MEDIA);

    r.setType((uint8_t *)mimeType.c_str(), mimeType.length());

    r.setPayload(payload, payloadLength);

    addRecord(r);
}

void NdefMessage::addTextRecord(std::string text)
{
    addTextRecord(text, "en");
}

void NdefMessage::addTextRecord(std::string text, std::string encoding)
{
    NdefRecord r = NdefRecord();
    r.setTnf(TNF_WELL_KNOWN);

    uint8_t RTD_TEXT[1] = { 0x54 }; // TODO this should be a constant or preprocessor
    r.setType(RTD_TEXT, sizeof(RTD_TEXT));

    // X is a placeholder for encoding length
    // TODO is it more efficient to build w/o string concatenation?
    std::string payloadString = "X" + encoding + text;

    // replace X with the real encoding length
    payloadString[0] = encoding.length();

    r.setPayload((uint8_t*)payloadString.c_str(), payloadString.length());

    addRecord(r);
}

void NdefMessage::addUriRecord(std::string uri)
{
    NdefRecord* r = new NdefRecord();
    r->setTnf(TNF_WELL_KNOWN);

    uint8_t RTD_URI[1] = { 0x55 }; // TODO this should be a constant or preprocessor
    r->setType(RTD_URI, sizeof(RTD_URI));

    // X is a placeholder for identifier code
    std::string payloadString = "X" + uri;

    // add identifier code 0x0, meaning no prefix substitution
    payloadString[0] = 0x0;

    r->setPayload((uint8_t*)payloadString.c_str(), payloadString.length());

    addRecord(*r);
    delete(r);
}

void NdefMessage::addEmptyRecord()
{
    NdefRecord* r = new NdefRecord();
    r->setTnf(TNF_EMPTY);
    addRecord(*r);
    delete(r);
}

NdefRecord NdefMessage::getRecord(int index)
{
    if (index > -1 && index < _recordCount)
    {
        return _records[index];
    }
    else
    {
        return NdefRecord(); // would rather return NULL
    }
}

NdefRecord NdefMessage::operator[](int index)
{
    return getRecord(index);
}

void NdefMessage::print()
{
    printf("\nNDEF Message ");
    printf("%d", _recordCount);
    printf(" record");
    _recordCount == 1 ? printf(", ") : printf("s, ");
    printf("%d", getEncodedSize());
    printf(" bytes\n");

    int i;
    for (i = 0; i < _recordCount; i++)
    {
         _records[i].print();
    }
}
