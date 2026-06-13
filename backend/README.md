# Brainyard Backend

FastAPI backend for the Brainyard / Vineyard Intelligence hackathon MVP.

The backend mirrors the current React frontend data contract in `UI_brainyard/src/types/vineyard.ts` and keeps the Arduino/ESP32 code untouched.

## What it does

- Serves dashboard data compatible with the existing frontend mock service.
- Accepts live soil moisture readings from a sensor or manual curl request.
- Records field events.
- Creates AI-assisted study summaries using OpenAI when `OPENAI_API_KEY` is configured.
- Falls back to a deterministic local insight engine when OpenAI is not configured, so the demo never blocks.

## Setup

```bash
cd backend
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Optional OpenAI configuration:

```bash
cp .env.example .env
export OPENAI_API_KEY="your-key-here"
export OPENAI_MODEL="gpt-4o-mini"
```

The code reads environment variables directly. If you use `.env`, load it before starting the server or add your own dotenv loader.

## Run

```bash
cd backend
uvicorn app.main:app --reload --host 127.0.0.1 --port 8000
```

Open API docs:

```text
http://127.0.0.1:8000/docs
```

## Test

```bash
cd backend
python3 -m pytest -q
```

## Endpoints

### `GET /health`

Returns service status and whether OpenAI is enabled.

### `GET /api/dashboard`

Returns a `DashboardData` object matching the frontend TypeScript contract:

- `vineyard`
- `plots`
- `sensors`
- `measurements`
- `fieldEvents`
- `alerts`
- `activities`
- `studies`
- `irrigationMarkers`

### `POST /api/sensor/readings`

Accepts a live soil moisture reading and updates the dashboard state.

Example:

```bash
curl -X POST http://127.0.0.1:8000/api/sensor/readings \
  -H 'Content-Type: application/json' \
  -d '{
    "sensorId": "soil-01",
    "plotId": "plot-a",
    "soilMoisture": 44.5,
    "batteryLevel": 76,
    "status": "Online",
    "timestamp": "2026-06-13T16:00:00+02:00"
  }'
```

### `POST /sensor`

Compatibility endpoint for the current ESP32 firmware in `post_to_database.cpp`.

The firmware sends raw values:

```json
{
  "temperature": 27.4,
  "moisture": 2245,
  "battery": 1238
}
```

The backend maps them to the dashboard contract:

- `sensorId`: `soil-01`
- `plotId`: `plot-a`
- `soilMoisture`: converted with the same formula used by `how_moist()` in the firmware
- `batteryLevel`: approximate raw-to-percent conversion for the hackathon demo

When the ESP32 is on the same network as the backend machine, run FastAPI with `--host 0.0.0.0` and point the firmware URL to the machine LAN IP.

### `GET /api/sensor/latest`

Returns the latest ingested live sensor reading, or `null` if none was sent in this server session.

### `POST /api/field-events`

Creates a field event compatible with the frontend `FieldEvent` type.

### `POST /api/studies`

Creates a study compatible with the frontend `Study` type.

If `OPENAI_API_KEY` is available, this endpoint calls OpenAI to generate:

- `observation`
- `interpretation`
- `limitations`
- `suggestedNextStep`

If the key is missing or the API call fails, it returns a local deterministic fallback with the same fields.

## Demo narrative supported

1. A soil sensor sends a moisture reading.
2. The backend updates the current plot/sensor state.
3. The dashboard combines real reading plus vineyard mock context.
4. The study endpoint uses OpenAI or fallback logic to generate decision-support interpretation.
5. The frontend can show a believable vineyard intelligence workflow in under 3 minutes.
