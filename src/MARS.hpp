/**
 * \file MARS.hpp
 * \author Malte Langosz
 *
 */

#pragma once

#include <iostream>
#include <string>

//#include <envire_visualizer/EnvireGraphVisualizer.hpp>

namespace lib_manager
{
    class LibManager;
}

namespace mars
{


    namespace interfaces
    {
        class ControlCenter;
    }

    namespace interfaces
    {
        class MarsGuiInterface;
    }

    namespace app
    {
#ifdef NO_GUI
        class GraphicsTimer {};
#else
        class GraphicsTimer;
#endif

        void exit_main(int signal);
        void handle_abort(int signal);

        class MARS
        {
        public:
            MARS();
            MARS(lib_manager::LibManager *theManager);
            ~MARS();

            void readArguments(int argc, char **argv);
            void init();
            void start(int argc, char **argv, bool startThread = true,
                       bool handleLibraryLoading = true);
            void loadCoreLibs();
            void loadAdditionalLibs();
            int runWoQApp();
            inline lib_manager::LibManager* getLibManager() {return libManager;}

            // TODO: this function for debug purposes, and should be taken removed later
            //void saveOSGGraph();

            static interfaces::ControlCenter *control;
            static bool quit;
            std::string configDir;
            std::string coreConfigFile;
            bool needQApp, noGUI;

        private:
            //std::shared_ptr<envire::viz::EnvireGraphVisualizer> graphVisualizer;

            void releaseEnvireLibs();

            lib_manager::LibManager *libManager;
            app::GraphicsTimer *graphicsTimer;
            interfaces::MarsGuiInterface *marsGui;
            bool ownLibManager;
            bool argConfDir;
            bool initialized;
        };

    } // end of namespace app
} // end of namespace mars
