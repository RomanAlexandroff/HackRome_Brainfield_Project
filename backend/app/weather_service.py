import json
import time
from dataclasses import dataclass
from typing import Any, Optional
from urllib import error, parse, request


VITERBO_LATITUDE = 42.4207
VITERBO_LONGITUDE = 12.1077
WEATHER_CACHE_TTL_SECONDS = 15 * 60


@dataclass
class WeatherSnapshot:
    source: str
    location: str
    temperature: float
    humidity: int
    rainfall: float
    wind_speed: float
    rain_next_12h: float
    rain_next_72h: float
    max_temperature_next_72h: float
    forecast_summary: str


_cached_snapshot: Optional[WeatherSnapshot] = None
_cached_at = 0.0


def _safe_float(value: Any, default: float = 0.0) -> float:
    try:
        if value is None:
            return default
        return float(value)
    except (TypeError, ValueError):
        return default


def _safe_int(value: Any, default: int = 0) -> int:
    try:
        if value is None:
            return default
        return int(round(float(value)))
    except (TypeError, ValueError):
        return default


def _build_summary(snapshot: WeatherSnapshot) -> str:
    if snapshot.rain_next_12h >= 5:
        return "Rain is expected in the next 12 hours; delay non-critical irrigation and monitor runoff."
    if snapshot.rain_next_72h >= 8:
        return "Rain is likely within 72 hours; irrigation can be reduced unless live soil moisture drops sharply."
    if snapshot.temperature >= 30 and snapshot.humidity < 45:
        return "Hot and dry conditions around Tuscia; water stress can increase quickly in exposed plots."
    if snapshot.humidity >= 75 and snapshot.temperature >= 18:
        return "Humid conditions may increase disease pressure; keep an eye on mildew risk."
    return "Stable Tuscia weather conditions; combine the forecast with live soil moisture before irrigating."


def _fallback_weather() -> WeatherSnapshot:
    snapshot = WeatherSnapshot(
        source="fallback",
        location="Tuscia, Viterbo, Italy",
        temperature=27.0,
        humidity=48,
        rainfall=0.0,
        wind_speed=11.0,
        rain_next_12h=0.0,
        rain_next_72h=1.0,
        max_temperature_next_72h=30.0,
        forecast_summary="Fallback weather: warm and mostly dry around Tuscia; use live soil moisture to prioritize irrigation.",
    )
    return snapshot


def fetch_viterbo_weather(force_refresh: bool = False) -> WeatherSnapshot:
    global _cached_at, _cached_snapshot

    now = time.time()
    if not force_refresh and _cached_snapshot and now - _cached_at < WEATHER_CACHE_TTL_SECONDS:
        return _cached_snapshot

    query = parse.urlencode(
        {
            "latitude": VITERBO_LATITUDE,
            "longitude": VITERBO_LONGITUDE,
            "current": "temperature_2m,relative_humidity_2m,wind_speed_10m,rain",
            "hourly": "temperature_2m,relative_humidity_2m,precipitation_probability,rain,wind_speed_10m",
            "forecast_days": 3,
            "timezone": "Europe/Rome",
        }
    )
    url = f"https://api.open-meteo.com/v1/forecast?{query}"

    try:
        with request.urlopen(url, timeout=8) as response:
            payload = json.load(response)
    except (OSError, error.URLError, TimeoutError, json.JSONDecodeError):
        snapshot = _fallback_weather()
        _cached_snapshot = snapshot
        _cached_at = now
        return snapshot

    current = payload.get("current", {})
    hourly = payload.get("hourly", {})
    hourly_rain = [_safe_float(item) for item in hourly.get("rain", [])]
    hourly_temperatures = [_safe_float(item) for item in hourly.get("temperature_2m", [])]

    snapshot = WeatherSnapshot(
        source="open-meteo",
        location="Tuscia, Viterbo, Italy",
        temperature=round(_safe_float(current.get("temperature_2m"), 27.0), 1),
        humidity=_safe_int(current.get("relative_humidity_2m"), 50),
        rainfall=round(_safe_float(current.get("rain"), 0.0), 1),
        wind_speed=round(_safe_float(current.get("wind_speed_10m"), 0.0), 1),
        rain_next_12h=round(sum(hourly_rain[:12]), 1),
        rain_next_72h=round(sum(hourly_rain[:72]), 1),
        max_temperature_next_72h=round(max(hourly_temperatures[:72] or [27.0]), 1),
        forecast_summary="",
    )
    snapshot.forecast_summary = _build_summary(snapshot)

    _cached_snapshot = snapshot
    _cached_at = now
    return snapshot
