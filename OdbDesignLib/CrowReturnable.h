#pragma once

#include "crow.h"


class CrowReturnable : public crow::returnable
{
    CrowReturnable()
        : returnable("text/plain")
    {}

    // 
    virtual std::string as_string() const = 0;

    std::string dump() const override
    {
        return this->as_string();
    }
};
