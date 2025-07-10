//
// Created by Jean-Michel Frouin on 09/07/2025.
//

#ifndef CONFIG_H
#define CONFIG_H

#include "def.h"
#define CONF_FILE NAME ".conf"

namespace Richy {
    enum EErrors {
        eNoError = 0,
        //Generic
        eCannotOpenFile = 1,
        eCannotDeleteLogFile = 2,
        eConfEmpty = 3,
        eCannotOpenPath = 4,
        eAccessDenied = 5,
        eUnknown = 6,
        eCannotFork = 7,
    };

    class CConfiguration {
        public:
            CConfiguration();
            ~CConfiguration();

            EErrors Load();
            EErrors Save();

        public:
            //HTTP Conf
            std::string mHost;
            std::string mAPIKey;
            std::string mAPISecret;
    };
}

#endif //CONFIG_H