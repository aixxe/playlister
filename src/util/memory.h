#pragma once

#include <windows.h>

namespace util::memory
{
    class page_guard
    {
        public:
            page_guard(void* target, std::size_t size, DWORD rights):
                _target(target), _new_rights(rights), _size(size)
            {
                VirtualProtect(_target, _size, _new_rights, &_old_rights);
            }

            ~page_guard()
                { VirtualProtect(_target, _size, _old_rights, &_old_rights); }
        private:
            void* _target = nullptr;
            DWORD _old_rights = 0;
            DWORD _new_rights = 0;
            std::size_t _size = 0;
    };

    class code_patch
    {
        public:
            template <typename... T> explicit code_patch(void* target, T... bytes):
                _target(target), _size(sizeof...(T)), _original(_size)
            {
                _patch = { static_cast<std::uint8_t>(bytes)... };
                std::memcpy(_original.data(), _target, _size);
            }

            auto enable() const -> void
            {
                auto guard = page_guard {_target, _size, PAGE_EXECUTE_READWRITE };
                std::memcpy(_target, _patch.data(), _size);
            }

            auto disable() const -> void
            {
                auto guard = page_guard {_target, _size, PAGE_EXECUTE_READWRITE };
                std::memcpy(_target, _original.data(), _size);
            }
        private:
            void* _target = nullptr;
            std::size_t _size = 0;
            std::vector<std::uint8_t> _original = {};
            std::vector<std::uint8_t> _patch = {};
    };
}