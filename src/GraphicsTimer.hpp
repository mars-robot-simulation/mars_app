#pragma once

#include <QObject>
#include <QTimer>

#include <mars_interfaces/graphics/GraphicsManagerInterface.h>
#include <mars_interfaces/sim/SimulatorInterface.h>

namespace mars
{
    namespace app
    {

        class GraphicsTimer : public QObject
        {
            Q_OBJECT;

        public:
            GraphicsTimer(mars::interfaces::GraphicsManagerInterface *graphics_,
                          mars::interfaces::SimulatorInterface *sim_);

            ~GraphicsTimer()
                {
                    graphicsTimer->stop();
                }

            void run();
            void runOnce();
            void stop();
        signals:
            void internalRun();

        public slots:
            void timerEvent(void);
            void runOnceInternal(void);

        private:
            QTimer *graphicsTimer;
            mars::interfaces::GraphicsManagerInterface *graphics;
            mars::interfaces::SimulatorInterface *sim;
            bool runFinished;

        }; // end of class GraphicsTimer

    } // end of namespace app
} // end of namespace mars
