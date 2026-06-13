from typing import Optional

from .models import (
    Activity,
    Alert,
    Artifact,
    DashboardData,
    EvidenceSummary,
    FieldEvent,
    FieldEventDraft,
    IrrigationMarker,
    Measurement,
    SensorReading,
    Study,
    StudyDraft,
    now_iso,
)
from .openai_service import generate_study_insight
from .weather_service import WeatherSnapshot, fetch_viterbo_weather


class VineyardStore:
    def __init__(self, dashboard: DashboardData):
        self.dashboard = dashboard
        self.latest_sensor_reading: Optional[SensorReading] = None
        self._field_event_counter = 1
        self._study_counter = 1

    def get_dashboard(self) -> DashboardData:
        weather = fetch_viterbo_weather()
        self._apply_weather_context(weather)
        return self.dashboard

    def get_latest_sensor_reading(self) -> Optional[SensorReading]:
        return self.latest_sensor_reading

    def ingest_sensor_reading(self, reading: SensorReading) -> SensorReading:
        timestamp = reading.timestamp or now_iso()
        reading.timestamp = timestamp
        self.latest_sensor_reading = reading
        self.dashboard.vineyard.lastSensorUpdate = timestamp

        plot = next((item for item in self.dashboard.plots if item.id == reading.plotId), None)
        if plot:
            plot.soilMoisture = reading.soilMoisture
            plot.status = "Water stress risk" if reading.soilMoisture < 20 else "Healthy"

        sensor = next((item for item in self.dashboard.sensors if item.id == reading.sensorId), None)
        if sensor:
            sensor.lastReading = reading.soilMoisture
            sensor.batteryLevel = reading.batteryLevel
            sensor.status = reading.status

        self.dashboard.measurements.append(
            Measurement(
                id=f"measurement-live-{len(self.dashboard.measurements) + 1}",
                plotId=reading.plotId,
                timestamp=timestamp,
                soilMoisture=reading.soilMoisture,
                airTemperature=plot.airTemperature if plot else 27.0,
                airHumidity=plot.airHumidity if plot else 55,
                rainfall=0,
            )
        )
        self._upsert_sensor_alert(reading)
        return reading

    def create_field_event(self, draft: FieldEventDraft) -> FieldEvent:
        created_at = now_iso()
        event = FieldEvent(
            id=f"field-event-{self._field_event_counter}",
            plotId=draft.plotId,
            type=draft.type,
            date=draft.date,
            time=draft.time,
            durationMinutes=draft.durationMinutes,
            notes=draft.notes,
            createdAt=created_at,
        )
        self._field_event_counter += 1
        self.dashboard.fieldEvents.insert(0, event)
        self.dashboard.activities.insert(
            0,
            Activity(
                id=f"activity-event-{self._field_event_counter}",
                type="irrigation" if event.type == "Irrigation" else "event",
                title=f"{event.type} recorded",
                description=f"{event.durationMinutes}-minute {event.type.lower()} added for {self._plot_label(event.plotId)}.",
                timestamp=created_at,
            ),
        )
        if event.type == "Irrigation":
            self.dashboard.irrigationMarkers.append(
                IrrigationMarker(timestamp=f"{event.date}T{event.time}:00+02:00", plotId=event.plotId, label="Irrigation event")
            )
        return event

    def create_study(self, draft: StudyDraft) -> Study:
        plot = next((item for item in self.dashboard.plots if item.id == draft.plotId), self.dashboard.plots[0])
        measurements = [item for item in self.dashboard.measurements if item.plotId == draft.plotId]
        related_event = None
        if draft.relatedFieldEventId:
            related_event = next((item for item in self.dashboard.fieldEvents if item.id == draft.relatedFieldEventId), None)

        insight = generate_study_insight(draft, plot, measurements, related_event)
        created_at = now_iso()
        field_events_count = len([item for item in self.dashboard.fieldEvents if item.plotId == draft.plotId])
        title_topic = "Irrigation response" if "irrig" in draft.researchQuestion.lower() else "AI vineyard assessment"
        study = Study(
            id=f"study-{self._study_counter}",
            title=f"{title_topic} in {plot.name}",
            researchQuestion=draft.researchQuestion,
            plotId=draft.plotId,
            dateRange=draft.dateRange,
            metrics=draft.metrics,
            relatedFieldEventId=draft.relatedFieldEventId,
            notes=draft.notes,
            status="Ready for review",
            observation=insight["observation"],
            evidence=EvidenceSummary(
                sensorReadings=len(measurements),
                fieldEvents=field_events_count,
                observationWindow="Selected observation window",
                generatedArtifacts=3,
            ),
            interpretation=insight["interpretation"],
            limitations=insight["limitations"],
            suggestedNextStep=insight["suggestedNextStep"],
            artifacts=[
                Artifact(id="artifact-measurements", name="measurements.csv", type="csv", size="18 KB"),
                Artifact(id="artifact-metadata", name="study-metadata.json", type="json", size="4 KB"),
                Artifact(id="artifact-ai-summary", name="openai-study-summary.json", type="json", size="6 KB"),
            ],
            syncState="not_synchronized",
            createdAt=created_at,
        )
        self._study_counter += 1
        self.dashboard.studies.insert(0, study)
        self.dashboard.activities.insert(
            0,
            Activity(
                id=f"activity-study-{self._study_counter}",
                type="study",
                title="OpenAI study generated",
                description=f"AI evidence package prepared for {plot.name} using sensor readings and field context.",
                timestamp=created_at,
            ),
        )
        return study

    def _apply_weather_context(self, weather: WeatherSnapshot) -> None:
        self.dashboard.vineyard.location = weather.location

        plot_offsets = {
            "plot-a": (0.0, 0),
            "plot-b": (0.8, -3),
            "plot-c": (-0.4, 4),
        }
        for plot in self.dashboard.plots:
            temperature_offset, humidity_offset = plot_offsets.get(plot.id, (0.0, 0))
            plot.airTemperature = round(weather.temperature + temperature_offset, 1)
            plot.airHumidity = max(0, min(100, weather.humidity + humidity_offset))

        latest_by_plot = {}
        for measurement in self.dashboard.measurements:
            latest_by_plot[measurement.plotId] = measurement
        for plot_id, measurement in latest_by_plot.items():
            plot = next((item for item in self.dashboard.plots if item.id == plot_id), None)
            if plot:
                measurement.airTemperature = plot.airTemperature
                measurement.airHumidity = plot.airHumidity
                measurement.rainfall = weather.rainfall

        for sensor in self.dashboard.sensors:
            plot = next((item for item in self.dashboard.plots if item.id == sensor.plotId), None)
            if not plot:
                continue
            if sensor.type == "Air temperature":
                sensor.lastReading = plot.airTemperature
            elif sensor.type == "Air humidity":
                sensor.lastReading = plot.airHumidity
            elif sensor.type == "Rain gauge":
                sensor.lastReading = weather.rainfall

        self._upsert_weather_alerts(weather)

    def _upsert_weather_alerts(self, weather: WeatherSnapshot) -> None:
        self.dashboard.alerts = [
            alert
            for alert in self.dashboard.alerts
            if alert.id not in {"alert-weather-rain", "alert-weather-dry-stress", "alert-weather-mildew"}
        ]
        timestamp = now_iso()
        dry_plots = [plot for plot in self.dashboard.plots if plot.soilMoisture < 25]
        if weather.rain_next_12h >= 5:
            self.dashboard.alerts.insert(
                0,
                Alert(
                    id="alert-weather-rain",
                    title="Rain expected near Viterbo",
                    description=f"Open-Meteo forecasts {weather.rain_next_12h:.1f} mm in the next 12 hours. Consider delaying irrigation unless live soil moisture remains critical.",
                    severity="info",
                    timestamp=timestamp,
                ),
            )
        elif dry_plots and weather.rain_next_72h < 3 and weather.max_temperature_next_72h >= 28:
            driest_plot = min(dry_plots, key=lambda item: item.soilMoisture)
            self.dashboard.alerts.insert(
                0,
                Alert(
                    id="alert-weather-dry-stress",
                    plotId=driest_plot.id,
                    title="Dry forecast increases irrigation priority",
                    description=f"{driest_plot.name} is at {driest_plot.soilMoisture:.1f}% soil moisture, Tuscia forecast rain is only {weather.rain_next_72h:.1f} mm over 72 hours, and peak temperature may reach {weather.max_temperature_next_72h:.1f}°C.",
                    severity="warning",
                    timestamp=timestamp,
                ),
            )
        if weather.humidity >= 75 and weather.temperature >= 18:
            self.dashboard.alerts.insert(
                0,
                Alert(
                    id="alert-weather-mildew",
                    title="Weather pattern can raise mildew risk",
                    description=f"Relative humidity is {weather.humidity}% around Viterbo with mild temperatures. Schedule canopy inspection if leaves remain wet.",
                    severity="warning",
                    timestamp=timestamp,
                ),
            )

    def _plot_label(self, plot_id: str) -> str:
        plot = next((item for item in self.dashboard.plots if item.id == plot_id), None)
        return f"{plot.name} - {plot.fieldName}" if plot else "selected plot"

    def _upsert_sensor_alert(self, reading: SensorReading) -> None:
        self.dashboard.alerts = [
            alert for alert in self.dashboard.alerts if alert.id not in {"alert-live-low-moisture", "alert-live-low-battery"}
        ]
        if reading.soilMoisture < 20:
            self.dashboard.alerts.insert(
                0,
                Alert(
                    id="alert-live-low-moisture",
                    plotId=reading.plotId,
                    title="Low soil moisture detected from live sensor",
                    description=f"{reading.sensorId} reports {reading.soilMoisture:.1f}% soil moisture. Review irrigation priority within 24 hours.",
                    severity="critical",
                    timestamp=reading.timestamp or now_iso(),
                ),
            )
        if reading.batteryLevel < 25:
            self.dashboard.alerts.insert(
                0,
                Alert(
                    id="alert-live-low-battery",
                    plotId=reading.plotId,
                    title="Sensor battery below threshold",
                    description=f"{reading.sensorId} battery is at {reading.batteryLevel}%. Schedule replacement before the next monitoring cycle.",
                    severity="warning",
                    timestamp=reading.timestamp or now_iso(),
                ),
            )
