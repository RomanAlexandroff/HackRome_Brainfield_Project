from datetime import datetime
from typing import List, Literal, Optional

from pydantic import BaseModel, ConfigDict, Field


PlotStatus = Literal["Healthy", "Water stress risk", "Monitoring"]
SensorStatus = Literal["Online", "Low battery", "Offline"]
AlertSeverity = Literal["critical", "warning", "info"]
FieldEventType = Literal[
    "Irrigation",
    "Rainfall",
    "Treatment",
    "Fertilization",
    "Pruning",
    "Sampling",
    "Agronomist inspection",
    "Other",
]
ActivityType = Literal["irrigation", "import", "note", "study", "event"]
StudyStatus = Literal["Draft", "Ready for review", "Synchronized"]
FlywheelSyncState = Literal[
    "not_synchronized",
    "preparing_evidence",
    "uploading_artifacts",
    "synchronized",
]


class CamelModel(BaseModel):
    model_config = ConfigDict(populate_by_name=True)


class Vineyard(CamelModel):
    id: str
    name: str
    location: str
    tagline: str
    lastSensorUpdate: str


class Plot(CamelModel):
    id: str
    name: str
    fieldName: str
    soilMoisture: float
    airTemperature: float
    airHumidity: float
    status: PlotStatus
    activeSensors: int
    areaHectares: float
    grapeVariety: str


class Sensor(CamelModel):
    id: str
    label: str
    plotId: str
    type: Literal["Soil moisture", "Air temperature", "Air humidity", "Rain gauge"]
    batteryLevel: int
    status: SensorStatus
    lastReading: float
    unit: Literal["%", "°C", "mm"]


class Measurement(CamelModel):
    id: str
    plotId: str
    timestamp: str
    soilMoisture: float
    airTemperature: float
    airHumidity: float
    rainfall: float


class FieldEvent(CamelModel):
    id: str
    plotId: str
    type: FieldEventType
    date: str
    time: str
    durationMinutes: int
    notes: str
    createdAt: str


class Alert(CamelModel):
    id: str
    title: str
    description: str
    severity: AlertSeverity
    timestamp: str
    plotId: Optional[str] = None


class Activity(CamelModel):
    id: str
    type: ActivityType
    title: str
    description: str
    timestamp: str


class Artifact(CamelModel):
    id: str
    name: str
    type: Literal["csv", "json", "image"]
    size: str


class EvidenceSummary(CamelModel):
    sensorReadings: int
    fieldEvents: int
    observationWindow: str
    generatedArtifacts: int


class DateRange(CamelModel):
    start: str
    end: str


class Study(CamelModel):
    id: str
    title: str
    researchQuestion: str
    plotId: str
    dateRange: DateRange
    metrics: List[str]
    notes: str
    status: StudyStatus
    observation: str
    evidence: EvidenceSummary
    interpretation: str
    limitations: str
    suggestedNextStep: str
    artifacts: List[Artifact]
    syncState: FlywheelSyncState
    createdAt: str
    relatedFieldEventId: Optional[str] = None


class IrrigationMarker(CamelModel):
    timestamp: str
    plotId: str
    label: str


class DashboardData(CamelModel):
    vineyard: Vineyard
    plots: List[Plot]
    sensors: List[Sensor]
    measurements: List[Measurement]
    fieldEvents: List[FieldEvent]
    alerts: List[Alert]
    activities: List[Activity]
    studies: List[Study]
    irrigationMarkers: List[IrrigationMarker]


class StudyDraft(CamelModel):
    researchQuestion: str
    plotId: str
    dateRange: DateRange
    metrics: List[str]
    notes: str = ""
    relatedFieldEventId: Optional[str] = None


class FieldEventDraft(CamelModel):
    plotId: str
    type: FieldEventType
    date: str
    time: str
    durationMinutes: int = Field(ge=0)
    notes: str = ""


class SensorReading(CamelModel):
    sensorId: str
    plotId: str
    soilMoisture: float = Field(ge=0, le=100)
    batteryLevel: int = Field(ge=0, le=100)
    status: SensorStatus = "Online"
    timestamp: Optional[str] = None


class RawSensorPayload(CamelModel):
    temperature: float
    moisture: int
    battery: int


def now_iso() -> str:
    return datetime.now().astimezone().isoformat(timespec="seconds")
