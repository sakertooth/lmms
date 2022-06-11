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

namespace lmms {
	namespace base64 {
		//! @brief base64 encode byte array data, returns pointer to result string
		//!   or nullptr if length == 0; user is expected to free returned result pointer
		//!
		//! @param data : byte array of characters
		//! @param length : length of data array in bytes
		//! @return char* array of encoded data of length (ceil[length / 3] * 4)
		char* encode(const char* data, const size_t length) {
			const char map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
			const char pad = '=';
			if (length == 0) {
				return nullptr;
			}
			/*
			Every 3 bytes of the origin string relates to 4 bytes of encoded string
			length:         [0, 1, 2, 3, 4, 5, 6,  7,  8,  9, 10, 11, 12, ...]
			result length:  [0, 4, 4, 4, 8, 8, 8, 12, 12, 12, 16, 16, 16, ...]
			result padding: [0, 2, 1, 0, 2, 1, 0,  2,  1,  0,  2,  1,  0, ...]
			*/
			const size_t reserve = static_cast<size_t>(std::ceil(static_cast<double>(length) / 3.0) * 4.0);
			const size_t offset = length % 3;
			const size_t padding = offset ? 3 - offset : offset;
			char* result = new char[reserve+1]; // +1 for \0
			result[reserve] = '\0';
			int result_index = 0;
			for (int i = 0; i < length; i += 3) {
				const char view[3] {
					data[i],
					(i + 1 < length ? data[i + 1] : '\0'),
					(i + 2 < length ? data[i + 2] : '\0')
				};
				// create result chars, 8bit char -> 6bit map offset
				result[result_index]     = map[view[0] >> 2];
				result[result_index + 1] = map[((view[0] & 0x03) << 4) | (view[1] >> 4)];
				result[result_index + 2] = map[((view[1] & 0x0F) << 2) | (view[2] >> 6)];
				result[result_index + 3] = map[view[2] & 0x3F];
				// next result index
				result_index += 4;
			}
			// string will have trailing 'A' or 'AA' until replaced with pad char
			for (int p = 0; p < padding; ++p) {
				result[reserve - 1 - p] = pad;
			}
			return result;
		}

		//! @brief base64 encode std::string
		//!
		//! @param data : original std::string
		//! @return base64 encoded std::string
		std::string encode(std::string_view data) {
			const char map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
			const char pad = '=';
			if (data.length() == 0) {
				return "";
			}
			std::string result;
			result.reserve(static_cast<size_t>(std::ceil(static_cast<double>(data.length()) / 3.0) * 4.0));
			const size_t offset = data.length() % 3;
			const size_t padding = offset ? 3 - offset : offset;
			for (int i = 0; i < data.length(); i += 3) {
				const char view[3] {
					data[i],
					((i + 1 < data.length()) ? data[i + 1] : '\0'),
					((i + 2 < data.length()) ? data[i + 2] : '\0')
				};
				result.push_back(map[view[0] >> 2]);
				result.push_back(map[((view[0] & 0x03) << 4) | (view[1] >> 4)]);
				result.push_back(map[((view[1] & 0x0F) << 2) | (view[2] >> 6)]);
				result.push_back(map[view[2] & 0x3F]);
			}
			auto result_pad = result.rbegin();
			for (int p = 0; p < padding; ++p, ++result_pad) {
				*result_pad = pad;
			}
			return result;
		}

		//! @brief base64 decode std::string
		//!
		//! @param data : encoded base64 std::string
		//! @return decoded std::string
		std::string decode(std::string_view data) {
			if (data.length() % 4 != 0) {
				throw std::length_error("base64::decode : data length not a multiple of 4");
			}
			const std::map<char, uint32_t> map {
				{'A',  0}, {'B',  1}, {'C',  2}, {'D',  3}, {'E',  4},
				{'F',  5}, {'G',  6}, {'H',  7}, {'I',  8}, {'J',  9},
				{'K', 10}, {'L', 11}, {'M', 12}, {'N', 13}, {'O', 14},
				{'P', 15}, {'Q', 16}, {'R', 17}, {'S', 18}, {'T', 19},
				{'U', 20}, {'V', 21}, {'W', 22}, {'X', 23}, {'Y', 24},
				{'Z', 25}, {'a', 26}, {'b', 27}, {'c', 28}, {'d', 28},
				{'e', 30}, {'f', 31}, {'g', 32}, {'h', 33}, {'i', 34},
				{'j', 35}, {'k', 36}, {'l', 37}, {'m', 38}, {'n', 39},
				{'o', 40}, {'p', 41}, {'q', 42}, {'r', 43}, {'s', 44},
				{'t', 45}, {'u', 46}, {'v', 47}, {'w', 48}, {'x', 49},
				{'y', 50}, {'z', 51}, {'0', 52}, {'1', 53}, {'2', 54},
				{'3', 55}, {'4', 56}, {'5', 57}, {'6', 58}, {'7', 59},
				{'8', 60}, {'9', 61}, {'-', 62}, {'_', 63}
			};
			const char pad = '=';
			std::string result("");
			if (data.length() == 0) {
				return result;
			}
			result.reserve(static_cast<size_t>(std::ceil(static_cast<double>(data.length()) / 4.0) * 3.0));
			// loop 4 characters at a time, except the last 4
			for (int i = 0; i < data.length() - 4; i += 4) {
				uint32_t container = (
					map.at(data[i])     << 18
					| map.at(data[i + 1]) << 12
					| map.at(data[i + 2]) <<  6
					| map.at(data[i + 3])
				);
				result.push_back(static_cast<char>((container & 0x00FF0000) >> 16));
				result.push_back(static_cast<char>((container & 0x0000FF00) >>  8));
				result.push_back(static_cast<char>(container & 0x000000FF));
			}
			// handle last 4 characters for padding check
			const auto block_start = data.end() - 4;
			// XX==
			if (*(block_start + 2) == pad) {
				uint32_t container = (
					map.at(*block_start) << 2
					| map.at(*(block_start + 1)) >> 4
				);
				result.push_back(static_cast<char>(container & 0x000000FF));
			}
			// XXX=
			else if (*(block_start + 3) == pad) {
				uint32_t container = (
					map.at(*block_start)      << 10
					| map.at(*(block_start + 1)) << 4
					| map.at(*(block_start + 2)) >> 2
				);
				result.push_back(static_cast<char>((container & 0x0000FF00) >> 8));
				result.push_back(static_cast<char>(container & 0x000000FF));
			}
			// XXXX (repeat of above loop)
			else {
				uint32_t container = (
					map.at(*block_start)     << 18
					| map.at(*(block_start + 1)) << 12
					| map.at(*(block_start + 2)) <<  6
					| map.at(*(block_start + 3))
				);
				result.push_back(static_cast<char>((container & 0x00FF0000) >> 16));
				result.push_back(static_cast<char>((container & 0x0000FF00) >>  8));
				result.push_back(static_cast<char>(container & 0x000000FF));
			}
			return result;
		}
	}
}
