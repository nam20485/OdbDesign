#pragma once

#include <string>
#include <set>
#include <map>
#include <vector>
#include <atomic>
#include <sstream>
#include "../../odbdesign_export.h"

namespace Odb::Lib::FileModel::Design
{
    // Utility class for tracing component height data flow
    // Tracks specific components (configurable list) and first N failures
    class ODBDESIGN_EXPORT ComponentHeightTracer
    {
    public:
        static ComponentHeightTracer &instance();

        // Check if a component should be traced
        bool shouldTrace(const std::string &compName, unsigned int pkgRef) const;

        // Check if this is a failure case (for first N failures tracking)
        bool shouldTraceFailure() const;

        // Mark a failure (increments failure counter) - const because atomic is thread-safe
        void markFailure() const;

        // Logging helpers
        void logParseStart(const std::string &compName, unsigned int pkgRef, const std::string &attrIdString) const;
        void logParseResult(const std::string &compName, unsigned int pkgRef,
                            const std::map<std::string, std::string> &lookupTable,
                            const std::vector<std::string> &attributeNames) const;
        void logSerialization(const std::string &compName, unsigned int pkgRef,
                              const std::map<std::string, std::string> &lookupTable,
                              const std::vector<std::string> &attributeNames) const;
        void logGrpcResponse(const std::string &compName, unsigned int pkgRef,
                             const std::map<std::string, std::string> &lookupTable,
                             const std::vector<std::string> &attributeNames) const;
        void logRestResponse(const std::string &compName, unsigned int pkgRef,
                             const std::map<std::string, std::string> &lookupTable,
                             const std::vector<std::string> &attributeNames) const;

        // Configuration
        void addComponentToTrace(const std::string &compName);
        void setMaxFailuresToTrace(int maxFailures);

    private:
        ComponentHeightTracer();
        ~ComponentHeightTracer() = default;

        std::set<std::string> m_componentsToTrace;
        mutable std::atomic<int> m_failureCount{0};
        int m_maxFailuresToTrace{20};

        std::string formatLookupTable(const std::map<std::string, std::string> &lookupTable) const;
        std::string formatAttributeNames(const std::vector<std::string> &attributeNames) const;
        std::string getCompKey(const std::string &compName, unsigned int pkgRef) const;
    };
}
