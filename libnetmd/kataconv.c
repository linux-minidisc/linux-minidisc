/*
 * kataconv.c
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
 * Copyright (C) 2020 Charlie Somerville
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "kataconv.h"

struct tail {
    uint8_t a;
    uint8_t b;
};

static const uint8_t FULL_HEAD = 0xe3;

// second and third bytes of each full-width katakana
// the first byte is always 0xe3, so we don't need to include it in this list
static const struct tail FULL_TAIL[] = {
    { 0x80, 0x81 }, // "、"
    { 0x80, 0x82 }, // "。"
    { 0x80, 0x8c }, // "「"
    { 0x80, 0x8d }, // "」"
    { 0x82, 0x9b }, // "゛"
    { 0x82, 0x9c }, // "゜"
    { 0x82, 0xa1 }, // "ァ"
    { 0x82, 0xa2 }, // "ア"
    { 0x82, 0xa3 }, // "ィ"
    { 0x82, 0xa4 }, // "イ"
    { 0x82, 0xa5 }, // "ゥ"
    { 0x82, 0xa6 }, // "ウ"
    { 0x82, 0xa7 }, // "ェ"
    { 0x82, 0xa8 }, // "エ"
    { 0x82, 0xa9 }, // "ォ"
    { 0x82, 0xaa }, // "オ"
    { 0x82, 0xab }, // "カ"
    { 0x82, 0xad }, // "キ"
    { 0x82, 0xaf }, // "ク"
    { 0x82, 0xb1 }, // "ケ"
    { 0x82, 0xb3 }, // "コ"
    { 0x82, 0xb5 }, // "サ"
    { 0x82, 0xb7 }, // "シ"
    { 0x82, 0xb9 }, // "ス"
    { 0x82, 0xbb }, // "セ"
    { 0x82, 0xbd }, // "ソ"
    { 0x82, 0xbf }, // "タ"
    { 0x83, 0x81 }, // "チ"
    { 0x83, 0x83 }, // "ッ"
    { 0x83, 0x84 }, // "ツ"
    { 0x83, 0x86 }, // "テ"
    { 0x83, 0x88 }, // "ト"
    { 0x83, 0x8a }, // "ナ"
    { 0x83, 0x8b }, // "ニ"
    { 0x83, 0x8c }, // "ヌ"
    { 0x83, 0x8d }, // "ネ"
    { 0x83, 0x8e }, // "ノ"
    { 0x83, 0x8f }, // "ハ"
    { 0x83, 0x92 }, // "ヒ"
    { 0x83, 0x95 }, // "フ"
    { 0x83, 0x98 }, // "ヘ"
    { 0x83, 0x9b }, // "ホ"
    { 0x83, 0x9e }, // "マ"
    { 0x83, 0x9f }, // "ミ"
    { 0x83, 0xa0 }, // "ム"
    { 0x83, 0xa1 }, // "メ"
    { 0x83, 0xa2 }, // "モ"
    { 0x83, 0xa3 }, // "ャ"
    { 0x83, 0xa4 }, // "ヤ"
    { 0x83, 0xa5 }, // "ュ"
    { 0x83, 0xa6 }, // "ユ"
    { 0x83, 0xa7 }, // "ョ"
    { 0x83, 0xa8 }, // "ヨ"
    { 0x83, 0xa9 }, // "ラ"
    { 0x83, 0xaa }, // "リ"
    { 0x83, 0xab }, // "ル"
    { 0x83, 0xac }, // "レ"
    { 0x83, 0xad }, // "ロ"
    { 0x83, 0xaf }, // "ワ"
    { 0x83, 0xb2 }, // "ヲ"
    { 0x83, 0xb3 }, // "ン"
    { 0x83, 0xbb }, // "・"
    { 0x83, 0xbc }, // "ー"
    { 0, 0 }, // end
};

static const uint8_t HALF_HEAD = 0xef;

// same as FULL_TAIL, but for the half width forms.
static const struct tail HALF_TAIL[] = {
    { 0xbd, 0xa4 }, // "､"
    { 0xbd, 0xa1 }, // "｡"
    { 0xbd, 0xa2 }, // "｢"
    { 0xbd, 0xa3 }, // "｣"
    { 0xbe, 0x9e }, // "ﾞ"
    { 0xbe, 0x9f }, // "ﾟ"
    { 0xbd, 0xa7 }, // "ｧ"
    { 0xbd, 0xb1 }, // "ｱ"
    { 0xbd, 0xa8 }, // "ｨ"
    { 0xbd, 0xb2 }, // "ｲ"
    { 0xbd, 0xa9 }, // "ｩ"
    { 0xbd, 0xb3 }, // "ｳ"
    { 0xbd, 0xaa }, // "ｪ"
    { 0xbd, 0xb4 }, // "ｴ"
    { 0xbd, 0xab }, // "ｫ"
    { 0xbd, 0xb5 }, // "ｵ"
    { 0xbd, 0xb6 }, // "ｶ"
    { 0xbd, 0xb7 }, // "ｷ"
    { 0xbd, 0xb8 }, // "ｸ"
    { 0xbd, 0xb9 }, // "ｹ"
    { 0xbd, 0xba }, // "ｺ"
    { 0xbd, 0xbb }, // "ｻ"
    { 0xbd, 0xbc }, // "ｼ"
    { 0xbd, 0xbd }, // "ｽ"
    { 0xbd, 0xbe }, // "ｾ"
    { 0xbd, 0xbf }, // "ｿ"
    { 0xbe, 0x80 }, // "ﾀ"
    { 0xbe, 0x81 }, // "ﾁ"
    { 0xbd, 0xaf }, // "ｯ"
    { 0xbe, 0x82 }, // "ﾂ"
    { 0xbe, 0x83 }, // "ﾃ"
    { 0xbe, 0x84 }, // "ﾄ"
    { 0xbe, 0x85 }, // "ﾅ"
    { 0xbe, 0x86 }, // "ﾆ"
    { 0xbe, 0x87 }, // "ﾇ"
    { 0xbe, 0x88 }, // "ﾈ"
    { 0xbe, 0x89 }, // "ﾉ"
    { 0xbe, 0x8a }, // "ﾊ"
    { 0xbe, 0x8b }, // "ﾋ"
    { 0xbe, 0x8c }, // "ﾌ"
    { 0xbe, 0x8d }, // "ﾍ"
    { 0xbe, 0x8e }, // "ﾎ"
    { 0xbe, 0x8f }, // "ﾏ"
    { 0xbe, 0x90 }, // "ﾐ"
    { 0xbe, 0x91 }, // "ﾑ"
    { 0xbe, 0x92 }, // "ﾒ"
    { 0xbe, 0x93 }, // "ﾓ"
    { 0xbd, 0xac }, // "ｬ"
    { 0xbe, 0x94 }, // "ﾔ"
    { 0xbd, 0xad }, // "ｭ"
    { 0xbe, 0x95 }, // "ﾕ"
    { 0xbd, 0xae }, // "ｮ"
    { 0xbe, 0x96 }, // "ﾖ"
    { 0xbe, 0x97 }, // "ﾗ"
    { 0xbe, 0x98 }, // "ﾘ"
    { 0xbe, 0x99 }, // "ﾙ"
    { 0xbe, 0x9a }, // "ﾚ"
    { 0xbe, 0x9b }, // "ﾛ"
    { 0xbe, 0x9c }, // "ﾜ"
    { 0xbd, 0xa6 }, // "ｦ"
    { 0xbe, 0x9d }, // "ﾝ"
    { 0xbd, 0xa5 }, // "･"
    { 0xbd, 0xb0 }, // "ｰ"
    { 0, 0 }, // end
};

static void
convert(uint8_t* input, uint8_t from_head, const struct tail* const from, uint8_t to_head, const struct tail* const to)
{
    for (; *input; input++) {
        if (*input == from_head) {
            const struct tail* from_i = from;
            const struct tail* to_i = to;

            // bounds check:
            if (input[1] == 0 || input[2] == 0) {
                // whoops, the string terminates early. leave it as-is
                return;
            }

            // search for this character in the conversion map
            for (; from_i->a; from_i++, to_i++) {
                if (from_i->a == input[1] && from_i->b == input[2]) {
                    input[0] = to_head;
                    input[1] = to_i->a;
                    input[2] = to_i->b;
                    input += 2;
                    break;
                }
            }
        }
    }
}

// rewrites full-width katakana to half-width in place
void
kata_full_to_half(uint8_t* input)
{
    convert(input, FULL_HEAD, FULL_TAIL, HALF_HEAD, HALF_TAIL);
}

// rewrites half-width katakana to full-width in place
void
kata_half_to_full(uint8_t* input)
{
    convert(input, HALF_HEAD, HALF_TAIL, FULL_HEAD, FULL_TAIL);
}
