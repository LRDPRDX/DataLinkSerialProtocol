#pragma once

#ifndef ARDUINO
#include <cstdint>
#else
#include <Arduino.h>
#endif


namespace proto
{
    enum ESpecial
    {
        eXOR    = 0x20,
        eHDR    = 0x7B,
        eESC    = 0x7C,
        eFTR    = 0x7D,
    };

    template<uint8_t NMaxMessage = 10>
    struct Bicoder
    {
        static_assert( (0U < NMaxMessage) and (NMaxMessage < 127U),
                       "The max length of the message is out of range" );

        static constexpr uint8_t maxEncodedSize = 2U * NMaxMessage + 2U;

        using State_t = bool (Bicoder::*)(const uint8_t data);

        Bicoder() = default;

        bool            decodeByte( const uint8_t data );
        bool            decodeMessage( const uint8_t* data, uint8_t size );
        bool            encodeMessage( const uint8_t* data, uint8_t size );
        void            reset();
        bool            isCompleted() const { return m_isCompleted; }
        uint8_t         size() const { return m_index; }
        const uint8_t*  buff() const { return m_message; }

        private :
            bool        waitHeader(const uint8_t data);
            bool        inMessage(const uint8_t data);
            bool        afterEscape(const uint8_t data);

            void        appendMessage( const uint8_t data );
            bool        pushByte( const uint8_t data );
            bool        encodeByte( const uint8_t data );

            State_t     m_state { &Bicoder::waitHeader };
            uint8_t     m_message[maxEncodedSize] = { 0 };
            uint8_t     m_index { 0 };
            bool        m_isCompleted { false };
    };

    template<uint8_t N>
    void Bicoder<N>::appendMessage( const uint8_t data )
    {
        m_message[m_index] = data;
        m_index++;
    }

    template<uint8_t N>
    bool Bicoder<N>::pushByte( const uint8_t data )
    {
        if( m_index >= N )
        {
            reset();
            return false;
        }

        appendMessage( data );

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
    bool Bicoder<N>::decodeByte( const uint8_t data )
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
    bool Bicoder<N>::encodeByte( const uint8_t data )
    {
        switch( data )
        {
            case( ESpecial::eHDR ) :
            case( ESpecial::eESC ) :
            case( ESpecial::eFTR ) :
                appendMessage( ESpecial::eESC );
                appendMessage( data ^ ESpecial::eXOR );
                break;
            default :
                appendMessage( data );
        }

        return true;
    }

    template<uint8_t N>
    bool Bicoder<N>::encodeMessage( const uint8_t* data, uint8_t size )
    {
        reset();

        if( N < size )
            return false;

        m_message[m_index++] = ESpecial::eHDR;

        for( uint8_t i = 0; i < size; ++i )
            encodeByte( data[i] );

        m_message[m_index++] = ESpecial::eFTR;

        return m_isCompleted = true;
    }

    template<uint8_t N>
    bool Bicoder<N>::waitHeader( const uint8_t data )
    {
        reset();

        if( data == ESpecial::eHDR )
            m_state = &Bicoder::inMessage;

        return true;
    }

    template<uint8_t N>
    bool Bicoder<N>::inMessage( const uint8_t data )
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
    bool Bicoder<N>::afterEscape( const uint8_t data )
    {
        m_state = &Bicoder::inMessage;
        pushByte( data ^ ESpecial::eXOR );
        return true;
    }
}// proto

