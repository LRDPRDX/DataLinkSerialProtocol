#pragma once

#include <cinttypes>

namespace proto
{
    enum ESpecial
    {
        eXOR    = 0x20,
        eHDR    = 0x7B,
        eESC    = 0x7C,
        eFTR    = 0x7D,
    };

    template<uint8_t TMaxData = 10>
    struct Bicoder
    {
        static_assert( (0U < TMaxData) and (TMaxData < 127U),
                       "The max length of the message is out of range" );

        static constexpr uint8_t maxEncodedSize = 2U * TMaxData + 2U;

        using State_t = bool (Bicoder::*)(uint8_t data);

        Bicoder() = default;

        bool            decodeByte( uint8_t data );
        bool            decodeMessage( const uint8_t* data, uint8_t size );
        bool            encodeMessage( const uint8_t* data, uint8_t size );
        void            reset();
        bool            isCompleted() const { return m_isCompleted; }
        uint8_t         size() const { return m_index; }
        const uint8_t*  buff() const { return m_message; }

        private :
            bool waitHeader(uint8_t data);
            bool inMessage(uint8_t data);
            bool afterEscape(uint8_t data);

            bool        pushByte( uint8_t data );

            uint8_t     m_message[maxEncodedSize] = { 0 };
            uint32_t    m_index { 0 };
            State_t     m_state { &Bicoder::waitHeader };
            bool        m_isCompleted { false };
    };

    template<uint8_t N>
    bool Bicoder<N>::pushByte( uint8_t data )
    {
        if( m_index >= N )
        {
            reset();
            return false;
        }

        m_message[m_index++] = data;
        return true;
    }

    template<uint8_t N>
    void Bicoder<N>::reset()
    {
        m_index             = 0;
        m_state             = &Bicoder::waitHeader;
        m_isCompleted       = false;
    }

    template<uint8_t N>
    bool Bicoder<N>::decodeByte( uint8_t data )
    {
        return (this->*m_state)( data );
    }

    template<uint8_t N>
    bool Bicoder<N>::decodeMessage( const uint8_t* data, uint8_t size )
    {
        reset();

        for( uint8_t i = 0; (i < size); ++i )
        {
            if( not decodeByte( data[i] ) )
                return false;
        }

        return m_isCompleted;
    }

    template<uint8_t N>
    bool Bicoder<N>::encodeMessage( const uint8_t* data, uint8_t size )
    {
        reset();

        if( N < size )
            return false;

        m_message[m_index++] = ESpecial::eHDR;

        for( uint8_t i = 0; i < size; ++i )
        {
            switch( data[i] )
            {
                case( ESpecial::eHDR ) :
                case( ESpecial::eESC ) :
                case( ESpecial::eFTR ) :
                    m_message[m_index++] = ESpecial::eESC;
                    m_message[m_index++] = (data[i] ^ ESpecial::eXOR);
                    break;
                default :
                    m_message[m_index++] = data[i];
            }
        }

        m_message[m_index++] = ESpecial::eFTR;

        return m_isCompleted = true;
    }

    template<uint8_t N>
    bool Bicoder<N>::waitHeader( uint8_t data )
    {
        reset();

        if( data == ESpecial::eHDR )
            m_state = &Bicoder::inMessage;

        return true;
    }

    template<uint8_t N>
    bool Bicoder<N>::inMessage( uint8_t data )
    {
        switch( data )
        {
            case ESpecial::eFTR :
                m_state = &Bicoder::waitHeader;
                m_isCompleted = true;
                return true;
            case ESpecial::eESC :
                m_state = &Bicoder::afterEscape;
                return true;
            case ESpecial::eHDR :
                reset();
                return false;
            default :
                return pushByte( data );
        }
    }

    template<uint8_t N>
    bool Bicoder<N>::afterEscape( uint8_t data )
    {
        m_state = &Bicoder::inMessage;
        pushByte( data ^ ESpecial::eXOR );
        return true;
    }
}// proto

