from dataclasses import asdict
from typing import Optional

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from .mock_data import fresh_dashboard
from .models import DashboardData, FieldEvent, FieldEventDraft, RawSensorPayload, SensorReading, Study, StudyDraft
from .openai_service import openai_enabled
from .store import VineyardStore
from .weather_service import fetch_viterbo_weather

app = FastAPI(
    title="Brainyard Backend",
    description="Hackathon MVP backend for AI-assisted vineyard monitoring and decision support.",
    version="0.1.0",
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)

store = VineyardStore(fresh_dashboard())


def _clamp(value: float, minimum: float, maximum: float) -> float:
    return max(minimum, min(maximum, value))


def moisture_raw_to_percent(raw_moisture: int) -> float:
    # Same conversion used by the ESP32 firmware how_moist() helper.
    return round(_clamp(((3180 - raw_moisture) / 1870.0) * 100.0, 0, 100), 1)


def battery_raw_to_percent(raw_battery: int) -> int:
    # Hackathon-friendly approximation: 875 is the firmware low-battery threshold.
    return int(round(_clamp(((raw_battery - 875) / (1600 - 875)) * 100.0, 0, 100)))


def raw_payload_to_sensor_reading(payload: RawSensorPayload) -> SensorReading:
    battery_level = battery_raw_to_percent(payload.battery)
    return SensorReading(
        sensorId="soil-01",
        plotId="plot-a",
        soilMoisture=moisture_raw_to_percent(payload.moisture),
        batteryLevel=battery_level,
        status="Low battery" if battery_level < 25 else "Online",
    )


@app.get("/health")
def health() -> dict:
    return {
        "status": "ok",
        "service": "brainyard-backend",
        "ai_enabled": openai_enabled(),
    }


@app.get("/api/dashboard", response_model=DashboardData)
def get_dashboard() -> DashboardData:
    return store.get_dashboard()


@app.get("/api/weather")
def get_weather() -> dict:
    return asdict(fetch_viterbo_weather())


@app.get("/api/sensor/latest", response_model=Optional[SensorReading])
def get_latest_sensor_reading():
    return store.get_latest_sensor_reading()


@app.post("/api/sensor/readings", response_model=SensorReading)
def ingest_sensor_reading(reading: SensorReading) -> SensorReading:
    return store.ingest_sensor_reading(reading)


@app.post("/sensor", response_model=SensorReading)
def ingest_raw_esp32_sensor_payload(payload: RawSensorPayload) -> SensorReading:
    """Compatibility endpoint for the current ESP32 firmware post_to_database.cpp."""
    return store.ingest_sensor_reading(raw_payload_to_sensor_reading(payload))


@app.post("/api/field-events", response_model=FieldEvent)
def create_field_event(draft: FieldEventDraft) -> FieldEvent:
    return store.create_field_event(draft)


@app.post("/api/studies", response_model=Study)
def create_study(draft: StudyDraft) -> Study:
    return store.create_study(draft)
