#include "attol/char.h"

namespace attol {

template<>
const typename CodeUnit<Encoding::UTF8>::type*& StepNextCharacter<Encoding::UTF8>(const typename CodeUnit<Encoding::UTF8>::type*& word)
{
    // step over continuation bytes
    do
    {
        ++word;
    } while (((*word) & 0xC0) == 0x80);
    return word;
}

template<>
const typename CodeUnit<Encoding::UTF16>::type*& StepNextCharacter<Encoding::UTF16>(const typename CodeUnit<Encoding::UTF16>::type*& word)
{
    if (((*word) & 0xFC00) == 0xD800)
        word += 2;
    else
        ++word;
    return word;
}

}
