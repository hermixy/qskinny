/******************************************************************************
 * QSkinny - Copyright (C) 2016 Uwe Rathmann
 * This file may be used under the terms of the QSkinny License, Version 1.0
 *****************************************************************************/

#include "QskAspect.h"

#include <QDebug>
#include <QMetaEnum>
#include <QtAlgorithms>
#include <QVector>

#include <limits>
#include <bitset>
#include <string>
#include <unordered_map>

static_assert( sizeof( QskAspect::Aspect ) == sizeof( quint64 ),
    "QskAspect::Aspect has to match quint64" );

namespace
{
    using namespace QskAspect;
    using namespace std;

    struct StateInfo
    {
        State state;
        QByteArray name;
    };

    struct AspectRegistry
    {
        QVector< QByteArray > subControlNames;
        unordered_map< const QMetaObject*, QVector< Subcontrol > > subControlTable;
        unordered_map< const QMetaObject*, QVector< StateInfo > > stateTable;
    };
}

Q_GLOBAL_STATIC( AspectRegistry, qskAspectRegistry )

QskAspect::State QskAspect::registerState(
    const QMetaObject* metaObject, State state, const char* name )
{
    StateInfo info;
    info.state = state;
    info.name = name;

    auto& stateTable = qskAspectRegistry->stateTable;
    stateTable[ metaObject ] += info;

    return state;
}

QskAspect::Subcontrol QskAspect::nextSubcontrol(
    const QMetaObject* metaObject, const char* name )
{
    auto& names = qskAspectRegistry->subControlNames;
    auto& hashTable = qskAspectRegistry->subControlTable;

    Q_ASSERT_X( names.size() <= LastSubcontrol,
        "QskAspect", "There are no free subcontrol aspects; please modify your"
        " application to declare fewer aspects, or increase the mask size of"
        " QskAspect::Subcontrol in QskAspect.h." );

    names += name;

    // 0 is QskAspect::Control, so we have to start with 1
    const auto subControl = static_cast< Subcontrol >( names.size() );
    hashTable[ metaObject ] += subControl;

    return subControl;
}

QByteArray QskAspect::subControlName( Subcontrol subControl )
{
    const auto& names = qskAspectRegistry->subControlNames;

    const int index = subControl;
    if ( index > 0 && index < names.size() )
        return names[ index - 1 ];

    return QByteArray();
}

QVector< QByteArray > QskAspect::subControlNames( const QMetaObject* metaObject )
{
    const auto& names = qskAspectRegistry->subControlNames;

    if ( metaObject == nullptr )
        return names;

    const auto subControls = QskAspect::subControls( metaObject );

    QVector< QByteArray > subControlNames;
    subControlNames.reserve( subControls.size() );

    for ( auto subControl : subControls )
        subControlNames += names[ subControl ];

    return subControlNames;
}

QVector< QskAspect::Subcontrol > QskAspect::subControls( const QMetaObject* metaObject )
{
    if ( metaObject )
    {
        const auto& hashTable = qskAspectRegistry->subControlTable;

        auto it = hashTable.find( metaObject );
        if ( it != hashTable.end() )
            return it->second;
    }

    return QVector< Subcontrol >();
}

#ifndef QT_NO_DEBUG_STREAM

static QByteArray qskEnumString( const char* name, int value )
{
    const QMetaObject& mo = QskAspect::staticMetaObject;

    const QMetaEnum metaEnum = mo.enumerator( mo.indexOfEnumerator( name ) );
    const char* key = metaEnum.valueToKey( value );

    return key ? QByteArray( key ) : QString::number( value ).toLatin1();
}

QByteArray qskStateKey( const QMetaObject* metaObject, quint16 state )
{
    auto& stateTable = qskAspectRegistry->stateTable;

    for ( auto mo = metaObject; mo != nullptr; mo = mo->superClass() )
    {
        auto it = stateTable.find( mo );
        if ( it != stateTable.end() )
        {
            for ( const auto& info : it->second )
            {
                if ( info.state == state )
                    return info.name;
            }
        }
    }

    return QByteArray();;
}

static QByteArray qskStateString(
    const QMetaObject* metaObject, QskAspect::State state )
{
    if ( state == 0 )
    {
        return "NoState";
    }

    if ( metaObject == nullptr )
    {
        const std::bitset< 16 > stateBits( state );
        return stateBits.to_string().c_str();
    }

    // not the fastest implementation, but as it is for debugging only

    QByteArray stateString;

    bool first = true;

    for ( int i = 0; i < 16; i++ )
    {
        const quint16 mask = 1 << i;

        if ( state & mask )
        {
            if ( first )
                first = false;
            else
                stateString += ", ";

            const auto key = qskStateKey( metaObject, mask );
            if ( key.isEmpty() )
            {
                const std::bitset< 16 > stateBits( state );
                stateString += stateBits.to_string().c_str();
            }
            else
            {
                stateString += key;
            }
        }
    }

    return stateString;
}

static inline QDebug qskDebugEnum(
    QDebug debug, const char* name, int value )
{
    qt_QMetaEnum_debugOperator( debug, value,
        &QskAspect::staticMetaObject, name );
    return debug;
}

QDebug operator<<( QDebug debug, const QskAspect::Type& type )
{
    return qskDebugEnum( debug, "Type", type );
}

QDebug operator<<( QDebug debug, const QskAspect::Edge& edge )
{
    return qskDebugEnum( debug, "Edge", edge );
}

QDebug operator<<( QDebug debug, const QskAspect::Corner& corner )
{
    return qskDebugEnum( debug, "Corner", corner );
}

QDebug operator<<( QDebug debug, const QskAspect::BoxPrimitive& primitive )
{
    return qskDebugEnum( debug, "BoxPrimitive", primitive );
}

QDebug operator<<( QDebug debug, const QskAspect::FlagPrimitive& primitive )
{
    return qskDebugEnum( debug, "FlagPrimitive", primitive );
}

QDebug operator<<( QDebug debug, const QskAspect::ColorPrimitive& primitive )
{
    return qskDebugEnum( debug, "ColorPrimitive", primitive );
}

QDebug operator<<( QDebug debug, const QskAspect::MetricPrimitive& primitive )
{
    return qskDebugEnum( debug, "MetricPrimitive", primitive );
}

QDebug operator<<( QDebug debug, const QskAspect::Subcontrol& subControl )
{
    QDebugStateSaver saver( debug );

    debug.nospace();
    debug << "QskAspect::Subcontrol" << '(';
    debug << subControlName( subControl );
    debug << ')';

    return debug;
}

QDebug operator<<( QDebug debug, const QskAspect::State& state )
{
    qskDebugState( debug, nullptr, state );
    return debug;
}

QDebug operator<<( QDebug debug, const QskAspect::Aspect& aspect )
{
    qskDebugAspect( debug, nullptr, aspect );
    return debug;
}

void qskDebugState( QDebug debug, const QMetaObject* metaObject, QskAspect::State state )
{
    QDebugStateSaver saver(debug);

    debug.resetFormat();
    debug.noquote();
    debug.nospace();

    debug << "QskAspect::State( " << qskStateString( metaObject, state ) << " )";
}

void qskDebugAspect( QDebug debug, const QMetaObject* metaObject, QskAspect::Aspect aspect )
{
    using namespace QskAspect;

    QDebugStateSaver saver(debug);

    debug.resetFormat();
    debug.noquote();
    debug.nospace();

    debug << "QskAspect( ";

    const auto subControlName = QskAspect::subControlName( aspect.subControl() );
    if ( subControlName.isEmpty() )
        debug << QByteArrayLiteral( "NoSubcontrol" );
    else
        debug << subControlName;

    debug << ", " << qskEnumString( "Type", aspect.type() );
    if ( aspect.isAnimator() )
        debug << "(A)";

    debug << ", ";

    if ( !aspect.isBoxPrimitive() )
    {
        switch( aspect.type() )
        {
            case Color:
            {
                debug << qskEnumString( "ColorPrimitive", aspect.colorPrimitive() );
                break;
            }
            case Metric:
            {
                debug << qskEnumString( "MetricPrimitive", aspect.metricPrimitive() );
                break;
            }
            default:
            {
                debug << qskEnumString( "FlagPrimitive", aspect.flagPrimitive() );
            }
        }
    }
    else
    {
        debug << qskEnumString( "BoxPrimitive", aspect.boxPrimitive() );

        switch( aspect.boxPrimitive() )
        {
            case Border:
            {
                if ( aspect.edge() )
                    debug << ", " << qskEnumString( "Edge", aspect.edge() );
                break;
            }
            default:
            {
                if ( aspect.corner() )
                    debug << ", " << qskEnumString( "Corner", aspect.corner() );
            }
        }
    }

    if ( aspect.state() )
        debug << ", " << qskStateString( metaObject, aspect.state() );

    debug << " )";
}

#endif

const char* QskAspect::Aspect::toPrintable() const
{
    QString tmp;

    QDebug debug( &tmp );
    debug << *this;

    // we should find a better impementation
    static QByteArray bytes[10];
    static int counter = 0;

    counter = ( counter + 1 ) % 10;

    bytes[counter] = tmp.toUtf8();
    return bytes[counter].constData();
}

QskAspect::State QskAspect::Aspect::topState() const
{
    if ( m_states == NoState )
        return NoState;

    /*
        Before Qt 5.8 qCountLeadingZeroBits does not use
        _BitScanReverse - we can live with this.
     */

    const auto n = qCountLeadingZeroBits( static_cast< quint16 >( m_states ) );
    return static_cast< QskAspect::State >( 1 << ( 15 - n ) );
}

#include "moc_QskAspect.cpp"
