/*
 * Wazuh SYSINFO
 * Copyright (C) 2015, Wazuh Inc.
 * January 28, 2021.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef _PACKAGE_LINUX_PARSER_HELPER_H
#define _PACKAGE_LINUX_PARSER_HELPER_H

#include "stringHelper.h"
#include "json.hpp"
#include "timeHelper.h"
#include "sharedDefs.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#include <sstream>

// Parse helpers for standard Linux packaging systems (rpm, dpkg, ...)
namespace PackageLinuxHelper
{

    static nlohmann::json parseDpkg(const std::vector<std::string>& entries)
    {
        std::map<std::string, std::string> info;
        nlohmann::json ret;

        for (const auto& entry : entries)
        {
            const auto pos{entry.find(":")};

            if (pos != std::string::npos)
            {
                const auto key{Utils::trim(entry.substr(0, pos))};
                const auto value{Utils::trim(entry.substr(pos + 1), " \n")};
                info[key] = value;
            }
        }

        if (!info.empty() && info.at("Status") == "install ok installed")
        {
            ret["name"] = info.at("Package");

            std::string priority;
            std::string groups;
            std::string multiarch;
            std::string architecture;
            std::string source;
            std::string version;
            std::string vendor;
            std::string description;
            int size                 { 0 };

            auto it{info.find("Priority")};

            if (it != info.end())
            {
                priority = it->second;
            }

            it = info.find("Section");

            if (it != info.end())
            {
                groups = it->second;
            }

            it = info.find("Installed-Size");

            if (it != info.end())
            {
                size = stol(it->second);
            }

            it = info.find("Multi-Arch");

            if (it != info.end())
            {
                multiarch = it->second;
            }

            it = info.find("Architecture");

            if (it != info.end())
            {
                architecture = it->second;
            }

            it = info.find("Source");

            if (it != info.end())
            {
                source = it->second;
            }

            it = info.find("Version");

            if (it != info.end())
            {
                version = it->second;
            }

            it = info.find("Maintainer");

            if (it != info.end())
            {
                vendor = it->second;
            }

            it = info.find("Description");

            if (it != info.end())
            {
                description = Utils::substrOnFirstOccurrence(it->second, "\n");
            }

            ret["priority"]     = priority;
            ret["groups"]       = groups;
            ret["size"]         = size;
            ret["multiarch"]    = multiarch;
            ret["architecture"] = architecture;
            ret["source"]       = source;
            ret["version"]      = version;
            ret["format"]       = "deb";
            ret["vendor"]       = vendor;
            ret["description"]  = description;
        }

        return ret;
    }

    static nlohmann::json parseSnap(const nlohmann::json& info)
    {
        nlohmann::json ret;

        std::string name;
        std::string version;
        std::string vendor       { UNKNOWN_VALUE };
        std::string install_time { UNKNOWN_VALUE };
        std::string description  { UNKNOWN_VALUE };
        int         size         { 0 };
        bool        hasName      { false };
        bool        hasVersion   { false };

        if (info.contains("name"))
        {
            name = info.at("name");

            if (name.length())
            {
                hasName = true;
            }
        }

        if (info.contains("version"))
        {
            version = info.at("version");

            if (version.length())
            {
                hasVersion = true;
            }
        }

        if (!(hasVersion && hasName))
        {
            ret.clear();
            return ret;
        }

        if (info.contains("publisher"))
        {
            auto& publisher = info.at("publisher");

            if (publisher.contains("display-name"))
            {
                vendor = publisher.at("display-name");
            }
        }

        if (info.contains("install-date"))
        {
            struct std::tm tm;
            std::istringstream iss(info.at("install-date").get<std::string>());
            iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y/%m/%d %H:%M:%S");
            install_time = oss.str();
        }

        if (info.contains("summary"))
        {
            description = info.at("summary");
        }

        if (info.contains("installed-size"))
        {
            auto& data = info.at("installed-size");

            if (data.is_number())
            {
                size = data.get<int>();
            }
            else if (data.is_string())
            {
                auto stringData = data.get_ref<const std::string&>();

                if (stringData.length())
                {
                    try
                    {
                        size = std::stoi( stringData );
                    }
                    catch (const std::exception& e)
                    {
                        size = 0;
                    }
                }
            }
        }

        ret["name"]             = name;
        ret["location"]         = "/snap/" + name;
        ret["version"]          = version;
        ret["vendor"]           = vendor;
        ret["install_time"]     = install_time;
        ret["description"]      = description;
        ret["size"]             = size;
        ret["source"]           = "snapcraft";
        ret["format"]           = "snap";

        ret["priority"]         = UNKNOWN_VALUE;
        ret["multiarch"]        = UNKNOWN_VALUE;
        ret["architecture"]     = UNKNOWN_VALUE;
        ret["groups"]           = UNKNOWN_VALUE;

        return ret;
    }


};

#pragma GCC diagnostic pop

#endif // _PACKAGE_LINUX_PARSER_HELPER_H
