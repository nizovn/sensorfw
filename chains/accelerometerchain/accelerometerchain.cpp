/**
   @file accelerometerchain.cpp
   @brief AccelerometerChain

   <p>
   Copyright (C) 2009-2010 Nokia Corporation

   @author Timo Rongas <ext-timo.2.rongas@nokia.com>
   @author Ustun Ergenoglu <ext-ustun.ergenoglu@nokia.com>

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

#include "accelerometerchain.h"
#include <QVariant>
#include "sensord/sensormanager.h"
#include "sensord/bin.h"
#include "sensord/bufferreader.h"

#include "filters/coordinatealignfilter/coordinatealignfilter.h"

double AccelerometerChain::aconv_[3][3] = { {-1.0, 0.0, 0.0}, 
                                            { 0.0,-1.0, 0.0},
                                            { 0.0, 0.0,-1.0} };

AccelerometerChain::AccelerometerChain(const QString& id) :
    AbstractChain(id)
{
    SensorManager& sm = SensorManager::instance();
    
    accelerometerAdaptor_ = sm.requestDeviceAdaptor("accelerometeradaptor");
    Q_ASSERT( accelerometerAdaptor_ );
    if (!accelerometerAdaptor_->isValid()) {
        isValid_ = false;
    } else {
        isValid_ = true;
    }

    accelerometerReader_ = new BufferReader<AccelerationData>(1024);

    accCoordinateAlignFilter_ = sm.instantiateFilter("coordinatealignfilter");
    Q_ASSERT(accCoordinateAlignFilter_);
    qRegisterMetaType<TMatrix>("TMatrix");
    ((CoordinateAlignFilter*)accCoordinateAlignFilter_)->setProperty("transMatrix", QVariant::fromValue(TMatrix(aconv_)));

    outputBuffer_ = new RingBuffer<AccelerationData>(1024);
    nameOutputBuffer("accelerometer", outputBuffer_);

    // Create buffers for filter chain
    filterBin_ = new Bin;

    filterBin_->add(accelerometerReader_, "accelerometer");
    filterBin_->add(accCoordinateAlignFilter_, "acccoordinatealigner");
    filterBin_->add(outputBuffer_, "buffer");

    // Join filterchain buffers
    filterBin_->join("accelerometer", "source", "acccoordinatealigner", "sink");
    filterBin_->join("acccoordinatealigner", "source", "buffer", "sink");

    // Join datasources to the chain
    RingBufferBase* rb;
    rb = accelerometerAdaptor_->findBuffer("accelerometer");
    Q_ASSERT(rb);
    rb->join(accelerometerReader_);


}

AccelerometerChain::~AccelerometerChain()
{
    SensorManager& sm = SensorManager::instance();

    RingBufferBase* rb;
    rb = accelerometerAdaptor_->findBuffer("accelerometer");
    Q_ASSERT(rb);
    rb->unjoin(accelerometerReader_);

    sm.releaseDeviceAdaptor("accelerometeradaptor");

    delete accelerometerReader_;
    delete accCoordinateAlignFilter_;
    delete outputBuffer_;
    delete filterBin_;
}

// TODO: Solve thread safety for start...
bool AccelerometerChain::start()
{
    if (AbstractSensorChannel::start()) {
        sensordLogD() << "Starting AccelerometerChain";
        filterBin_->start();
        accelerometerAdaptor_->startSensor("accelerometer");
    }
    return true;
}

bool AccelerometerChain::stop()
{
    if (AbstractSensorChannel::stop()) {
        sensordLogD() << "Stopping AccelerometerChain";
        accelerometerAdaptor_->stopSensor("accelerometer");
        filterBin_->stop();
    }
    return true;
}