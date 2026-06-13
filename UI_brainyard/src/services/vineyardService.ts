import { dashboardData } from "../data/mockData";
import type {
  DashboardData,
  FieldEvent,
  FieldEventDraft,
  Study,
  StudyDraft,
} from "../types/vineyard";
import { generateId } from "../utils/formatters";

const API_BASE_URL = import.meta.env.VITE_API_BASE_URL ?? "http://127.0.0.1:8000";
const MOCK_DELAY_MS = 350;

function delay<T>(data: T, duration = MOCK_DELAY_MS): Promise<T> {
  return new Promise((resolve) => {
    window.setTimeout(() => resolve(data), duration);
  });
}

function cloneMockDashboard(): DashboardData {
  return {
    ...dashboardData,
    plots: [...dashboardData.plots],
    sensors: [...dashboardData.sensors],
    measurements: [...dashboardData.measurements],
    fieldEvents: [...dashboardData.fieldEvents],
    alerts: [...dashboardData.alerts],
    activities: [...dashboardData.activities],
    studies: [...dashboardData.studies],
    irrigationMarkers: [...dashboardData.irrigationMarkers],
  };
}

async function fetchJson<T>(path: string, init?: RequestInit): Promise<T> {
  const response = await fetch(`${API_BASE_URL}${path}`, {
    headers: {
      "Content-Type": "application/json",
      ...(init?.headers ?? {}),
    },
    ...init,
  });

  if (!response.ok) {
    throw new Error(`Backend request failed: ${response.status} ${response.statusText}`);
  }

  return response.json() as Promise<T>;
}

function createMockFieldEvent(draft: FieldEventDraft): FieldEvent {
  return {
    id: generateId("field-event"),
    createdAt: new Date().toISOString(),
    ...draft,
  };
}

function createMockStudy(draft: StudyDraft): Study {
  return {
    id: generateId("study"),
    title: "Irrigation response in Plot A",
    researchQuestion: draft.researchQuestion,
    plotId: draft.plotId,
    dateRange: draft.dateRange,
    metrics: draft.metrics,
    relatedFieldEventId: draft.relatedFieldEventId,
    notes: draft.notes,
    status: "Ready for review",
    observation: "Soil moisture increased from 21.3% to 33.8% after the irrigation event.",
    evidence: {
      sensorReadings: 144,
      fieldEvents: 1,
      observationWindow: "24-hour observation window",
      generatedArtifacts: 2,
    },
    interpretation: "The irrigation produced a measurable increase in soil moisture near sensor soil-01.",
    limitations:
      "Only one irrigation event was analyzed. The measurement represents the area surrounding one sensor and should not be generalized to the entire plot.",
    suggestedNextStep:
      "Repeat the same observation for at least three irrigation events and compare measurements at different soil depths.",
    artifacts: [
      {
        id: "artifact-measurements",
        name: "measurements.csv",
        type: "csv",
        size: "18 KB",
      },
      {
        id: "artifact-metadata",
        name: "study-metadata.json",
        type: "json",
        size: "4 KB",
      },
      {
        id: "artifact-chart",
        name: "soil-moisture-chart.png",
        type: "image",
        size: "92 KB",
      },
    ],
    syncState: "not_synchronized",
    createdAt: new Date().toISOString(),
  };
}

export const vineyardService = {
  async getDashboardData(): Promise<DashboardData> {
    try {
      return await fetchJson<DashboardData>("/api/dashboard");
    } catch (error) {
      console.warn("Falling back to local mock dashboard data", error);
      return delay(cloneMockDashboard());
    }
  },

  async createFieldEvent(draft: FieldEventDraft): Promise<FieldEvent> {
    try {
      return await fetchJson<FieldEvent>("/api/field-events", {
        method: "POST",
        body: JSON.stringify(draft),
      });
    } catch (error) {
      console.warn("Falling back to local mock field event", error);
      return delay(createMockFieldEvent(draft), 250);
    }
  },

  async createStudy(draft: StudyDraft): Promise<Study> {
    try {
      return await fetchJson<Study>("/api/studies", {
        method: "POST",
        body: JSON.stringify(draft),
      });
    } catch (error) {
      console.warn("Falling back to local mock study", error);
      return delay(createMockStudy(draft), 350);
    }
  },
};
