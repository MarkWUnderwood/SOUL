/*
    _____ _____ _____ __
   |   __|     |  |  |  |      The SOUL language
   |__   |  |  |  |  |  |__    Copyright (c) 2019 - ROLI Ltd.
   |_____|_____|_____|_____|

   The code in this file is provided under the terms of the ISC license:

   Permission to use, copy, modify, and/or distribute this software for any purpose
   with or without fee is hereby granted, provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
   NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
   DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
   IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*  The following string literal forms part of a set of SOUL code chunks that form
    the built-in library. (See the soul::getBuiltInLibraryCode() function)
*/
R"library(

//==============================================================================
namespace soul::midi
{
    /** This type is used to represent a packed short MIDI message. When you create
        an input event endpoint and would like it to receive MIDI, this is the type
        that you should use for it.
    */
    struct Message
    {
        int midiBytes;  /**< Format: (byte[0] << 16) | (byte[1] << 8) | byte[2] */
    }

    int getByte1 (Message m)     { return (m.midiBytes >> 16) & 0xff; }
    int getByte2 (Message m)     { return (m.midiBytes >> 8) & 0xff; }
    int getByte3 (Message m)     { return m.midiBytes & 0xff; }

    /** This event processor receives incoming MIDI events, parses them as MPE,
        and then emits a stream of note events using the types in soul::note_events.
        A synthesiser can then handle the resulting events without needing to go
        near any actual MIDI or MPE data.
    */
    processor MPEParser  [[ main: false ]]
    {
        input event Message parseMIDI;

        output event (soul::note_events::NoteOn,
                      soul::note_events::NoteOff,
                      soul::note_events::PitchBend,
                      soul::note_events::Pressure,
                      soul::note_events::Slide,
                      soul::note_events::Control) eventOut;

        let MPESlideControllerID = 74;

        event parseMIDI (Message message)
        {
            let messageByte1 = message.getByte1();
            let messageByte2 = message.getByte2();
            let messageByte3 = message.getByte3();

            let messageType  = messageByte1 & 0xf0;
            let channel      = messageByte1 & 0x0f;

            if (messageType == 0x80)
            {
                eventOut << soul::note_events::NoteOff (channel, float (messageByte2), normaliseValue (messageByte3));
            }
            else if (messageType == 0x90)
            {
                // Note on with zero velocity should be treated as a note off
                if (messageByte3 == 0)
                    eventOut << soul::note_events::NoteOff (channel, float (messageByte2), 0);
                else
                    eventOut << soul::note_events::NoteOn (channel, float (messageByte2), normaliseValue (messageByte3));
            }
            else if (messageType == 0xb0)
            {
                if (messageByte2 == MPESlideControllerID)
                    eventOut << soul::note_events::Slide (channel, normaliseValue (messageByte3));
                else
                    eventOut << soul::note_events::Control (channel, messageByte2, normaliseValue (messageByte3));
            }
            else if (messageType == 0xd0)
            {
                eventOut << soul::note_events::Pressure (channel, normaliseValue (messageByte2));
            }
            else if (messageType == 0xe0)
            {
                eventOut << soul::note_events::PitchBend (channel, translateBendSemitones (messageByte3, messageByte2));
            }
        }

        float normaliseValue (int i)
        {
            return i * (1.0f / 127.0f);
        }

        float translateBendSemitones (int msb, int lsb)
        {
            let value = msb * 128 + lsb;
            let bendRange = 48.0f;
            return float (value - 8192) / (8192.0f / bendRange);
        }
    }
}

)library"
