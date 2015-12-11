#ifndef CODEPAGE_H
#define CODEPAGE_H

BOOL codepage_init(const Path filename);
void codepage_free();

int codepage_guessLdbEncoding(const Path filename);

#endif
