#include <iostream>
#include <algorithm>
#include <cassert>

#include "DataLinkSerialProtocol.h"

bool compareBuffers( const uint8_t* buff1, uint8_t size1,
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

using namespace proto;

int main()
{
    constexpr uint8_t maxN = 10;
    constexpr uint8_t hdr = ESpecial::eHDR;
    constexpr uint8_t ftr = ESpecial::eFTR;
    constexpr uint8_t esc = ESpecial::eESC;
    constexpr uint8_t x   = ESpecial::eXOR;

    Bicoder<maxN> bicoder;

    /****** Good Message ******/
    {
        constexpr uint8_t msg[10] =
            { hdr, esc, hdr, 0, 0, 0, esc, ftr, 0, 0 };

        constexpr uint8_t msgEnc[] =
            { hdr,
              esc, (hdr ^ x), esc, (esc ^ x), esc, (hdr ^ x), 0, 0, 0, esc, (esc ^ x), esc, (ftr ^ x), 0, 0,
              ftr };

        assert( bicoder.encodeMessage( msg, sizeof(msg) ) );
        assert( compareBuffers( bicoder.buff(), bicoder.size(),
                                msgEnc, sizeof(msgEnc) ) );

        assert( bicoder.decodeMessage( msgEnc, sizeof(msgEnc) ) );
        assert( compareBuffers( bicoder.buff(), bicoder.size(),
                                msg, sizeof(msg) ) );
    }

    /****** Corrupted Message ******/
    {
        constexpr uint8_t msgEncCorrupted[] =
            { hdr,
              esc, (hdr ^ x), esc, (esc ^ x), esc, (hdr ^ x), 0, 0, 0, esc, (esc ^ x), esc, (ftr ^ x), 0, 0,
              hdr };  // header instead of footer

        assert( not bicoder.decodeMessage( msgEncCorrupted, sizeof( msgEncCorrupted ) ) );
    }

    /****** Long Message ******/
    {
        constexpr uint8_t msgLong[11] =
            { esc, esc, esc, esc, esc, esc, esc, esc, esc, esc, esc };

        constexpr uint8_t msgLongEnc[] =
            { hdr,
              esc, (esc ^ x), esc, (esc ^ x), esc, (esc ^ x), esc, (esc ^ x), esc, (esc ^ x),
              esc, (esc ^ x), esc, (esc ^ x), esc, (esc ^ x), esc, (esc ^ x), esc, (esc ^ x), esc, (esc ^ x),
              ftr };

        assert( not bicoder.encodeMessage( msgLong, sizeof(msgLong) ) );
        assert( not bicoder.decodeMessage( msgLongEnc, sizeof(msgLongEnc) ) );

        Bicoder<11> lBicoder;

        assert( lBicoder.encodeMessage( msgLong, sizeof(msgLong) ) );
        assert( lBicoder.decodeMessage( msgLongEnc, sizeof(msgLongEnc) ) );
    }

    /****** Two Messages (mocking a stream) ******/
    {
        constexpr uint8_t msgStream[2] = { esc, hdr };

        constexpr uint8_t msgStreamDoubleEnc[] =
        {
            0, 0, 0,
            hdr, esc, (esc ^ x), esc, (hdr ^ x), ftr,
            hdr, esc, (esc ^ x), esc, (hdr ^ x), ftr,
            0,
            hdr, esc, (esc ^ x), esc, (hdr ^ x), ftr,
            0, 0, 0
        };

        bicoder.reset();
        uint8_t numMsg = 0;
        for( uint8_t i = 0; i < sizeof(msgStreamDoubleEnc); ++i )
        {
            if( bicoder.isCompleted() )
            {
                ++numMsg;
                assert( compareBuffers( bicoder.buff(), bicoder.size(),
                                        msgStream, sizeof(msgStream) ) );
                bicoder.reset();
            }

            if( bicoder.decodeByte( msgStreamDoubleEnc[i] ) )
                continue;
        }
        assert( numMsg == 3 );
    }

    //Bicoder<127> bc; // shouldn't compile

    std::cout << "Test has been passed !\n";
}
