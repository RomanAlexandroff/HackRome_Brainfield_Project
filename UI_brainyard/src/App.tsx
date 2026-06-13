import { useEffect, useMemo, useState } from "react";
import { EventForm } from "./components/EventForm";
import { FieldObservationDetail } from "./components/FieldObservationDetail";
import { FieldObservationForm } from "./components/FieldObservationForm";
import { Header } from "./components/Header";
import { LoadingState } from "./components/LoadingState";
import { Sidebar, type NavigationKey } from "./components/Sidebar";
import { StudyForm } from "./components/StudyForm";
import { StudyResult } from "./components/StudyResult";
import { fieldObservationService } from "./services/fieldObservationService";
import { flywheelService } from "./services/flywheelService";
import { vineyardService } from "./services/vineyardService";
import type { FieldObservation, FieldObservationDraft } from "./types/fieldObservation";
import type {
  Activity,
  DashboardData,
  FieldEventDraft,
  FlywheelSyncState,
  StudyDraft,
} from "./types/vineyard";
import { buildPlotHistoryData, getMarkersForPlot } from "./utils/chartData";
import { generateId } from "./utils/formatters";
import { FieldLogView } from "./views/FieldLogView";
import { FieldObservationsView } from "./views/FieldObservationsView";
import { OverviewView } from "./views/OverviewView";
import { PlotDetailView } from "./views/PlotDetailView";
import { PlotsView } from "./views/PlotsView";
import { SettingsView } from "./views/SettingsView";
import { StudiesView } from "./views/StudiesView";

type AppView =
  | NavigationKey
  | "plot-detail"
  | "create-study"
  | "study-result"
  | "add-field-observation"
  | "field-observation-detail";

function getSidebarView(activeView: AppView): NavigationKey {
  if (activeView === "plot-detail") {
    return "plots";
  }

  if (activeView === "create-study" || activeView === "study-result") {
    return "studies";
  }

  if (activeView === "add-field-observation" || activeView === "field-observation-detail") {
    return "field-observations";
  }

  return activeView;
}

function App() {
  const [dashboardData, setDashboardData] = useState<DashboardData | null>(null);
  const [fieldObservations, setFieldObservations] = useState<FieldObservation[]>([]);
  const [activeView, setActiveView] = useState<AppView>("overview");
  const [selectedPlotId, setSelectedPlotId] = useState("plot-a");
  const [selectedStudyId, setSelectedStudyId] = useState("study-irrigation-a");
  const [selectedObservationId, setSelectedObservationId] = useState("");
  const [isEventFormOpen, setIsEventFormOpen] = useState(false);
  const [isSavingEvent, setIsSavingEvent] = useState(false);
  const [isCreatingStudy, setIsCreatingStudy] = useState(false);
  const [isSavingObservation, setIsSavingObservation] = useState(false);
  const [synchronizingStudyId, setSynchronizingStudyId] = useState<string | null>(null);
  const [toastMessage, setToastMessage] = useState("");
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState("");

  useEffect(() => {
    let isMounted = true;

    Promise.all([vineyardService.getDashboardData(), fieldObservationService.getFieldObservations()])
      .then(([data, observations]) => {
        if (!isMounted) {
          return;
        }

        setDashboardData(data);
        setSelectedPlotId(data.plots[0]?.id ?? "plot-a");
        setSelectedStudyId(data.studies[0]?.id ?? "");
        setFieldObservations(observations);
        setSelectedObservationId(observations[0]?.id ?? "");
      })
      .catch(() => {
        if (isMounted) {
          setError("Brainyard could not load vineyard data from the backend.");
        }
      })
      .finally(() => {
        if (isMounted) {
          setIsLoading(false);
        }
      });

    return () => {
      isMounted = false;
    };
  }, []);

  useEffect(() => {
    const interval = window.setInterval(() => {
      vineyardService
        .getDashboardData()
        .then((data) => {
          setDashboardData(data);
          setError("");
        })
        .catch(() => {
          setError("Brainyard could not refresh live vineyard data.");
        });
    }, 7000);

    return () => window.clearInterval(interval);
  }, []);

  const selectedPlot = useMemo(
    () => dashboardData?.plots.find((plot) => plot.id === selectedPlotId),
    [dashboardData?.plots, selectedPlotId],
  );

  const selectedStudy = useMemo(
    () => dashboardData?.studies.find((study) => study.id === selectedStudyId),
    [dashboardData?.studies, selectedStudyId],
  );

  const selectedObservation = useMemo(
    () => fieldObservations.find((observation) => observation.id === selectedObservationId),
    [fieldObservations, selectedObservationId],
  );

  const pageTitle = useMemo(() => {
    if (activeView === "overview") {
      return "Vineyard Overview";
    }

    if (activeView === "plots") {
      return "Plots";
    }

    if (activeView === "field-log") {
      return "Field Log";
    }

    if (activeView === "studies") {
      return "Studies";
    }

    if (activeView === "field-observations") {
      return "Field Observations";
    }

    if (activeView === "settings") {
      return "Settings";
    }

    if (activeView === "plot-detail" && selectedPlot) {
      return `${selectedPlot.name} Detail`;
    }

    if (activeView === "create-study") {
      return "Create Study";
    }

    if (activeView === "study-result") {
      return "Study Result";
    }

    if (activeView === "add-field-observation") {
      return "Add Field Observation";
    }

    if (activeView === "field-observation-detail") {
      return "Field Observation Detail";
    }

    return "Brainyard";
  }, [activeView, selectedPlot]);

  useEffect(() => {
    document.title = `Brainyard | ${pageTitle}`;
  }, [pageTitle]);

  useEffect(() => {
    if (!toastMessage) {
      return;
    }

    const timeout = window.setTimeout(() => setToastMessage(""), 4200);
    return () => window.clearTimeout(timeout);
  }, [toastMessage]);

  function handleNavigate(view: NavigationKey) {
    setActiveView(view);
  }

  function handleSelectPlot(plotId: string) {
    setSelectedPlotId(plotId);
    setActiveView("plot-detail");
  }

  async function handleCreateFieldEvent(draft: FieldEventDraft) {
    if (!dashboardData) {
      return;
    }

    setIsSavingEvent(true);

    try {
      const fieldEvent = await vineyardService.createFieldEvent(draft);
      const plot = dashboardData.plots.find((item) => item.id === fieldEvent.plotId);
      const plotLabel = plot ? `${plot.name} - ${plot.fieldName}` : "selected plot";
      const activity: Activity = {
        id: generateId("activity"),
        type: fieldEvent.type === "Irrigation" ? "irrigation" : "event",
        title: `${fieldEvent.type} recorded`,
        description: `${fieldEvent.durationMinutes}-minute ${fieldEvent.type.toLowerCase()} added for ${plotLabel}.`,
        timestamp: fieldEvent.createdAt,
      };

      setDashboardData((current) =>
        current
          ? {
              ...current,
              fieldEvents: [fieldEvent, ...current.fieldEvents],
              activities: [activity, ...current.activities],
            }
          : current,
      );
      setIsEventFormOpen(false);
      setToastMessage("Field event recorded successfully.");
    } finally {
      setIsSavingEvent(false);
    }
  }

  async function handleCreateStudy(draft: StudyDraft) {
    setIsCreatingStudy(true);

    try {
      const study = await vineyardService.createStudy(draft);
      setDashboardData((current) =>
        current
          ? {
              ...current,
              studies: [study, ...current.studies],
            }
          : current,
      );
      setSelectedStudyId(study.id);
      setActiveView("study-result");
      setToastMessage("Study created and evidence package prepared.");
    } finally {
      setIsCreatingStudy(false);
    }
  }

  async function handleCreateFieldObservation(draft: FieldObservationDraft) {
    setIsSavingObservation(true);

    try {
      const observation = await fieldObservationService.createFieldObservation(draft);
      const activity: Activity = {
        id: generateId("activity"),
        type: "note",
        title: "Field observation recorded",
        description: `${observation.plotName} · ${observation.sampledVines} vines sampled.`,
        timestamp: observation.createdAt,
      };

      setFieldObservations((current) => [observation, ...current]);
      setDashboardData((current) =>
        current
          ? {
              ...current,
              activities: [activity, ...current.activities],
            }
          : current,
      );
      setSelectedObservationId(observation.id);
      setActiveView("field-observation-detail");
      setToastMessage("Field observation saved successfully.");
    } finally {
      setIsSavingObservation(false);
    }
  }

  function updateStudySyncState(studyId: string, syncState: FlywheelSyncState) {
    setDashboardData((current) =>
      current
        ? {
            ...current,
            studies: current.studies.map((study) =>
              study.id === studyId
                ? {
                    ...study,
                    syncState,
                    status: syncState === "synchronized" ? "Synchronized" : study.status,
                  }
                : study,
            ),
          }
        : current,
    );
  }

  async function handleSynchronizeStudy() {
    if (!selectedStudy) {
      return;
    }

    setSynchronizingStudyId(selectedStudy.id);

    try {
      await flywheelService.synchronizeStudy(selectedStudy.id, (state) => {
        updateStudySyncState(selectedStudy.id, state);
      });
      setToastMessage("Evidence package synchronized with Flywheel.");
    } finally {
      setSynchronizingStudyId(null);
    }
  }

  if (isLoading) {
    return (
      <main className="standalone-state">
        <LoadingState />
      </main>
    );
  }

  if (error || !dashboardData) {
    return (
      <main className="standalone-state">
        <section className="state-card state-card--error">
          <h1>Brainyard</h1>
          <p>{error || "Local vineyard data is unavailable."}</p>
        </section>
      </main>
    );
  }

  const selectedStudyPlot = selectedStudy
    ? dashboardData.plots.find((plot) => plot.id === selectedStudy.plotId)
    : undefined;

  return (
    <div className="app-shell">
      <Sidebar activeView={getSidebarView(activeView)} onNavigate={handleNavigate} />

      <div className="app-content">
        <Header
          pageTitle={pageTitle}
          vineyard={dashboardData.vineyard}
          onCreateStudy={() => setActiveView("create-study")}
          onRecordEvent={() => setIsEventFormOpen(true)}
        />

        {toastMessage ? (
          <div className="toast" role="status" aria-live="polite">
            {toastMessage}
          </div>
        ) : null}

        <main>
          {activeView === "overview" ? (
            <OverviewView
              activities={dashboardData.activities}
              alerts={dashboardData.alerts}
              irrigationMarkers={dashboardData.irrigationMarkers}
              fieldObservations={fieldObservations}
              measurements={dashboardData.measurements}
              plots={dashboardData.plots}
              onAddObservation={() => setActiveView("add-field-observation")}
              onSelectPlot={handleSelectPlot}
              onViewFieldObservations={() => setActiveView("field-observations")}
            />
          ) : null}

          {activeView === "plots" ? (
            <PlotsView plots={dashboardData.plots} onSelectPlot={handleSelectPlot} />
          ) : null}

          {activeView === "plot-detail" && selectedPlot ? (
            <PlotDetailView
              fieldEvents={dashboardData.fieldEvents}
              fieldObservations={fieldObservations}
              irrigationMarkers={dashboardData.irrigationMarkers}
              measurements={dashboardData.measurements}
              plot={selectedPlot}
              sensors={dashboardData.sensors}
              onAddObservation={() => setActiveView("add-field-observation")}
              onAnalyzeIrrigationResponse={() => setActiveView("create-study")}
              onBack={() => setActiveView("overview")}
              onOpenObservation={(observationId) => {
                setSelectedObservationId(observationId);
                setActiveView("field-observation-detail");
              }}
            />
          ) : null}

          {activeView === "field-log" ? (
            <FieldLogView
              fieldEvents={dashboardData.fieldEvents}
              plots={dashboardData.plots}
              onRecordEvent={() => setIsEventFormOpen(true)}
            />
          ) : null}

          {activeView === "field-observations" ? (
            <FieldObservationsView
              observations={fieldObservations}
              plots={dashboardData.plots}
              onAddObservation={() => setActiveView("add-field-observation")}
              onOpenObservation={(observationId) => {
                setSelectedObservationId(observationId);
                setActiveView("field-observation-detail");
              }}
            />
          ) : null}

          {activeView === "studies" ? (
            <StudiesView
              plots={dashboardData.plots}
              studies={dashboardData.studies}
              onCreateStudy={() => setActiveView("create-study")}
              onOpenStudy={(studyId) => {
                setSelectedStudyId(studyId);
                setActiveView("study-result");
              }}
            />
          ) : null}

          {activeView === "create-study" ? (
            <StudyForm
              fieldEvents={dashboardData.fieldEvents}
              isSubmitting={isCreatingStudy}
              plots={dashboardData.plots}
              onCancel={() => setActiveView("studies")}
              onSubmit={handleCreateStudy}
            />
          ) : null}

          {activeView === "add-field-observation" ? (
            <FieldObservationForm
              defaultPlotId={selectedPlotId}
              isSubmitting={isSavingObservation}
              plots={dashboardData.plots}
              vineyard={dashboardData.vineyard}
              onCancel={() => setActiveView("field-observations")}
              onSubmit={handleCreateFieldObservation}
            />
          ) : null}

          {activeView === "field-observation-detail" && selectedObservation ? (
            <FieldObservationDetail
              observation={selectedObservation}
              onBack={() => setActiveView("field-observations")}
            />
          ) : null}

          {activeView === "study-result" && selectedStudy && selectedStudyPlot ? (
            <StudyResult
              chartData={buildPlotHistoryData(dashboardData.measurements, selectedStudy.plotId)}
              isSynchronizing={synchronizingStudyId === selectedStudy.id}
              markers={getMarkersForPlot(dashboardData.irrigationMarkers, selectedStudy.plotId)}
              plot={selectedStudyPlot}
              study={selectedStudy}
              syncState={selectedStudy.syncState}
              onBack={() => setActiveView("studies")}
              onSynchronize={handleSynchronizeStudy}
            />
          ) : null}

          {activeView === "settings" ? <SettingsView /> : null}
        </main>
      </div>

      {isEventFormOpen ? (
        <EventForm
          isSubmitting={isSavingEvent}
          plots={dashboardData.plots}
          onClose={() => setIsEventFormOpen(false)}
          onSubmit={handleCreateFieldEvent}
        />
      ) : null}
    </div>
  );
}

export default App;
