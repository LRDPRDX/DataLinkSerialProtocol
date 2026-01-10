#include <cassert>

#include "DataLinkSerialProtocol.h"

using namespace proto;

// An auxiliary function to compare two buffers
bool compare( const uint8_t* buff1, uint8_t size1,
              const uint8_t* buff2, uint8_t size2 )
{
    if( size1 != size2 )
        return false;

    for( uint8_t i = 0; i < size1; ++i )
    {
        if( buff1[i] != buff2[i] )
            return false;
    }

    return true;
}

int main ()
{
    Bicoder<5> bicoder;

    // Aliases to type less
    constexpr uint8_t HDR = ESpecial::eHDR;
    constexpr uint8_t FTR = ESpecial::eFTR;
    constexpr uint8_t ESC = ESpecial::eESC;
    constexpr uint8_t XOR = ESpecial::eXOR;

    // Raw data containing only two bytes both of which are special
    constexpr uint8_t data[] = { HDR, ESC };

    // The encoded frame
    constexpr uint8_t frame[] = {
                                  HDR,            // header
                                  ESC, HDR ^ XOR, // escape HDR
                                  ESC, ESC ^ XOR, // escape ESC
                                  FTR             // footer
                                };

    // Encode the raw message
    bicoder.encodeMessage(data, sizeof(data));
    assert(bicoder.isCompleted());
    assert(compare(bicoder.buff(), bicoder.size(),
                   frame,          sizeof(frame)));

    bicoder.reset();
    for(uint8_t i = 0; i < sizeof(frame); ++i)
    {
        if(bicoder.isCompleted())
        {
            assert(compare(bicoder.buff(), bicoder.size(),
                           data,           sizeof(data)));
            break;
        }

        bicoder.decodeByte(frame[i]);
    }
}
