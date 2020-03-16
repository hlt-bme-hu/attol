#include <stdio.h>
#include <string>

#include "attol/char.h"
#include "attol/Transducer.h"
#include "attol/FlagDiacritics.h"

namespace attol {

template<Encoding enc, size_t storageSize>
class PrintFunction
{
private:
    typedef Transducer<enc, storageSize> T;
    typename T::ResultHandler f;
    typedef std::char_traits<typename T::CharType> CharTraits;
    FILE* output;
    bool has_analyses;
public:
    PrintFunction(int print, FILE* o) : output(o), has_analyses(false)
    {
        switch (print)
        {
        case 0:
            f = [this](const typename T::Path& path)
            {
                static typename T::CharType c;
                for (const auto& v : path)
                {
                    const auto output_s = v.GetOutput();
                    fwrite(output_s, sizeof(c), CharTraits::length(output_s), output);
                }
                c = '\n';
                fwrite(&c, sizeof(c), 1, output);
                has_analyses = true;
            }; break;
        case 1:
            f = [this](const typename T::Path& path)
            {
                static typename T::CharType c;
                static typename T::string weight_s;
                typename T::Float weight = 0;
                for (const auto& v : path)
                {
                    const auto output_s = v.GetOutput();
                    weight += v.GetWeight();
                    fwrite(output_s, sizeof(c), CharTraits::length(output_s), output);
                }
                c = '\t';
                fwrite(&c, sizeof(c), 1, output);
                weight_s = attol::WriteFloat<typename T::CharType>(weight);
                fwrite(weight_s.c_str(), sizeof(c), weight_s.size(), output);
                c = '\n';
                fwrite(&c, sizeof(c), 1, output);
                has_analyses = true;
            }; break;
        case 2:
            f = [this](const typename T::Path& path)
            {
                static typename T::CharType c;
                for (const auto& v : path)
                {
                    const auto output_s = v.InterpretOutput();
                    fwrite(output_s, sizeof(c), CharTraits::length(output_s), output);
                }
                c = '\n';
                fwrite(&c, sizeof(c), 1, output);
                has_analyses = true;
            }; break;
        case 3:
            f = [this](const typename T::Path& path)
            {
                static typename T::CharType c;
                static typename T::string weight_s;
                typename T::Float weight = 0;
                for (const auto& v : path)
                {
                    const auto output_s = v.InterpretOutput();
                    weight += v.GetWeight();
                    fwrite(output_s, sizeof(c), CharTraits::length(output_s), output);
                }
                c = '\t';
                fwrite(&c, sizeof(c), 1, output);
                weight_s = attol::WriteFloat<typename T::CharType>(weight);
                fwrite(weight_s.c_str(), sizeof(c), weight_s.size(), output);
                c = '\n';
                fwrite(&c, sizeof(c), 1, output);
                has_analyses = true;
            }; break;
        case 4:
            f = [this](const typename T::Path& path)
            {
                static typename T::CharType c;
                static typename T::string out_s;
                auto it = path.begin();
                out_s = attol::WriteIndex<typename T::CharType>(it->GetId() + 1);
                c = ' ';
                fwrite(out_s.c_str(), sizeof(c), out_s.size(), output);
                for (++it; it != path.end(); ++it)
                {
                    fwrite(&c, sizeof(c), 1, output);
                    out_s = attol::WriteIndex<typename T::CharType>(it->GetId() + 1);
                    fwrite(out_s.c_str(), sizeof(c), out_s.size(), output);
                }
                c = '\n';
                fwrite(&c, sizeof(c), 1, output);
                has_analyses = true;
            }; break;
        case 5:
            f = [this](const typename T::Path& path)
            {
                static typename T::CharType c;
                static typename T::string out_s;
                auto it = path.begin();
                typename T::Float weight = it->GetWeight();
                out_s = attol::WriteIndex<typename T::CharType>(it->GetId() + 1);
                fwrite(out_s.c_str(), sizeof(c), out_s.size(), output);
                for (++it; it != path.end(); ++it)
                {
                    weight += it->GetWeight();
                    out_s.assign(1, ' ');
                    out_s += attol::WriteIndex<typename T::CharType>(it->GetId() + 1);
                    fwrite(out_s.c_str(), sizeof(c), out_s.size(), output);
                }
                c = '\t';
                fwrite(&c, sizeof(c), 1, output);
                c = '\n';
                out_s = attol::WriteFloat<typename T::CharType>(weight);
                fwrite(out_s.c_str(), sizeof(c), out_s.size(), output);
                fwrite(&c, sizeof(c), 1, output);
                has_analyses = true;
            }; break;
        case 6:
            f = [this](const typename T::Path& path)
            {
                static typename T::CharType c;
                auto it = path.begin();
                c = '|';
                fwrite(it->GetInput(), sizeof(c), CharTraits::length(it->GetInput()), output);
                for (++it; it != path.end(); ++it)
                {
                    fwrite(&c, sizeof(c), 1, output);
                    fwrite(it->GetInput(), sizeof(c), CharTraits::length(it->GetInput()), output);
                }
                c = '\n';
                fwrite(&c, sizeof(c), 1, output);
                has_analyses = true;
            }; break;
        case 7:
            f = [this](const typename T::Path& path)
            {
                static typename T::CharType c;
                static typename T::string out_s;
                auto it = path.begin();
                typename T::Float weight = it->GetWeight();
                c = '|';
                fwrite(it->GetInput(), sizeof(c), CharTraits::length(it->GetInput()), output);
                for (++it; it != path.end(); ++it)
                {
                    weight += it->GetWeight();
                    fwrite(&c, sizeof(c), 1, output);
                    fwrite(it->GetInput(), sizeof(c), CharTraits::length(it->GetInput()), output);
                }
                c = '\t';
                out_s = attol::WriteFloat<typename T::CharType>(weight);
                fwrite(&c, sizeof(c), 1, output);
                fwrite(out_s.c_str(), sizeof(c), out_s.size(), output);
                c = '\n';
                fwrite(&c, sizeof(c), 1, output);
                has_analyses = true;
            }; break;
        case 8:
            f = [this](const typename T::Path& path)
            {
                static typename T::CharType c;
                auto it = path.begin();
                c = '|';
                if (!T::FlagDiacriticsType::IsIt(it->GetInput()))
                    fwrite(it->GetInput(), sizeof(c), CharTraits::length(it->GetInput()), output);
                for (++it; it != path.end(); ++it)
                {
                    fwrite(&c, sizeof(c), 1, output);
                    if (!T::FlagDiacriticsType::IsIt(it->GetInput()))
                        fwrite(it->GetInput(), sizeof(c), CharTraits::length(it->GetInput()), output);
                }
                c = '\n';
                fwrite(&c, sizeof(c), 1, output);
                has_analyses = true;
            }; break;
        case 9:
            f = [this](const typename T::Path& path)
            {
                static typename T::CharType c;
                static typename T::string out_s;
                auto it = path.begin();
                typename T::Float weight = it->GetWeight();
                c = '|';
                if (!T::FlagDiacriticsType::IsIt(it->GetInput()))
                    fwrite(it->GetInput(), sizeof(c), CharTraits::length(it->GetInput()), output);
                for (++it; it != path.end(); ++it)
                {
                    weight += it->GetWeight();
                    fwrite(&c, sizeof(c), 1, output);
                    if (!T::FlagDiacriticsType::IsIt(it->GetInput()))
                        fwrite(it->GetInput(), sizeof(c), CharTraits::length(it->GetInput()), output);
                }
                c = '\t';
                out_s = attol::WriteFloat<typename T::CharType>(weight);
                fwrite(&c, sizeof(c), 1, output);
                fwrite(out_s.c_str(), sizeof(c), out_s.size(), output);
                c = '\n';
                fwrite(&c, sizeof(c), 1, output);
                has_analyses = true;
            }; break;
        default:
            f = [](const typename T::Path&) {};
        }
    }
    void Reset() { has_analyses = false; }
    bool Succeeded() const{ return has_analyses; }
    typename T::ResultHandler& GetF() { return f; }
};

}
