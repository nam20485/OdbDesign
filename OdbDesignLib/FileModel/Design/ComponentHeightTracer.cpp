#include "ComponentHeightTracer.h"
#include <Logger.h>
#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace Odb::Lib::FileModel::Design
{
    ComponentHeightTracer &ComponentHeightTracer::instance()
    {
        static ComponentHeightTracer tracer;
        return tracer;
    }

    ComponentHeightTracer::ComponentHeightTracer()
    {
        // Default: empty trace set. Use addComponentToTrace() or set
        // ODBDESIGN_TRACE_COMPONENTS env var (comma-separated) to enable.
        const char* envComponents = std::getenv("ODBDESIGN_TRACE_COMPONENTS");
        if (envComponents != nullptr && envComponents[0] != '\0')
        {
            std::istringstream stream(envComponents);
            std::string name;
            while (std::getline(stream, name, ','))
            {
                // Trim whitespace
                const auto firstNonWs = name.find_first_not_of(" \t");
                if (firstNonWs == std::string::npos)
                {
                    name.clear();
                }
                else
                {
                    const auto lastNonWs = name.find_last_not_of(" \t");
                    name = name.substr(firstNonWs, lastNonWs - firstNonWs + 1);
                }
                if (!name.empty())
                {
                    m_componentsToTrace.insert(name);
                }
            }
        }

        const char* envMaxFailures = std::getenv("ODBDESIGN_TRACE_MAX_FAILURES");
        if (envMaxFailures != nullptr)
        {
            try
            {
                m_maxFailuresToTrace = std::stoi(envMaxFailures);
                // Validate bounds
                if (m_maxFailuresToTrace <= 0)
                {
                    logwarn("[ComponentHeightTracer] ODBDESIGN_TRACE_MAX_FAILURES must be positive, using default value 20");
                    m_maxFailuresToTrace = 20;
                }
            }
            catch (...)
            {
                logwarn("[ComponentHeightTracer] Failed to parse ODBDESIGN_TRACE_MAX_FAILURES env var, using default value 20");
                m_maxFailuresToTrace = 20;
            }
        }
    }

    bool ComponentHeightTracer::shouldTrace(const std::string &compName, unsigned int pkgRef) const
    {
        // Check if component is in trace list
        if (m_componentsToTrace.find(compName) != m_componentsToTrace.end())
        {
            return true;
        }
        // Check if we should trace failures (first N failures)
        return shouldTraceFailure();
    }

    bool ComponentHeightTracer::shouldTraceFailure() const
    {
        return m_failureCount.load() < m_maxFailuresToTrace;
    }

    void ComponentHeightTracer::markFailure() const
    {
        m_failureCount.fetch_add(1);
    }

    std::string ComponentHeightTracer::getCompKey(const std::string &compName, unsigned int pkgRef) const
    {
        std::stringstream ss;
        ss << compName << "(Package=" << pkgRef << ")";
        return ss.str();
    }

    std::string ComponentHeightTracer::formatLookupTable(const std::map<std::string, std::string> &lookupTable) const
    {
        std::stringstream ss;
        ss << "{";
        bool first = true;
        for (const auto &kv : lookupTable)
        {
            if (!first)
                ss << ", ";
            ss << "\"" << kv.first << "\":\"" << kv.second << "\"";
            first = false;
        }
        ss << "}";
        return ss.str();
    }

    std::string ComponentHeightTracer::formatAttributeNames(const std::vector<std::string> &attributeNames) const
    {
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < attributeNames.size(); ++i)
        {
            if (i > 0)
                ss << ", ";
            ss << i << ":\"" << attributeNames[i] << "\"";
        }
        ss << "]";
        return ss.str();
    }

    void ComponentHeightTracer::logParseStart(const std::string &compName, unsigned int pkgRef, const std::string &attrIdString) const
    {
        if (!shouldTrace(compName, pkgRef))
            return;

        std::stringstream ss;
        ss << "[CompHeightTrace][PARSE_START] Component=" << compName
           << ", Package=" << pkgRef
           << ", attrIdString=\"" << attrIdString << "\"";
        loginfo(ss.str());
    }

    void ComponentHeightTracer::logParseResult(const std::string &compName, unsigned int pkgRef,
                                               const std::map<std::string, std::string> &lookupTable,
                                               const std::vector<std::string> &attributeNames) const
    {
        if (!shouldTrace(compName, pkgRef))
            return;

        std::stringstream ss;
        ss << "[CompHeightTrace][PARSE_RESULT] Component=" << compName
           << ", Package=" << pkgRef
           << ", AttributeNames=" << formatAttributeNames(attributeNames)
           << ", AttributeLookupTable=" << formatLookupTable(lookupTable);

        // Check if .comp_height is at index 0 and if key "0" exists
        bool hasCompHeightAttr = !attributeNames.empty() && attributeNames[0] == ".comp_height";
        bool hasKey0 = lookupTable.find("0") != lookupTable.end();

        ss << ", .comp_height@index0=" << (hasCompHeightAttr ? "YES" : "NO")
           << ", key\"0\"_exists=" << (hasKey0 ? "YES" : "NO");

        if (hasCompHeightAttr && !hasKey0)
        {
            ss << " ⚠️ MISSING_KEY_0";
            markFailure();
        }
        else if (hasKey0)
        {
            ss << ", height_value=\"" << lookupTable.at("0") << "\"";
        }

        loginfo(ss.str());
    }

    void ComponentHeightTracer::logSerialization(const std::string &compName, unsigned int pkgRef,
                                                 const std::map<std::string, std::string> &lookupTable,
                                                 const std::vector<std::string> &attributeNames) const
    {
        if (!shouldTrace(compName, pkgRef))
            return;

        std::stringstream ss;
        ss << "[CompHeightTrace][SERIALIZE] Component=" << compName
           << ", Package=" << pkgRef
           << ", AttributeNames=" << formatAttributeNames(attributeNames)
           << ", AttributeLookupTable=" << formatLookupTable(lookupTable);

        bool hasKey0 = lookupTable.find("0") != lookupTable.end();
        if (hasKey0)
        {
            ss << ", height_value=\"" << lookupTable.at("0") << "\"";
        }
        else
        {
            ss << ", key\"0\"_missing";
        }

        loginfo(ss.str());
    }

    void ComponentHeightTracer::logGrpcResponse(const std::string &compName, unsigned int pkgRef,
                                                const std::map<std::string, std::string> &lookupTable,
                                                const std::vector<std::string> &attributeNames) const
    {
        if (!shouldTrace(compName, pkgRef))
            return;

        std::stringstream ss;
        ss << "[CompHeightTrace][GRPC_RESPONSE] Component=" << compName
           << ", Package=" << pkgRef
           << ", AttributeNames=" << formatAttributeNames(attributeNames)
           << ", AttributeLookupTable=" << formatLookupTable(lookupTable);

        bool hasKey0 = lookupTable.find("0") != lookupTable.end();
        if (hasKey0)
        {
            ss << ", height_value=\"" << lookupTable.at("0") << "\"";
        }
        else
        {
            ss << ", key\"0\"_missing";
        }

        loginfo(ss.str());
    }

    void ComponentHeightTracer::logRestResponse(const std::string &compName, unsigned int pkgRef,
                                                const std::map<std::string, std::string> &lookupTable,
                                                const std::vector<std::string> &attributeNames) const
    {
        if (!shouldTrace(compName, pkgRef))
            return;

        std::stringstream ss;
        ss << "[CompHeightTrace][REST_RESPONSE] Component=" << compName
           << ", Package=" << pkgRef
           << ", AttributeNames=" << formatAttributeNames(attributeNames)
           << ", AttributeLookupTable=" << formatLookupTable(lookupTable);

        bool hasKey0 = lookupTable.find("0") != lookupTable.end();
        if (hasKey0)
        {
            ss << ", height_value=\"" << lookupTable.at("0") << "\"";
        }
        else
        {
            ss << ", key\"0\"_missing";
        }

        loginfo(ss.str());
    }

    void ComponentHeightTracer::addComponentToTrace(const std::string &compName)
    {
        m_componentsToTrace.insert(compName);
    }

    void ComponentHeightTracer::setMaxFailuresToTrace(int maxFailures)
    {
        m_maxFailuresToTrace = maxFailures;
    }
}
