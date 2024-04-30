#include "GraphicsTimer.hpp"
#include <mars_utils/misc.h>
#include <stdlib.h>

namespace mars
{
    namespace app
    {

        using mars::interfaces::GraphicsManagerInterface;
        using mars::interfaces::SimulatorInterface;

        GraphicsTimer::GraphicsTimer(GraphicsManagerInterface *graphics_,
                                     SimulatorInterface *sim_)
            : graphics(graphics_), sim(sim_)
        {
            graphicsTimer = new QTimer();
            connect(graphicsTimer, SIGNAL(timeout()), this, SLOT(timerEvent()));
            connect(this, SIGNAL(internalRun()), this, SLOT(runOnceInternal()),
                    Qt::QueuedConnection);
        }

        void GraphicsTimer::run()
        {
            char* marsTimerStepsize = getenv("MARS_GRAPHICS_UPDATE_TIME");
            int updateTime = 10;
            if(marsTimerStepsize)
            {
                updateTime = atoi(marsTimerStepsize);
            }
            graphicsTimer->start(updateTime);
        }

        void GraphicsTimer::stop()
        {
            graphicsTimer->stop();
        }

        void GraphicsTimer::runOnce()
        {
            runFinished=false;
            emit internalRun();
            while(!runFinished)
            {
                mars::utils::msleep(1);
            }
        }

        void GraphicsTimer::runOnceInternal()
        {
            timerEvent();
            runFinished=true;
        }

        void GraphicsTimer::timerEvent(void)
        {
            if(sim->getAllowDraw() || !sim->getSyncGraphics())
            {
                if(graphics)
                {
                    graphics->draw();
                } else
                {
                    sim->finishedDraw();
                }
            }
        }

    } // end of namespace app
} // end of namespace mars
