/**
   @file orientationbin.cpp
   @brief Orientation Bin for ContextFW
   <p>
   Copyright (C) 2009-2010 Nokia Corporation

   @author Marja Hassinen <ext-marja.2.hassinen@nokia.com>
   @author Üstün Ergenoglu <ext-ustun.ergenoglu@nokia.com>

   This file is part of Sensord.

   Sensord is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License
   version 2.1 as published by the Free Software Foundation.

   Sensord is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with Sensord.  If not, see <http://www.gnu.org/licenses/>.
   </p>
*/

#include "orientationbin.h"
#include "sensord/sensormanager.h"

#include <QDebug>

#define STABILITY_THRESHOLD 7
#define UNSTABILITY_THRESHOLD 300
#define STABILITY_HYSTERESIS 0.1

OrientationBin::OrientationBin(ContextProvider::Service& s):
    topEdgeProperty(s, "Screen.TopEdge"),
    isCoveredProperty(s, "Screen.IsCovered"),
    isStableProperty(s, "Position.Stable"),
    isShakyProperty(s, "Position.Shaky"),
    accelerometerReader(10),
    orientationReader(10),
    screenInterpreterFilter(&topEdgeProperty, &isCoveredProperty),
    cutterFilter(4.0),
    avgVarFilter(100),
    stabilityFilter(&isStableProperty, &isShakyProperty,
                    STABILITY_THRESHOLD, UNSTABILITY_THRESHOLD, STABILITY_HYSTERESIS)
{
    //qDebug() << "Creating the orientation bin";

    accelerometerAdaptor = SensorManager::instance().requestDeviceAdaptor("accelerometeradaptor");
    Q_ASSERT(accelerometerAdaptor);

    orientationChain = SensorManager::instance().requestChain("orientationchain");
    Q_ASSERT(orientationChain);

    add(&accelerometerReader, "accelerometer");
    add(&orientationReader, "orientation");
    add(&screenInterpreterFilter, "screeninterpreterfilter");
    add(&normalizerFilter, "normalizerfilter");
    add(&cutterFilter, "cutterfilter");
    add(&avgVarFilter, "avgvarfilter");
    add(&stabilityFilter, "stabilityfilter");

    // Create a branching filter chain
    join("orientation", "source", "screeninterpreterfilter", "sink");

    join("accelerometer", "source", "normalizerfilter", "sink");
    join("normalizerfilter", "source", "cutterfilter", "sink");
    join("cutterfilter", "source", "avgvarfilter", "sink");
    join("avgvarfilter", "source", "stabilityfilter", "sink");

    RingBufferBase* rb;
    rb = accelerometerAdaptor->findBuffer("accelerometer");
    Q_ASSERT(rb);
    rb->join(&accelerometerReader);

    rb = orientationChain->findBuffer("orientation");
    Q_ASSERT(rb);
    rb->join(&orientationReader);

    // Context group
    group.add(topEdgeProperty);
    group.add(isCoveredProperty);
    group.add(isStableProperty);
    group.add(isShakyProperty);
    connect(&group, SIGNAL(firstSubscriberAppeared()), this, SLOT(startRun()));
    connect(&group, SIGNAL(lastSubscriberDisappeared()), this, SLOT(stopRun()));

    // Set default values (if the default isn't Unknown)
    topEdgeProperty.setValue("top");
    isCoveredProperty.setValue(false);
}

OrientationBin::~OrientationBin()
{
    RingBufferBase* rb;
    rb = accelerometerAdaptor->findBuffer("accelerometer");
    Q_ASSERT(rb);
    rb->unjoin(&accelerometerReader);
    SensorManager::instance().releaseDeviceAdaptor("accelerometeradaptor");
}

void OrientationBin::startRun()
{
    //qDebug() << "Staring the run on the orientationbin";

    // Reset the status of the avg & var computation; reset default
    // values for properties whose values aren't reliable after a
    // restart
    avgVarFilter.reset();
    isStableProperty.unsetValue();
    isShakyProperty.unsetValue();
    start();
    accelerometerAdaptor->startSensor("accelerometer");
    orientationChain->start();
}

void OrientationBin::stopRun()
{
    //qDebug() << "Stopping the run on the orientationbin";
    accelerometerAdaptor->stopSensor("accelerometer");
    orientationChain->stop();
    stop();
}