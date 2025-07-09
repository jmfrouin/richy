/**
 * This file is part of sqlprox project.
 * Copyright (C) 2023,2024,2025 FROUIN Jean-Michel
 */

/*
 *@brief : Commons definition file.
 *@version 31/08/2023
 *@author Jean-Michel Frouin (jm@frouin.me)
 */
#ifndef DEF_H
#define DEF_H

#include <sstream>
#include "version.h"

//Bash Colors
#define GREEN "\e[0;32m"
#define RED "\e[0;31m"
#define BLUE "\e[0;34m"
#define VIOLET "\e[0;35m"
#define CYAN "\e[0;36m"
#define WHITE "\e[0;37m"
#define YELLOW "\e[0;33m"
#define STOP "\e[0m"

//General app infos
#define NAME "richy"
#define FULLNAME NAME " " VERSION_INFO_STRING
#define VERSION_FILELOG 1.0

//Paths
#define LOG_FILE NAME ".log"
#define CLIENT_CONF_FILE NAME ".conf"

inline int atoi(const std::string &v)
{
    int Temp;
    std::istringstream(v) >> Temp;
    return Temp;
}

//Network stuff
const int BUFFER_SIZE = 4096;

#endif // DEF_H
/* vi:set ts=4: */
