#pragma once

#ifndef ARDUINO
#include <cstdint>
#else
#include <Arduino.h>
#endif


/**
 * The namespace containing the library.
 */
namespace proto
{
    /**
     * These are special bytes used by this protocol
     */
    enum ESpecial
    {
        /**
         * The xor byte: used together with the `eESC` byte to escape special bytes
         */
        eXOR    = 0x20,
        /**
         * The header byte: indicates the start of a frame
         */
        eHDR    = 0x7B,
        /**
         * The escape byte: used to escape special bytes
         */
        eESC    = 0x7C,
        /**
         * The footer byte: indicates the end of a frame
         */
        eFTR    = 0x7D,
    };

    /**
     * The main (and only) class containing the whole functionality. 
     * This class holds both the encoder and decoder.
     * It stores the encoded or decoded message in the internal buffer.
     * A typical workflow looks like this:
     *
     * ```
     * Bicoder<5> bicoder; // Create an instance
     * if(bicoder.encodeMessage(data, length)) // encode
     * {
     *   uint8_t* pBuffer = bicoder.buff(); // pointer to the buffer
     *   for(uint8_t i = 0; i < bicoder.size(); ++i)
     *   {
     *     print(pBuffer[i]); // do something with the encoded message
     *   }
     * }
     *
     * if(bicoder.decodeMessage(data, length)) // decode
     * {
     *   // similar to the code above
     * }
     * ```
     *
     * > [!NOTE]
     * > The buffer is shared between the encoder and decoder so don't try to encode
     * > a message while you haven't finished decoding, and vice versa.
     * > It **doesn't** mean you have to
     * > create 2 instances of this class: one for the encoder and one for the decoder (though
     * > you can if you want). It just means once you've started, f.e.
     * > the encoding process any data in the buffer
     * > (if any) is lost, and vice versa. Once you've read the encoded (decoded) message
     * > and you don't need it anymore you can start a new operation.
     *
     * @tparam NMaxMessage The maximum length of a (raw) message, i.e. the maximum number
     * of bytes that the data may consist of.
     */
    template<uint8_t NMaxMessage = 10>
    struct Bicoder
    {
        static_assert( (0U < NMaxMessage) and (NMaxMessage < 127U),
                       "The max length of the message is out of range" );

        /**
         * Maximum possible length of the encoded message.
         * This is the capacity of the internal buffer.
         */
        static constexpr uint8_t maxEncodedSize = 2U * NMaxMessage + 2U;

        Bicoder() = default;

        /**
         * Decode a single byte.
         * An auxiliary function. Used by the `decodeMessage` member-function.
         * @param data the byte to decode
         * @return `true` if succeedded, `false` - otherwise
         *
         * > [!IMPORTANT]
         * > Before this function is called on the **first** byte of the message it is necessary
         * > to call the reset() member-function first:
         *
         * ```
         * Bicoder<5> bicoder;
         * ...
         * bicoder.reset(); // call this first before decoding a message byte by byte
         * for(uint8_t i = 0; i < messageLength; ++i)
         * {
         *   bicoder.decodeByte(message[i]);
         * }
         * ```
         *
         * @see `isCompleted()`
         */
        bool            decodeByte( const uint8_t data );
        /**
         * Decode an encoded message.
         *
         * > [!NOTE]
         * > This function overwrites the internal buffer, so be sure you don't need its content
         * > anymore before call this member-function.
         *
         * > [!NOTE]
         * > This member-function calls the reset() member-function.
         *
         * @param data the encoded message
         * @param size the length of the encoded message (the number of bytes in \p data)
         * @return `true` if succeedded, `false` - otherwise
         * @see `size()`, `buff()`
         */
        bool            decodeMessage( const uint8_t* data, uint8_t size );

        /**
         * Encode a raw message.
         *
         * > [!NOTE]
         * > This function rewrites the internal buffer, so be sure you don't need its content
         * > anymore before call this member-function.
         *
         * > [!NOTE]
         * > This member-function calls the reset() member-function.
         *
         * @param data the raw message
         * @param size the length of the message (the number of bytes in \p data)
         * @return `true` if succeedded, `false` - otherwise
         * @see `size()`, `buff()`
         */
        bool            encodeMessage( const uint8_t* data, uint8_t size );

        /**
         * Resets the internal state of the instance.
         * After this member function has been called the state is the following:
         * - the `isCompleted` member function will return `false`
         * - the decoder is in the `waitHeader` state awaiting for the start of the frame
         * - the current index of the internal buffer is set to 0
         *
         * \note It doesn't clear the buffer - just sets the current index to 0
         */
        void            reset();

        /**
         * Indicates whether a frame has been received (decoded) completely.
         * Call it to check if the buffer contains a complete frame, i.e. the message
         * has been decoded. Usually used in conjunction with the `decodeByte()` member function.
         * @return `true` if the internal buffer contains a complete frame and can be read using
         * the `size` and `buff` member functions. `false` - otherwise.
         * @see `decodeByte()`
         */
        bool            isCompleted() const { return m_isCompleted; }

        /**
         * Returns the size of the internal buffer.
         * Use this member-function in conjunction with the `buff` member-function
         * to read the encoded/decoded message.
         * @see `buff()`
         */
        uint8_t         size() const { return m_index; }

        /**
         * Use this member-function in conjunction with the `size` member-function
         * to read the encoded/decoded message.
         * @see `size()`
         */
        const uint8_t*  buff() const { return m_message; }

        private :
            using State_t = bool (Bicoder::*)(const uint8_t data);

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

