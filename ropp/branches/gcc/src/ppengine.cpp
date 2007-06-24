/*
 * OpenKore - Padded Packet Emulator.
 * Copyright (c) 2007 kLabMouse, Japplegame, and many other contributors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl.html for the full license.
 */

#include <memory.h>
#include <stdlib.h>
#include "ppengine.h"
#include "Algorithms/algorithms.h"

namespace OpenKore {
namespace PaddedPackets {
	

/********** Block **********/

Block::Block()
{
	// Reserve some space
	bufLen = 5;
	buffer = new dword[bufLen];

	memset(buffer, 0, bufLen * sizeof(dword));
	currentPos = 0;
}

Block::~Block()
{
	if (buffer != NULL) {
		delete[] buffer;
	}
}

void Block::reset()
{
	currentPos = 0;
}

void Block::add(dword data)
{
	if (buffer == NULL) {
		return;
	}

	if (currentPos == (bufLen - 1)) {
		// Allocate more space.
		bufLen = bufLen + 10;
		dword *newBuffer = new dword[ bufLen ];

		//if ( newBuffer == NULL )
		//throw something here

		memcpy( (void*)newBuffer, (void*)buffer, bufLen );
		delete[] buffer;
		buffer = newBuffer;
	} else {
		// Write to buffer directly.
		buffer[currentPos] = data;
		currentPos++;
	}
}

unsigned int
Block::getSize() const
{
	return currentPos;
}

dword
Block::operator[](unsigned int index) const
{
	if (index < currentPos) {
		return buffer[index];
	} else {
		return 0;
	}
}


/********** Engine **********/

Engine::Engine()
{
	serverMapSync = 0;
	clientSync = 0;

	memset( pktBuffer, 0, PPENGINE_BUFSIZE * sizeof(byte) );
}

Engine::~Engine()
{
}

void
Engine::AddKey(dword data)
{
	inputKeys.add( data );
}

void
Engine::SetSync(dword sync)
{
	clientSync = sync;
}

void
Engine::SetMapSync(dword mapSync)
{
	serverMapSync = mapSync;
}

void
Engine::SetAccId(dword accId)
{
	clientAccId = accId;
}

void
Engine::SetPacket(byte *packet, dword len)
{
	memcpy( pktBuffer, packet, len);
}

unsigned int
Engine::Encode(byte *dest, word type)
{
	dword offsets[] = { 15, 14, 12, 9, 5, 0 };

	dword hashData = createHash( serverMapSync, clientSync, clientAccId, type );

	unsigned int packetLength = 0;
	int iterations = 5;
	// pad_2
	for( int iter = 0; iter <= iterations; iter++) {
		packetLength = (1 + inputKeys.getSize()) * 4;

		dword intCtr = 5;
		byte *writePtr = pktBuffer + 4;
		for( unsigned int pass = 0; pass < inputKeys.getSize(); pass++ ) {
			dword magic = ((intCtr * (dword)pass) + (hashData - offsets[iter])) % 0x27;
			packetLength += magic;
			intCtr += 3;

			writePtr += (4 + magic);
			*((dword*)writePtr - 1) = inputKeys[pass] + iter - 5;
		}
	}

	pktBuffer[2] = (byte)packetLength;
	*(word*)pktBuffer = (word)type;

	// Reset input keys for next generation
	inputKeys.reset();

	memcpy(dest, pktBuffer, packetLength);
	return packetLength;
}

void
Engine::Decode(byte *src, unsigned int keys)
{
	// Reset output keys
	outputKeys.reset();

	dword hashData = createHash( serverMapSync, clientSync, clientAccId, *(word*)src );

	dword intCtr = 5;
	byte *readPtr = src + 4;
	for( unsigned int pass = 0; pass < keys; pass++ ) {
		dword magic = ((intCtr * (dword)pass) + hashData) % 0x27;
		intCtr += 3;

		readPtr += (4 + magic);
		outputKeys.add( *((dword*)readPtr - 1) );
	}
}

dword Engine::GetKey(unsigned int index) const
{
	return outputKeys[index];
}

} // PaddedPackets
} // OpenKore
