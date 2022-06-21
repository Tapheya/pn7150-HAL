#ifndef NdefMessage_h
#define NdefMessage_h

#include <string>
#include <Ndef.h>
#include <NdefRecord.h>

#define MAX_NDEF_RECORDS 4

class NdefMessage
{
    public:
        NdefMessage(void);
        NdefMessage(const uint8_t *data, const int numBytes);
        NdefMessage(const NdefMessage& rhs);
        ~NdefMessage();
        NdefMessage& operator=(const NdefMessage& rhs);

        int getEncodedSize(); // need so we can pass array to encode
        void encode(uint8_t *data);

        bool addRecord(NdefRecord& record);
        void addMimeMediaRecord(std::string mimeType, std::string payload);
        void addMimeMediaRecord(std::string mimeType, uint8_t *payload, int payloadLength);
        void addTextRecord(std::string text);
        void addTextRecord(std::string text, std::string encoding);
        void addUriRecord(std::string uri);
        void addEmptyRecord();

        unsigned int getRecordCount();
        NdefRecord getRecord(int index);
        NdefRecord operator[](int index);

        void print();
    private:
        NdefRecord _records[MAX_NDEF_RECORDS];
        unsigned int _recordCount;
};

#endif