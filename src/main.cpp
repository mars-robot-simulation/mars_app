#include "MARS.hpp"
#include "MyApp.hpp"

#include <signal.h>
#include <cstdio>
//#include "HandleFileNames.h"
//#include <QPlastiqueStyle>


#include <stdexcept>


void qtExitHandler(int sig)
{
    mars::app::exit_main(sig);
}

void ignoreSignal(int sig)
{
    (void)(sig);
}

/**
 * The main function, that starts the GUI and init the physical environment.
 *Convention that start the simulation
 */
int main(int argc, char *argv[])
{

    //  Q_INIT_RESOURCE(resources);
#ifndef WIN32
    signal(SIGQUIT, qtExitHandler);
    signal(SIGPIPE, qtExitHandler);
    //signal(SIGKILL, qtExitHandler);
    signal(SIGUSR1, ignoreSignal);
#else
    signal(SIGFPE, qtExitHandler);
    signal(SIGILL, qtExitHandler);
    signal(SIGSEGV, qtExitHandler);
    signal(SIGBREAK, qtExitHandler);
#endif
    signal(SIGABRT, qtExitHandler);
    signal(SIGTERM, qtExitHandler);
    signal(SIGINT, qtExitHandler);


    mars::app::MARS *simulation = new mars::app::MARS();
    simulation->readArguments(argc, argv);

    mars::app::MyApp *app = nullptr;
    if(simulation->needQApp)
    {
        app = new mars::app::MyApp(argc, argv);
        //app->setStyle(new QPlastiqueStyle);
    }

    // for osx relase build:
    /*
      QDir dir(QApplication::applicationDirPath());
      dir.cdUp();
      dir.cd("PlugIns");
      QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
    */

    //app->setWindowIcon(QIcon(QString::fromStdString(Pathes::getGuiPath()) + "images/mars_icon.ico"));

    simulation->start(argc, argv);

    int state;
    if(simulation->needQApp) state = app->exec();
    else state = simulation->runWoQApp();
    delete simulation;
    fprintf(stderr, "\n################################\n");
    fprintf(stderr, "## everything closed fine ^-^ ##\n");
    fprintf(stderr, "################################\n\n");
    return state;
}
