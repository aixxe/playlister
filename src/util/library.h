#pragma once

namespace util
{
    class library
    {
        public:
            library() = default;

            explicit library(void* base)
            {
                if (!base)
                    return;

                _base = static_cast<std::uint8_t*>(base);

                auto const dos = reinterpret_cast<PIMAGE_DOS_HEADER>(_base);
                auto const nt = reinterpret_cast<PIMAGE_NT_HEADERS>(_base + dos->e_lfanew);

                _size = nt->OptionalHeader.SizeOfImage;
            }

            auto static from_name(const std::string& name) -> library
                { return library { GetModuleHandleA(name.c_str()) }; }

            [[nodiscard]] std::uint8_t* start() const
                { return _base; }

            [[nodiscard]] std::uint8_t* end() const
                { return _base + _size; }

            [[nodiscard]] std::size_t size() const
                { return _size; }

            [[nodiscard]] std::uint8_t* operator*() const
                { return _base; }

            [[nodiscard]] std::uint8_t* operator+(std::ptrdiff_t offset) const
                { return _base + offset; }

            [[nodiscard]] std::uint8_t* operator-(std::ptrdiff_t offset) const
                { return _base - offset; }
        private:
            std::size_t _size = 0;
            std::uint8_t* _base = nullptr;
    };
}