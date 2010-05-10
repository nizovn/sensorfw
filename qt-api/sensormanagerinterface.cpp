/**
   @file sensormanagerinterface.cpp
   @brief Interface for SensorManager

   <p>
   Copyright (C) 2009-2010 Nokia Corporation

   @author Joep van Gassel <joep.van.gassel@nokia.com>
   @author Timo Rongas <ext-timo.2.rongas@nokia.com>

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

#include "serviceinfo.h"
#include "idutils.h"
#include "sensormanagerinterface.h"

SensorManagerInterface* SensorManagerInterface::ifc_ = 0;

SensorManagerInterface::SensorManagerInterface()
  : LocalSensorManagerInterface( SERVICE_NAME, OBJECT_PATH, QDBusConnection::systemBus() )
{
}

SensorManagerInterface& SensorManagerInterface::instance()
{
    if ( !ifc_ )
    {
        ifc_ = new SensorManagerInterface;
        if (! ifc_->isValid() )
        {
            qDebug() << "ERROR:" << ifc_->lastError().message();
        }
        //Q_ASSERT( ifc_->isValid() ); // Stay alive, check with isValid() on client side
    }

    return *ifc_;
}

bool SensorManagerInterface::registeredAndCorrectClassName(const QString& id, const QString& className ) const
{
    QString cleanId = getCleanId(id);
    return ( sensorInterfaceMap_.contains(cleanId) ) && ( sensorInterfaceMap_[cleanId].type == className );
}

const QDBusAbstractInterface* SensorManagerInterface::listenInterface(const QString& id)
{
    QString cleanId = getCleanId(id);

    if ( !sensorInterfaceMap_.contains(id) )
    {
        qDebug() << "Requested sensor id " << id << "interface not known";
        return 0;
    }

    SensorManagerInterface& sm = SensorManagerInterface::instance();
    QDBusAbstractInterface* ifc = 0;
    int sessionId = sm.requestListenSensor(id);
    if ( sessionId >= 0 ) // sensor is available
    {
        // TODO: who owns this interface and how to deal with derived Sensors?
        // TODO: make a real distinction between control and listening
        qDebug() << "New listen sensor" << id << "interface created with session id" << sessionId << "...";

        // TODO: uses both id for key and as parameter...
        ifc = sensorInterfaceMap_[cleanId].sensorInterfaceFactory(cleanId, sessionId);
    }
    else
    {
        qDebug( "Request for listen interface not granted..." );
    }

    return ifc;
}

QDBusAbstractInterface* SensorManagerInterface::controlInterface(const QString& id)
{
    QString cleanId = getCleanId(id);

    // TODO: who owns this interface and how to deal with derived Sensors?
    // TODO: make a real distinction between control and listening
    SensorManagerInterface& sm = SensorManagerInterface::instance();
    QDBusAbstractInterface* ifc = 0;
    int sessionId = sm.requestControlSensor(id);
    if ( sessionId >= 0 ) // sensor is available
    {
        qDebug() << "New control sensor" << id << "interface created with session id" << sessionId << "...";
        
        // TODO: uses both id for key and as parameter...
        Q_ASSERT( sensorInterfaceMap_.contains(cleanId) );
        ifc = sensorInterfaceMap_[cleanId].sensorInterfaceFactory(cleanId, sessionId);
    }
    else
    {
        qDebug( "Request for control interface not granted..." );
    }

    return ifc;
}

bool SensorManagerInterface::releaseInterface(const QString& id, int sessionId)
{
    QString cleanId = getCleanId(id);

    QDBusReply<bool> reply = LocalSensorManagerInterface::releaseSensor(cleanId, sessionId);
    if ( !reply.isValid() )
    {
        qDebug() << "Error:" << reply.error().message();
    }
    Q_ASSERT( reply.isValid() );
    return reply.value();
}
