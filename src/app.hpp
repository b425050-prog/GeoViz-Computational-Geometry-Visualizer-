#pragma once
#include "session_files.hpp"
#include "ui_support.hpp"

namespace gv::ui {
// Coordinates workspace commands, input and presentation through focused interfaces.
class App {
    std::unique_ptr<AlgorithmRunner> engine = std::make_unique<Engine>();
    Scene scene = sampleScene(Mode::Hull, 1);
    Scene labScene = scene;
    std::string labInputName = sampleName(Mode::Hull, 1) + " | 2/6";
    std::uint64_t labInputRevision = scene.revision();
    std::vector<Point> fenceDraft;
    Mode labMode = Mode::Hull;
    View labView;
    int labSelected = -1, labEdge = -1;
    EditHistory edits;
    std::string savedLabKey, savedSecurityKey;
    bool labDirty = false, securityDirty = false;
    std::filesystem::path sessionPath, labSessionPath, securitySessionPath;
    std::vector<std::string> recentFiles;
    bool samplePicker = false, compareVisible = false, presentation = false;
    bool noteOpen = false, sensitivityVisible = false;
    std::string sensitivityCache;
    std::string noteText, noteVerdict;
    TextCursor noteCursor;
    int noteFocus = 0;
    std::unique_ptr<Run> comparison;
    int sampleVariant = 1, securitySample = 0;
    std::vector<SecuritySample> securityCatalogue;
    std::map<std::pair<int, int>, std::string> sampleExpectations;
    std::string securityDataset = "Compact reference";
    std::set<int> pressedKeys;
    Mode mode = Mode::Hull;
    std::unique_ptr<Run> run, previous;
    Playback playback;
    View view;
    SecurityConsole security;
    std::vector<Observation> observations;
    std::vector<MapLine> map;
    std::string evaluationCache;
    bool evaluationDirty = true;
    int securityView = 0; // 0 lab, 1 geographic, 2 behavioural.
    int selected = -1, selectedEdge = -1, edgeStart = -1, drag = -1, polygonDrag = -1;
    bool polygonTool = false, help = false;
    SettingsPanel settings;
    std::string status = "Click to add/select points; drag to move. Run records decisions.";
    std::filesystem::path root;
    SessionFiles files;
    bool smoke = false;
    bool closeRequested = false;
    int smokeFrame = 0;
    double sidebarScroll = 0;
    double helpScroll = 0;
    int windowWidth = 1440, windowHeight = 900;
    enum class RoutePick { None, Source, Destination };
    RoutePick routePick = RoutePick::None;

    // App commands
    Rectangle sidebarBounds() const;
    void updateLayout();
    bool pickEndpoint(int id);
    bool feature() const;
    bool modeAvailable(Mode candidate) const;
    void setMode(Mode candidate);
    void invalidateSecurityDisplays();
    void invalidate();
    void syncFence(bool required = false);
    void publishSecurity(PreparedSecurity prepared);
    void refreshSecurity();
    void resetFilters();
    void resetInteractions();
    WorkspaceView workspaceView() const;
    WorkspaceState workspace() const;
    void refreshDirty();
    bool requestClose();
    std::string labKey() const;
    std::string securityKey() const;
    bool dirty() const;
    void markSaved(bool laboratory);
    void recover();
    void protectWork();
    void finalFrame();
    void recordEdit();
    void restoreEdit(bool redo);
    void chooseSample(int index);
    void nextSample();
    void setView(int v);
    void execute();
    void loadFile(const std::string &path);
    void openNamed();
    void saveNamed();
    void save();
    void exportEvidence();
    void compare();
    void comparePrevious();
    void openSettings();
    void beginModal();
    void openSamples();
    void openReview(const std::string &verdict);
    void applySettings();
    void removeSelected();
    void review(const std::string &verdict, const std::string &note = "");
    void clearScene();
    void loadGrid();
    void clearFence();
    void togglePresentation();

    // App input
    bool pressed(int key) const;
    void input();

    // App inspector
    const Frame *frame() const;
    void canvas() const;
    void sidebar();

    // App render
    void sampleDialog();
    void reviewDialog();
    void settingsDialog();
    void helpDialog();
    void draw();

    // Desktop checks
    void featureChecks();
    void smokeInteractions();
    void smokeSample(Mode selectedMode, int variant);
    bool smokeStage(int stage);
    void smokeReport() const;

  public:
    explicit App(std::filesystem::path base, bool smokeMode, int width, int height);
    void loop();
};
} // namespace gv::ui
