/**
 * \file MARS.cpp
 * \author Malte Langosz
 *
 */

#include "MARS.hpp"

#ifndef NO_GUI
#include "GraphicsTimer.hpp"
#include <main_gui/MainGUI.h>
#endif

#include <lib_manager/LibManager.hpp>
#include <lib_manager/LibInterface.hpp>
#include <mars_interfaces/sim/SimulatorInterface.h>
#include <mars_interfaces/gui/MarsGuiInterface.h>
#include <mars_interfaces/sim/ControlCenter.h>
#include <mars_utils/Thread.h>
#include <mars_utils/misc.h>
#include <cfg_manager/CFGManagerInterface.h>

//#include <envire_visualizer/EnvireGraphVisualizer.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

#include <getopt.h>
#include <signal.h>

#ifndef DEFAULT_CONFIG_DIR
#define DEFAULT_CONFIG_DIR "."
#endif

namespace mars
{

    namespace app
    {

        mars::interfaces::ControlCenter* MARS::control = nullptr;
        bool MARS::quit = false;

        void exit_main(int signal)
        {
#ifndef WIN32
            if(signal == SIGPIPE)
            {
                return;
            }
#endif
            MARS::quit = true;
            if (signal)
            {
                fprintf(stderr, "\nI think we exit with an error! Signal: %d\n", signal);
                //MARS::control->sim->exitMars();
                //exit(-1);
                //MARS::control->sim->exitMars();
                //Convention: print the signal type
            }
#ifndef NO_GUI
            if(qApp) qApp->quit();
#endif
        }

        void handle_abort(int signal)
        {
            MARS::control->sim->handleError(interfaces::PHYSICS_DEBUG);
        }

        MARS::MARS() : configDir(DEFAULT_CONFIG_DIR),
                       libManager(new lib_manager::LibManager()),
                       marsGui(nullptr), ownLibManager(true),
                       argConfDir(false) {
            needQApp = true;
            noGUI = false;
            graphicsTimer = nullptr;
            initialized = false;
#ifdef WIN32
            // request a scheduler of 1ms
            timeBeginPeriod(1);
#endif //WIN32
        }

        void MARS::releaseEnvireLibs()
        {
        }

        MARS::MARS(lib_manager::LibManager *theManager) : configDir(DEFAULT_CONFIG_DIR),
                                                          libManager(theManager),
                                                          marsGui(nullptr), ownLibManager(false),
                                                          argConfDir(false)
        {
            needQApp = true;
            noGUI = false;
            graphicsTimer = nullptr;
            initialized = false;
#ifdef WIN32
            // request a scheduler of 1ms
            timeBeginPeriod(1);
#endif //WIN32
        }

        MARS::~MARS()
        {
            //! close simulation
            MARS::control->sim->exitMars();


            if(graphicsTimer) delete graphicsTimer;

            libManager->releaseLibrary("mars_core");
            if(marsGui) libManager->releaseLibrary("mars_gui");
            libManager->releaseLibrary("main_gui");
            libManager->releaseLibrary("cfg_manager");
            libManager->releaseLibrary("mars_graphics");
            if(ownLibManager) delete libManager;

#ifdef WIN32
            // end scheduler of 1ms
            timeEndPeriod(1);
#endif //WIN32
        }

        void MARS::init()
        {
            // then check locals
#ifndef WIN32
            setenv("LC_ALL","C", 1);
            unsetenv("LANG");
            setlocale(LC_ALL,"C");
#endif

            // Test if current locale supports ENGLISH number interpretation
            struct lconv *locale = localeconv();
            fprintf(stderr, "Active locale (LC_ALL): ");
            if( *(locale->decimal_point) != '.')
            {
                fprintf(stderr, " [FAIL] Current locale conflicts with mars\n");
                exit(1);
            } else
            {
                fprintf(stderr, " [OK]\n");
            }

            // load the simulation core_libs:
            if(!argConfDir)
            {
                FILE *testFile = fopen("core_libs.txt", "r");
                if(testFile)
                {
                    configDir = ".";
                    fclose(testFile);
                }
            }

            // we always need the cfg_manager to setup configurations correctly
            libManager->loadLibrary("cfg_manager");
            mars::cfg_manager::CFGManagerInterface *cfg;
            cfg = libManager->getLibraryAs<mars::cfg_manager::CFGManagerInterface>("cfg_manager");
            if(cfg)
            {
                cfg_manager::cfgPropertyStruct configPath, prefPath;
                configPath = cfg->getOrCreateProperty("Config", "config_path",
                                                      configDir);
                prefPath = cfg->getOrCreateProperty("Preferences", "resources_path",
                                                    "");
                if(prefPath.sValue == "")
                {
                    prefPath.sValue = "";
                }
                //cfg->setProperty(prefPath);
                // load preferences
                std::string loadFile = configDir + "/mars_Preferences.yaml";
                cfg->loadConfig(loadFile.c_str());
            }
            initialized = true;
        }

        void MARS::loadCoreLibs()
        {
            FILE *plugin_config;
            coreConfigFile = configDir+"/core_libs.txt";
            plugin_config = fopen(coreConfigFile.c_str() , "r");
            if(plugin_config)
            {
                fprintf(stderr, "MARS::loadCoreLibs: load core libs from core_libs.txt\n");
                fclose(plugin_config);
                libManager->loadConfigFile(coreConfigFile);
            } else
            {
                fprintf(stderr, "MARS::loadCoreLibs: Loading default core libraries...\n");
                libManager->loadLibrary("data_broker");
                libManager->loadLibrary("mars_core");
                if(!noGUI)
                {
                    libManager->loadLibrary("main_gui");
                    libManager->loadLibrary("mars_gui");
                }
            }
        }

        void MARS::loadAdditionalLibs()
        {
            {
                fprintf(stderr, "MARS::loadAdditionalLibs: Loading default additional libraries...\n");
                // loading errors will be silent for the following optional libraries
                if(!noGUI)
                {
                    // libManager->loadLibrary("log_console", nullptr, true);
                    // libManager->loadLibrary("connexion_plugin", nullptr, true);
                    libManager->loadLibrary("data_broker_gui", nullptr, true);
                    libManager->loadLibrary("cfg_manager_gui", nullptr, true);
                    libManager->loadLibrary("lib_manager_gui", nullptr, true);
                    // libManager->loadLibrary("SkyDomePlugin", nullptr, true);
                    // libManager->loadLibrary("CameraGUI", nullptr, true);
                    // libManager->loadLibrary("PythonMars", nullptr, true);
                    // libManager->loadLibrary("data_broker_plotter2", nullptr, true);
                    libManager->loadLibrary("envire_mars_graphics");
                }
                std::cout << "load envire plugins" << std::endl;
                libManager->loadLibrary("envire_mars_ode_physics");
                libManager->loadLibrary("envire_mars_ode_collision");
                libManager->loadLibrary("envire_mars_motors");
                libManager->loadLibrary("envire_mars_sensors");
                libManager->loadLibrary("mars_scene_loader", nullptr, true);
            }
        }

        // TODO: this function for debug purposes, and should be taken removed later
        // void MARS::saveOSGGraph() {
        //     std::string filename = "/opt/workspace/test.osg";
        //     if (osgDB::writeNodeFile(*graphVisualizer->getOSGRootNode(), filename))
        //        std::cout << "---------------SUCCESS " << std::endl;
        //     else
        //        std::cout << "---------------FAIL" << std::endl;
        // }

        void MARS::start(int argc, char **argv, bool startThread,
                         bool handleLibraryLoading)
        {

            if(!initialized) init();

            if(handleLibraryLoading)
            {
                loadCoreLibs();
            }

            // then get the simulation
            mars::interfaces::SimulatorInterface *marsSim;
            marsSim = libManager->getLibraryAs<mars::interfaces::SimulatorInterface>("mars_core");
            if(!marsSim)
            {
                fprintf(stderr, "main: error while casting simulation lib\n\n");
                exit(2);
            }
            control = marsSim->getControlCenter();
            // then read the simulation arguments
            control->sim->readArguments(argc, argv);

            marsGui = libManager->getLibraryAs<mars::interfaces::MarsGuiInterface>("mars_gui");
            if(marsGui)
            {
                marsGui->setupGui();
            }

#ifndef NO_GUI
            mars::main_gui::MainGUI *mainGui = nullptr;
            mainGui = libManager->getLibraryAs<mars::main_gui::MainGUI>("main_gui", true);

            // TODO: added graph visualizer for 3d view, it should be move later in other lib/plugin
            // graphVisualizer = std::make_shared<envire::viz::EnvireGraphVisualizer>();
            // graphVisualizer->init(marsSim->getGraph(), marsSim->getRootFrame());
            // graphVisualizer->setTransformer(false);
            // graphVisualizer->setAxes(false);
            // graphVisualizer->enablePlugin("smurf::Collidable", false);
            // mainGui->mainWindow_p()->setCentralWidget(graphVisualizer.get());

            mars::interfaces::GraphicsManagerInterface *marsGraphics = nullptr;
            marsGraphics = libManager->getLibraryAs<mars::interfaces::GraphicsManagerInterface>("mars_graphics", true);
            if(marsGraphics)
            {
                // init osg
                //initialize graphicsFactory
                if(mainGui)
                {
                    marsGraphics->initializeOSG(nullptr);
                    QWidget *widget = (QWidget*)marsGraphics->getQTWidget(1);
                    if (widget)
                    {
                        mainGui->mainWindow_p()->setCentralWidget(widget);
                    }
                } else
                {
                    marsGraphics->initializeOSG(nullptr, false);
                }
            }
#endif

            // load the simulation other_libs:
            if(handleLibraryLoading)
            {
                std::string otherConfigFile = configDir+"/other_libs.txt";
                FILE *plugin_config = fopen(otherConfigFile.c_str() , "r");
                if(plugin_config)
                {
                    fclose(plugin_config);
                    libManager->loadConfigFile(otherConfigFile);
                } else
                {
                    loadAdditionalLibs();
                }
            }

#ifndef NO_GUI
            // if we have a main gui, show it
            if(mainGui) mainGui->show();
#endif

            control->sim->runSimulation(startThread);
            graphicsTimer = new mars::app::GraphicsTimer(control->graphics,
                                                         control->sim);
            graphicsTimer->run();


#ifndef NO_GUI
            if(needQApp)
            {
                graphicsTimer = new mars::app::GraphicsTimer(nullptr,
                                                             control->sim);
                graphicsTimer->run();
            }
#endif

            //saveOSGGraph();
        }

        void MARS::readArguments(int argc, char **argv)
        {
            int c;
            int option_index = 0;

            int i;
            char** argv_copy = nullptr;
            unsigned int arg_len = 0;

            //remember how many arguments were already processed by getopt()
            const int old_opterr = opterr;

            //do not print error messages for unkown arguments
            //(may be a valid option for mars_sim).
            opterr = 0;

            static struct option long_options[] = {
                {"config_dir", required_argument, 0, 'C'},
                {"no-gui",no_argument,0,'G'},
                {"noQApp",no_argument,0,'Q'},
                {0, 0, 0, 0}
            };

            // copy the argument vector in order to prevent messing around with the order
            // of the arguments by getopt
            argv_copy = (char**) malloc(argc*sizeof(char*));
            for(i=0; i<argc; i++)
            {
                arg_len = strlen(argv[i]);
                argv_copy[i] = (char*) malloc((arg_len+1)*sizeof(char));
                memcpy(argv_copy[i],argv[i],arg_len);
                argv_copy[i][arg_len] = '\0';
            }

            // here just work with the copied argument vector ...
            while(1)
            {

#ifdef __linux__
                c = getopt_long(argc, argv, "GC:Q", long_options, &option_index);
#else
                c = getopt_long(argc, argv_copy, "GC:Q", long_options, &option_index);
#endif
                if (c == -1)
                    break;
                switch (c)
                {
                case 'C':
                    if( mars::utils::pathExists(std::string(optarg)) )
                    {
                        configDir = optarg;
                        argConfDir = true;
                    } else
                        printf("The given configuration Directory does not exists: %s\n",
                               optarg);
                    break;
                case 'Q':
                    needQApp = false;
                    break;
                case 'G':
                    noGUI = true;
                    break;
                }
            }

            // clean up the copied argument vector
            for(i=0; i<argc; i++)
            {
                free(argv_copy[i]);
            }
            free(argv_copy);

            //reset error message printing to original setting
            opterr = old_opterr;

            //reset index to read arguments again in other libraries (mars_sim).
            optind = 1;
            optarg = nullptr;

            return;
        }

        int MARS::runWoQApp()
        {
            while(!quit)
            {
                if(control->sim->getAllowDraw() || !control->sim->getSyncGraphics())
                {
                    control->sim->finishedDraw();
                }
                //mars::utils::msleep(2);
            }
            return 0;
        }

    } // end of namespace app
} // end of namespace mars
