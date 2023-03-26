#pragma once

#if defined(_WIN32) || defined(WIN32)
#define CR_DLLPUBLIC_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define CR_DLLPUBLIC_EXPORT __attribute__((visibility("default")))
#endif
