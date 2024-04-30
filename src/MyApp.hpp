/**
 * \file MY_APP.hpp
 * \author Malte Langosz
 *
 */

#pragma once

#ifndef NO_GUI
#include <QApplication>
#else
#include <stdlib.h>
#endif

namespace mars
{

    namespace app
    {

        /**
         * This function provides a clean exit of the simulation
         * in case of a kill-signal.
         */

#ifdef NO_GUI
        class MyApp
        {
        public:
            MyApp(int &argc, char **argv) {}
            int exec()
                {
                    abort();
                    return 0;
                }
            void processEvents()
                {
                    abort();
                }
        };
#else
        class MyApp : public QApplication
        {
        public:
            MyApp(int &argc, char **argv) : QApplication(argc, argv) {}
            virtual bool notify( QObject *receiver, QEvent *event )
                {
                    try
                    {
                        return QApplication::notify(receiver, event);
                    } catch (const std::exception &e)
                    {
                        std::cerr << e.what() << std::endl;
                        throw(e);
                    }
                }
        };
#endif

    } // end of namespace app
} // end of namespace mars
