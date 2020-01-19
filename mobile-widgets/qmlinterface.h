// SPDX-License-Identifier: GPL-2.0
#ifndef QMLINTERFACE_H
#define QMLINTERFACE_H
#include "core/settings/qPrefCloudStorage.h"
#include "core/settings/qPrefUnit.h"
#include "core/settings/qPrefDivePlanner.h"
#include "qt-models/diveplannermodel.h"

#include <QObject>
#include <QQmlContext>
// This class is a pure interface class and may not contain any implementation code
// Allowed are:
//     header
//        Q_PROPERTY
//        signal/slot for Q_PROPERTY functions
//        the functions may contain either
//             a) a function call to the implementation
//             b) a reference to a global variable like e.g. prefs.
//        Q_INVOCABLE functions
//        the functions may contain
//             a) a function call to the implementation
//    source
//        connect signal/signal to pass signals from implementation


class QMLInterface : public QObject {
	Q_OBJECT

	// Q_PROPERTY used in QML
	Q_PROPERTY(CLOUD_STATUS cloud_verification_status READ cloud_verification_status WRITE set_cloud_verification_status NOTIFY cloud_verification_statusChanged)
	Q_PROPERTY(DURATION duration_units READ duration_units WRITE set_duration_units NOTIFY duration_unitsChanged)
	Q_PROPERTY(LENGTH length READ length WRITE set_length NOTIFY lengthChanged)
	Q_PROPERTY(PRESSURE pressure READ pressure WRITE set_pressure NOTIFY pressureChanged)
	Q_PROPERTY(TEMPERATURE temperature READ temperature WRITE set_temperature NOTIFY temperatureChanged)
	Q_PROPERTY(UNIT_SYSTEM unit_system READ unit_system WRITE set_unit_system NOTIFY unit_systemChanged)
	Q_PROPERTY(TIME vertical_speed_time READ vertical_speed_time WRITE set_vertical_speed_time NOTIFY vertical_speed_timeChanged)
	Q_PROPERTY(VOLUME volume READ volume WRITE set_volume NOTIFY volumeChanged)
	Q_PROPERTY(WEIGHT weight READ weight WRITE set_weight NOTIFY weightChanged)

	Q_PROPERTY(int ascratelast6m READ ascratelast6m WRITE set_ascratelast6m NOTIFY ascratelast6mChanged);
	Q_PROPERTY(int ascratestops READ ascratestops WRITE set_ascratestops NOTIFY ascratestopsChanged);
	Q_PROPERTY(int ascrate50 READ ascrate50 WRITE set_ascrate50 NOTIFY ascrate50Changed);
	Q_PROPERTY(int ascrate75 READ ascrate75 WRITE set_ascrate75 NOTIFY ascrate75Changed);
	Q_PROPERTY(int descrate READ descrate WRITE set_descrate NOTIFY descrateChanged);

public:
	static QMLInterface *instance();

	// function to do the needed setup and do connect of signal/signal
	static void setup(QQmlContext *ct);

	// Duplicated enums, these enums are properly defined in the C/C++ structure
	// but duplicated here to make them available to QML.

	// Duplicating the enums poses a slight risk for forgetting to update
	// them if the proper enum is changed (e.g. assigning a new start value).

	// remark please do not use these enums outside the C++/QML interface.
	enum UNIT_SYSTEM {
		METRIC,
		IMPERIAL,
		PERSONALIZE
	};
	Q_ENUM(UNIT_SYSTEM);

	enum LENGTH {
		METERS,
		FEET
	};
	Q_ENUM(LENGTH);

	enum VOLUME {
		LITER,
		CUFT
	};
	Q_ENUM(VOLUME);

	enum PRESSURE {
		BAR,
		PSI,
		PASCALS
	};
	Q_ENUM(PRESSURE);

	enum TEMPERATURE {
		CELSIUS,
		FAHRENHEIT,
		KELVIN
	};
	Q_ENUM(TEMPERATURE);

	enum WEIGHT {
		KG,
		LBS
	};
	Q_ENUM(WEIGHT);

	enum TIME {
		SECONDS,
		MINUTES
	};
	Q_ENUM(TIME);

	enum DURATION {
		MIXED,
		MINUTES_ONLY,
		ALWAYS_HOURS
	};
	Q_ENUM(DURATION);

	enum CLOUD_STATUS {
			CS_UNKNOWN,
			CS_INCORRECT_USER_PASSWD,
			CS_NEED_TO_VERIFY,
			CS_VERIFIED,
			CS_NOCLOUD
	};
	Q_ENUM(CLOUD_STATUS);

public:
	CLOUD_STATUS cloud_verification_status() { return (CLOUD_STATUS)prefs.cloud_verification_status; }
	DURATION duration_units() { return (DURATION)prefs.units.duration_units; }
	LENGTH length() { return (LENGTH)prefs.units.length; }
	PRESSURE pressure() { return (PRESSURE)prefs.units.pressure; }
	TEMPERATURE temperature() { return (TEMPERATURE)prefs.units.temperature; }
	UNIT_SYSTEM unit_system() { return (UNIT_SYSTEM)prefs.unit_system; }
	TIME vertical_speed_time() { return (TIME)prefs.units.vertical_speed_time; }
	VOLUME volume() { return (VOLUME)prefs.units.volume; }
	WEIGHT weight() { return (WEIGHT)prefs.units.weight; }

	int ascratelast6m() { return DivePlannerPointsModel::instance()->ascratelast6mDisplay(); }
	int ascratestops() { return DivePlannerPointsModel::instance()->ascratestopsDisplay(); }
	int ascrate50() { return DivePlannerPointsModel::instance()->ascrate50Display(); }
	int ascrate75() { return DivePlannerPointsModel::instance()->ascrate75Display(); }
	int descrate() { return DivePlannerPointsModel::instance()->descrateDisplay(); }

public slots:
	void set_cloud_verification_status(CLOUD_STATUS value) {  qPrefCloudStorage::set_cloud_verification_status(value); }
	void set_duration_units(DURATION value) { qPrefUnits::set_duration_units((units::DURATION)value); }
	void set_length(LENGTH value) { qPrefUnits::set_length((units::LENGTH)value); }
	void set_pressure(PRESSURE value) { qPrefUnits::set_pressure((units::PRESSURE)value); }
	void set_temperature(TEMPERATURE value) { qPrefUnits::set_temperature((units::TEMPERATURE)value); }
	void set_unit_system(UNIT_SYSTEM value) { qPrefUnits::set_unit_system((unit_system_values)value); }
	void set_vertical_speed_time(TIME value) { qPrefUnits::set_vertical_speed_time((units::TIME)value); }
	void set_volume(VOLUME value) { qPrefUnits::set_volume((units::VOLUME)value); }
	void set_weight(WEIGHT value) { qPrefUnits::set_weight((units::WEIGHT)value); }

	void set_ascratelast6m(int value) { DivePlannerPointsModel::instance()->setAscratelast6mDisplay(value); }
	void set_ascratestops(int value) { DivePlannerPointsModel::instance()->setAscratestopsDisplay(value); }
	void set_ascrate50(int value) { DivePlannerPointsModel::instance()->setAscrate50Display(value); }
	void set_ascrate75(int value) { DivePlannerPointsModel::instance()->setAscrate75Display(value); }
	void set_descrate(int value) { DivePlannerPointsModel::instance()->setDescrateDisplay(value); }

signals:
	void cloud_verification_statusChanged(CLOUD_STATUS);
	void duration_unitsChanged(DURATION);
	void lengthChanged(LENGTH);
	void pressureChanged(PRESSURE);
	void temperatureChanged(TEMPERATURE);
	void unit_systemChanged(UNIT_SYSTEM);
	void vertical_speed_timeChanged(TIME);
	void volumeChanged(VOLUME);
	void weightChanged(WEIGHT);

	void ascratelast6mChanged(int);
	void ascratestopsChanged(int);
	void ascrate50Changed(int);
	void ascrate75Changed(int);
	void descrateChanged(int);

private:
	QMLInterface() {}
};
#endif // QMLINTERFACE_H
