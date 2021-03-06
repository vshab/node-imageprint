#ifndef XPS_THROWABLE_HRESULT_HPP
#define XPS_THROWABLE_HRESULT_HPP

#include <stdexcept>

#include <winnt.h>

// Syntax sugar to work with HRESULT. Throws exception when assigned FAILED hr.
// The idea is taken from http://www.codeproject.com/Articles/11679/HRESULT-Error-Check-Simplifier
class ThrowableHResult
{

public:

    // Exception class
    class HResultFailed:
        public std::runtime_error
    {

    public:
        HResultFailed():
            runtime_error("HRESULT failed")
        {
        }

        virtual ~HResultFailed()
        {
        }
    };

public:

    ThrowableHResult():
        hr(S_OK)
    {
    }

    ThrowableHResult& operator=(HRESULT hr)
    {
        this->hr = hr;

        if (FAILED(hr))
        {
            throw HResultFailed();
        }

        return *this;
    }

private:

    HRESULT hr;
};

#endif
