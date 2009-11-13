// Copyright (C) 2008, INRIA
// Author(s): Marc Fragu
//
// This file is part of the data assimilation library Verdandi.
//
// Verdandi is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your option)
// any later version.
//
// Verdandi is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
// more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Verdandi. If not, see http://www.gnu.org/licenses/.


#ifndef VERDANDI_FILE_SHARE_LOGGER_HXX

#include <ctime>
#include <iostream>
#include <map>


#ifndef VERDANDI_LOG_FILENAME
#define VERDANDI_LOG_FILENAME "verdandi_%{D}.log"
#endif
#ifndef VERDANDI_LOGGING_LEVEL
#define VERDANDI_LOGGING_LEVEL 2
#endif
#ifndef VERDANDI_LOG_WIDTH
#define VERDANDI_LOG_WIDTH 78
#endif
#ifndef VERDANDI_LOG_OPTIONS
#define VERDANDI_LOG_OPTIONS (stdout_ | file_)
#endif


namespace Verdandi
{


    //! Logging class.
    class Logger
    {

    public :

        // Flags associated with the requested logger options. Each flag is a
        // mask to be applied to 'options_' in order to extract the bit
        // associated with the flag's option.
        static const int stdout_ = 1, file_ = 2, uppercase_ = 4;

    private :

        /*** Log file ***/

        //! Characters per line.
        static const unsigned int width_ = VERDANDI_LOG_WIDTH;
        //! Log file path.
        static string file_name_;

        /*** Options ***/

        /*! \brief Integer in which each bit determines whether or not an
          option is activated. */
        static int options_;
        /*! \brief This value is used to reinitialize the options to their
          initial value. */
        static int default_options_;

        /*** Logging level ***/

        //! Level of verbosity in the log file.
        static int logging_level_;
        //! Default verbosity level.
        static const int default_logging_level = VERDANDI_LOGGING_LEVEL;

        typedef void (Logger::*LogCommand)(string, int);
        typedef map<string, LogCommand> CommandMap;
        //! List of the commands proposed by the 'Log' method.
        static CommandMap command_;

    public :

        /*** Initialization the static class ***/

        static void Initialize();
        static void Initialize(string configuration_file,
                               string section_name);
        static void Finalize();

        /*** Static public methods ***/

        static void InitializeOptions();

        static void SetOption(int option, bool value);
        static void SetStdout(bool value);
        static void SetFile(bool value);
        static void SetUppercase(bool value);

        static void SetLoggingLevel(int level);

        template <int LEVEL, class T>
        static void Log(const T& object, string message,
                        int options = options_);
        template <class T>
        static void Log(const T& object, string message,
                        int options = options_);

        static void Command(string command, string parameter,
                            int options = options_);

    private :

        static void InitializeDefaultOptions();
        static void InitializeDefaultOptions(string configuration_file,
                                             string section_name);
        static void InitializeFilename();
        static void InitializeFilename(string configuration_file,
                                       string section_name);
        static void InitializeLevel();
        static void InitializeLevel(string configuration_file,
                                    string section_name);
        static void InitializeCommand();

        template <class T>
        static void WriteMessage(const T& object, string message,
                                 int options);
        static void WriteMessage(const char* object, string message,
                                 int options);
        static void WriteMessage(string message, int options);

        static string FormatMessage(string object_name, string message);
        static string GenerateDate();

        /*** Logger specific commands ***/

        void HlineCommand(string parameter, int options);

    };


} // namespace Verdandi.


#define VERDANDI_FILE_SHARE_LOGGER_HXX
#endif
