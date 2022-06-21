#include <cstring>
#include "NdefRecord.h"

NdefRecord::NdefRecord()
{
    //Serial.println("NdefRecord Constructor 1");
    _tnf = 0;
    _typeLength = 0;
    _payloadLength = 0;
    _idLength = 0;
    _type = (uint8_t *)nullptr;
    _payload = (uint8_t *)nullptr;
    _id = (uint8_t *)nullptr;
}

NdefRecord::NdefRecord(const NdefRecord& rhs)
{
    //Serial.println("NdefRecord Constructor 2 (copy)");

    _tnf = rhs._tnf;
    _typeLength = rhs._typeLength;
    _payloadLength = rhs._payloadLength;
    _idLength = rhs._idLength;
    _type = (uint8_t *)nullptr;
    _payload = (uint8_t *)nullptr;
    _id = (uint8_t *)NULL;

    if (_typeLength)
    {
        _type = (uint8_t*)malloc(_typeLength);
        memcpy(_type, rhs._type, _typeLength);
    }

    if (_payloadLength)
    {
        _payload = (uint8_t*)malloc(_payloadLength);
        memcpy(_payload, rhs._payload, _payloadLength);
    }

    if (_idLength)
    {
        _id = (uint8_t*)malloc(_idLength);
        memcpy(_id, rhs._id, _idLength);
    }

}

// TODO NdefRecord::NdefRecord(tnf, type, payload, id)

NdefRecord::~NdefRecord()
{
    //Serial.println("NdefRecord Destructor");
    if (_typeLength)
    {
        free(_type);
    }

    if (_payloadLength)
    {
        free(_payload);
    }

    if (_idLength)
    {
        free(_id);
    }
}

NdefRecord& NdefRecord::operator=(const NdefRecord& rhs)
{
    //Serial.println("NdefRecord ASSIGN");

    if (this != &rhs)
    {
        // free existing
        if (_typeLength)
        {
            free(_type);
        }

        if (_payloadLength)
        {
            free(_payload);
        }

        if (_idLength)
        {
            free(_id);
        }

        _tnf = rhs._tnf;
        _typeLength = rhs._typeLength;
        _payloadLength = rhs._payloadLength;
        _idLength = rhs._idLength;

        if (_typeLength)
        {
            _type = (uint8_t*)malloc(_typeLength);
            memcpy(_type, rhs._type, _typeLength);
        }

        if (_payloadLength)
        {
            _payload = (uint8_t*)malloc(_payloadLength);
            memcpy(_payload, rhs._payload, _payloadLength);
        }

        if (_idLength)
        {
            _id = (uint8_t*)malloc(_idLength);
            memcpy(_id, rhs._id, _idLength);
        }
    }
    return *this;
}

// size of records in bytes
int NdefRecord::getEncodedSize()
{
    int size = 2; // tnf + typeLength
    if (_payloadLength > 0xFF)
    {
        size += 4;
    }
    else
    {
        size += 1;
    }

    if (_idLength)
    {
        size += 1;
    }

    size += (_typeLength + _payloadLength + _idLength);

    return size;
}

void NdefRecord::encode(uint8_t *data, bool firstRecord, bool lastRecord)
{
    // assert data > getEncodedSize()

    uint8_t* data_ptr = &data[0];

    *data_ptr = getTnfByte(firstRecord, lastRecord);
    data_ptr += 1;

    *data_ptr = _typeLength;
    data_ptr += 1;

    if (_payloadLength <= 0xFF) {  // short record
        *data_ptr = _payloadLength;
        data_ptr += 1;
    } else { // long format
        // 4 bytes but we store length as an int
        data_ptr[0] = 0x0; // (_payloadLength >> 24) & 0xFF;
        data_ptr[1] = 0x0; // (_payloadLength >> 16) & 0xFF;
        data_ptr[2] = (_payloadLength >> 8) & 0xFF;
        data_ptr[3] = _payloadLength & 0xFF;
        data_ptr += 4;
    }

    if (_idLength)
    {
        *data_ptr = _idLength;
        data_ptr += 1;
    }

    //Serial.println(2);
    memcpy(data_ptr, _type, _typeLength);
    data_ptr += _typeLength;

    memcpy(data_ptr, _payload, _payloadLength);
    data_ptr += _payloadLength;

    if (_idLength)
    {
        memcpy(data_ptr, _id, _idLength);
        data_ptr += _idLength;
    }
}

uint8_t NdefRecord::getTnfByte(bool firstRecord, bool lastRecord)
{
    int value = _tnf;

    if (firstRecord) { // mb
        value = value | 0x80;
    }

    if (lastRecord) { //
        value = value | 0x40;
    }

    // chunked flag is always false for now
    // if (cf) {
    //     value = value | 0x20;
    // }

    if (_payloadLength <= 0xFF) {
        value = value | 0x10;
    }

    if (_idLength) {
        value = value | 0x8;
    }

    return value;
}

uint8_t NdefRecord::getTnf()
{
    return _tnf;
}

void NdefRecord::setTnf(uint8_t tnf)
{
    _tnf = tnf;
}

unsigned int NdefRecord::getTypeLength()
{
    return _typeLength;
}

int NdefRecord::getPayloadLength()
{
    return _payloadLength;
}

unsigned int NdefRecord::getIdLength()
{
    return _idLength;
}

std::string NdefRecord::getType()
{
    char type[_typeLength + 1];
    memcpy(type, _type, _typeLength);
    type[_typeLength] = '\0'; // null terminate
    return std::string(type);
}

// this assumes the caller created type correctly
void NdefRecord::getType(uint8_t* type)
{
    memcpy(type, _type, _typeLength);
}

void NdefRecord::setType(const uint8_t * type, const unsigned int numBytes)
{
    if(_typeLength)
    {
        free(_type);
    }

    _type = (uint8_t*)malloc(numBytes);
    memcpy(_type, type, numBytes);
    _typeLength = numBytes;
}

// assumes the caller sized payload properly
void NdefRecord::getPayload(uint8_t *payload)
{
    memcpy(payload, _payload, _payloadLength);
}

void NdefRecord::setPayload(const uint8_t * payload, const int numBytes)
{
    if (_payloadLength)
    {
        free(_payload);
    }

    _payload = (uint8_t*)malloc(numBytes);
    memcpy(_payload, payload, numBytes);
    _payloadLength = numBytes;
}

std::string NdefRecord::getId()
{
    char id[_idLength + 1];
    memcpy(id, _id, _idLength);
    id[_idLength] = '\0'; // null terminate
    return std::string (id);
}

void NdefRecord::getId(uint8_t *id)
{
    memcpy(id, _id, _idLength);
}

void NdefRecord::setId(const uint8_t * id, const unsigned int numBytes)
{
    if (_idLength)
    {
        free(_id);
    }

    _id = (uint8_t*)malloc(numBytes);
    memcpy(_id, id, numBytes);
    _idLength = numBytes;
}

void NdefRecord::print()
{
    printf("  NDEF Record");
    printf(("    TNF 0x"));printf("%X", _tnf);printf(" ");
    switch (_tnf) {
    case TNF_EMPTY:
        printf(("Empty\n"));
        break;
    case TNF_WELL_KNOWN:
        printf(("Well Known\n"));
        break;
    case TNF_MIME_MEDIA:
        printf(("Mime Media\n"));
        break;
    case TNF_ABSOLUTE_URI:
        printf(("Absolute URI\n"));
        break;
    case TNF_EXTERNAL_TYPE:
        printf(("External\n"));
        break;
    case TNF_UNKNOWN:
        printf(("Unknown\n"));
        break;
    case TNF_UNCHANGED:
        printf(("Unchanged\n"));
        break;
    case TNF_RESERVED:
        printf(("Reserved\n"));
        break;
    default:
        printf("\n");
    }
    printf(("    Type Length 0x"));printf("%X", _typeLength);printf(" ");printf("%d\n", _typeLength);
    printf(("    Payload Length 0x"));printf("%X",_payloadLength);printf(" ");printf("%d\n",_payloadLength);
    if (_idLength)
    {
        printf(("    Id Length 0x"));printf("%X\n",_idLength);
    }
    printf(("    Type "));PrintHexChar(_type, _typeLength);
    // TODO chunk large payloads so this is readable
    printf(("    Payload "));PrintHexChar(_payload, _payloadLength);
    if (_idLength)
    {
        printf(("    Id "));PrintHexChar(_id, _idLength);
    }
    printf(("    Record is "));printf("%d",getEncodedSize());printf(" bytes\n");

}