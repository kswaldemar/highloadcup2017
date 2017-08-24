// Intitial version
// Copyright (C) 2003  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
//
// Changed by me
#pragma once

namespace dlib {

inline unsigned char from_hex(
    unsigned char ch
) {
    if (ch <= '9' && ch >= '0')
        ch -= '0';
    else if (ch <= 'f' && ch >= 'a')
        ch -= 'a' - 10;
    else if (ch <= 'F' && ch >= 'A')
        ch -= 'A' - 10;
    else
        ch = 0;
    return ch;
}

void inplace_urldecode(char *str) {
    using namespace std;
    char *wr = str;
    size_t size = strlen(str);
    for (size_t i = 0; i < size; ++i) {
        if (str[i] == '+') {
            *wr++ = ' ';
        } else if (str[i] == '%' && size > i + 2) {
            const unsigned char ch1 = from_hex(str[i + 1]);
            const unsigned char ch2 = from_hex(str[i + 2]);
            const unsigned char ch = (ch1 << 4) | ch2;
            *wr++ = ch;
            i += 2;
        } else {
            *wr++ = str[i];
        }
    }
    while (wr < str + size) {
        *wr++ = '\0';
    }
}

}