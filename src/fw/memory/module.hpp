#ifndef UNHIDENPCS_MODULE_HPP
#define UNHIDENPCS_MODULE_HPP
#pragma once
#include "range.hpp"
#include "Zydis/Zydis.h"

namespace memory
{
    class Module : public Range
    {
    private:
        HMODULE               _hModule {};
        std::string           _name {};
        std::filesystem::path _path {};

        bool               _sectionsInitialized {};
        std::vector<Range> _textSections {};
        std::vector<Range> _dataSections {};

        bool                                                   _ripRelativeInitialized {};
        std::unordered_map<uintptr_t, ZydisDecodedInstruction> _ripRelativeInstructions {};

        bool                                                       _refStringsInitialized {};
        std::unordered_map<uintptr_t, std::vector<memory::Handle>> _refStringsAscii {};
        std::unordered_map<uintptr_t, std::vector<memory::Handle>> _refStringsUtf16 {};

        void initRipRelativeIndex();
        void initSections();
        void initRefStrings();

    public:
        [[nodiscard]] HMODULE handle() const;

        const std::string& name();

        const std::filesystem::path& path();

        bool findPattern(const std::string& pattern, memory::Handle& result);

        bool findString(const std::string& string, memory::Handle& result);

        bool findWstring(const std::wstring& string, memory::Handle& result);

        bool findStringReference(const std::string& string, Handle& result);
        bool findStringReferences(const std::string& string, std::vector<Handle>& results, int max = 0);
        bool findWstringReference(const std::wstring& string, Handle& result);
        bool findWstringReferences(const std::wstring& string, std::vector<Handle>& results, int max = 0);

        [[nodiscard]] const std::vector<Range>& textSections();
        [[nodiscard]] const std::vector<Range>& dataSections();

        bool getDataSection(const Handle& handle, Range& result);

        const std::unordered_map<uintptr_t, ZydisDecodedInstruction>& ripRelativeInstructions();

        const std::unordered_map<uintptr_t, std::vector<Handle>>& refStringsAscii();
        const std::unordered_map<uintptr_t, std::vector<Handle>>& refStringsUtf16();

        bool isInCodeSection(const Handle& handle);
        bool isInDataSection(const Handle& handle);

        bool findReferencesTo(const Handle& handle, std::vector<Handle>& results, int max = 0);

        static Module getFromHandle(HMODULE hModule);

        static bool tryGetByName(const std::string& name, Module& result);

        static Module getByName(const std::string& name);

        static bool tryGetByAddr(const memory::Handle& addr, Module& result);

        static Module getMain();

        static Module getThis();
    };
}

#endif //UNHIDENPCS_MODULE_HPP
