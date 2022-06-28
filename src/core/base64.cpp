/*
 * base64.cpp - namespace base64 with methods for encoding/decoding binary data
 *              to/from base64
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "base64.h"

#include <bitset>

#include <QBuffer>
#include <QDataStream>

namespace base64
{


QVariant decode( const QString & _b64, QVariant::Type _force_type )
{
	char * dst = nullptr;
	int dsize = 0;
	base64::decode( _b64, &dst, &dsize );
	QByteArray ba( dst, dsize );
	QBuffer buf( &ba );
	buf.open( QBuffer::ReadOnly );
	QDataStream in( &buf );
	QVariant ret;
	in >> ret;
	if( _force_type != QVariant::Invalid && ret.type() != _force_type )
	{
		buf.reset();
		in.setVersion( QDataStream::Qt_3_3 );
		in >> ret;
	}
	delete[] dst;
	return( ret );
}


} ;

namespace lmms 
{
	namespace base64 
	{
		constexpr std::array<char, 64> map = 
		{
			'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
			'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
			'0','1','2','3','4','5','6','7','8','9',
			'-','_'
		};

		constexpr char pad = '=';

		//TODO C++20: It *may* be possible to make the following functions constepxr given C++20's constexpr std::string.

		/**
		 * @brief Base64 encodes data.
		 * @param data
		 * @return std::string containing the Base64 encoded data.
		 */
		std::string encode(std::string_view data)
		{
			if (data.empty()) { return ""; }
			std::string result;

			constexpr int numBytesPerChunk = 3;
			constexpr int numBase64CharPerChunk = 4;
			constexpr int numBitsPerBase64Char = 6;
			auto [numChunks, numTrailingBytes] = std::div(data.length(), numBytesPerChunk);

			//Consider the few trailing bytes at the end a chunk
			if (numTrailingBytes > 0) 
			{ 
				++numChunks; 
				numTrailingBytes = 3 - numTrailingBytes;
			}
			
			for (int currentChunk = 0; currentChunk < numChunks; ++currentChunk)
			{
				std::bitset<24> inputGroup;
				
				const int numBytesInChunk = currentChunk == numChunks - 1 ? numBytesPerChunk - numTrailingBytes : numBytesPerChunk;
				for (int i = 0; i < numBytesInChunk; ++i)
				{
					//Push the data bytes to the far left of the inputGroup, which makes processing it easier
					inputGroup |= static_cast<int>(data[currentChunk * numBytesPerChunk + i]) << (24 - 8 * (i + 1));
				}

				const int numBase64CharInChunk = currentChunk == numChunks - 1 ? numBase64CharPerChunk - numTrailingBytes : numBase64CharPerChunk;
				for (int i = 0; i < numBase64CharInChunk; ++i)
				{
					std::bitset<24> base64Mask = 0xFC0000 >> i * numBitsPerBase64Char;
					const int base64Idx = ((inputGroup & base64Mask) >> (3 - i) * numBitsPerBase64Char).to_ulong();
					result += map[base64Idx];
				}

				//Add padding at the last chunk
				if (currentChunk == numChunks - 1) 
				{
					for (int i = 0; i < numTrailingBytes; ++i) { result += pad; }
				}
			}
			
			return result;
		}

		/**
		 * @brief Decodes data in Base64.
		 * 
		 * @param data 
		 * @return std::string containing the original data. 
		 */
		std::string decode(std::string_view data)
		{
			if (data.empty()) { return ""; }
			std::string result;

			constexpr int chunkLength = 4;
			constexpr int outputChunkLength = 3;
			constexpr int numBitsPerBase64Char = 6;

			const int numChunks = data.length() / chunkLength;
			const int numPads = std::count_if(data.begin(), data.end(), [](char c) { return c == '='; });

			for (int currentChunk = 0; currentChunk < numChunks; ++currentChunk)
			{
				std::bitset<24> inputGroup;

				const int numBase64CharInChunk = currentChunk == numChunks - 1 ? chunkLength - numPads : chunkLength;
				for (int i = 0; i < numBase64CharInChunk; ++i) 
				{
					const int base64Idx = std::distance(map.begin(), std::find(map.begin(), map.end(), data[currentChunk * chunkLength + i]));
					inputGroup |= base64Idx << (3 - i) * numBitsPerBase64Char;
				}

				for (int i = outputChunkLength; i >= 0; --i)
				{
					result += static_cast<char>((inputGroup >> 8 * i).to_ulong());
				}
			}

			return result;
		}
	}
}
