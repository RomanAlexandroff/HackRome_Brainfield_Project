from fastapi.testclient import TestClient

from app.main import app


client = TestClient(app)


def test_health_endpoint_reports_service_ok():
    response = client.get("/health")

    assert response.status_code == 200
    assert response.json() == {
        "status": "ok",
        "service": "brainyard-backend",
        "ai_enabled": False,
    }


def test_dashboard_shape_matches_frontend_contract():
    response = client.get("/api/dashboard")

    assert response.status_code == 200
    data = response.json()
    assert set(data.keys()) == {
        "vineyard",
        "plots",
        "sensors",
        "measurements",
        "fieldEvents",
        "alerts",
        "activities",
        "studies",
        "irrigationMarkers",
    }
    assert data["vineyard"]["name"] == "Tenuta Verde"
    assert data["vineyard"]["location"] == "Tuscia, Viterbo, Italy"
    assert len(data["plots"]) >= 3
    assert data["plots"][0]["id"] == "plot-a"
    assert data["sensors"][0]["type"] == "Soil moisture"
    assert data["alerts"]


def test_weather_endpoint_returns_viterbo_context():
    response = client.get("/api/weather")

    assert response.status_code == 200
    data = response.json()
    assert data["location"] == "Tuscia, Viterbo, Italy"
    assert data["source"] in {"open-meteo", "fallback"}
    assert "temperature" in data
    assert "rain_next_72h" in data
    assert data["forecast_summary"]


def test_sensor_reading_updates_dashboard_plot_sensor_and_latest_reading():
    payload = {
        "sensorId": "soil-01",
        "plotId": "plot-a",
        "soilMoisture": 44.5,
        "batteryLevel": 76,
        "status": "Online",
        "timestamp": "2026-06-13T16:00:00+02:00",
    }

    response = client.post("/api/sensor/readings", json=payload)
    assert response.status_code == 200
    latest = response.json()
    assert latest["sensorId"] == "soil-01"
    assert latest["soilMoisture"] == 44.5

    dashboard = client.get("/api/dashboard").json()
    plot_a = next(plot for plot in dashboard["plots"] if plot["id"] == "plot-a")
    soil_01 = next(sensor for sensor in dashboard["sensors"] if sensor["id"] == "soil-01")
    assert plot_a["soilMoisture"] == 44.5
    assert soil_01["lastReading"] == 44.5
    assert soil_01["batteryLevel"] == 76
    assert dashboard["measurements"][-1]["soilMoisture"] == 44.5


def test_raw_esp32_sensor_endpoint_maps_firmware_payload():
    response = client.post(
        "/sensor",
        json={"temperature": 27.4, "moisture": 2245, "battery": 1238},
    )

    assert response.status_code == 200
    data = response.json()
    assert data["sensorId"] == "soil-01"
    assert data["plotId"] == "plot-a"
    assert 49 <= data["soilMoisture"] <= 51
    assert 49 <= data["batteryLevel"] <= 51


def test_create_field_event_returns_frontend_compatible_event():
    payload = {
        "plotId": "plot-b",
        "type": "Irrigation",
        "date": "2026-06-13",
        "time": "18:00",
        "durationMinutes": 45,
        "notes": "Emergency irrigation after low moisture alert.",
    }

    response = client.post("/api/field-events", json=payload)

    assert response.status_code == 200
    data = response.json()
    assert data["id"].startswith("field-event-")
    assert data["plotId"] == "plot-b"
    assert data["type"] == "Irrigation"
    assert data["createdAt"]


def test_create_study_returns_ai_like_fields_without_openai_key(monkeypatch):
    monkeypatch.delenv("OPENAI_API_KEY", raising=False)
    payload = {
        "researchQuestion": "Should Plot B be irrigated in the next 24 hours?",
        "plotId": "plot-b",
        "dateRange": {"start": "2026-06-12", "end": "2026-06-13"},
        "metrics": ["Soil moisture", "Air temperature"],
        "relatedFieldEventId": "event-inspection-b",
        "notes": "Focus on water stress and dry conditions.",
    }

    response = client.post("/api/studies", json=payload)

    assert response.status_code == 200
    data = response.json()
    assert data["id"].startswith("study-")
    assert data["status"] == "Ready for review"
    assert data["syncState"] == "not_synchronized"
    assert "Plot B" in data["title"]
    assert data["observation"]
    assert data["interpretation"]
    assert data["limitations"]
    assert data["suggestedNextStep"]
    assert data["evidence"]["sensorReadings"] > 0
    assert len(data["artifacts"]) == 3
