// main_window_model.h

#ifndef MAIN_WINDOW_MODEL_H
#define MAIN_WINDOW_MODEL_H

#include "base/user_types.h"
#include "block_visualisation/blocks_visualisation_model.h"

#include <memory>

#include <QObject>
#include <QStringList>
#include <QString>
#include <QSharedPointer>
#include <QTabWidget>

#include <list>
#include <map>

class QAction;
class QTextStream;

namespace BioSig_
{

class FileSignalReader;
class MainWindow;
class SignalBrowserModel;
class SignalBrowserView;
class EventTableFileReader;
class EventColorManager;
class BlocksVisualisationView;
class AbstractBrowserModel;

// main window model
class MainWindowModel : public QObject
{
    Q_OBJECT

public:

    enum State
    {
        STATE_EXIT,
        STATE_FILE_CLOSED,
        STATE_FILE_OPENED,
        STATE_FILE_CHANGED
    };

    enum SelectionState
    {
        SELECTION_STATE_OFF,
        SELECTION_STATE_NONE,
        SELECTION_STATE_ONE_CHANNEL,
        SELECTION_STATE_ALL_CHANNELS
    };

    MainWindowModel();
    ~MainWindowModel();

    QTextStream& getLogStream();
    QSharedPointer<EventTableFileReader> getEventTableFileReader();
    EventColorManager& getEventColorManager();
    void setMainWindow(MainWindow* main_window);
    MainWindow* getMainWindow();
    void loadSettings();
    void saveSettings();
    State getState();
    void setSelectionState(SelectionState selection_state);
    void setChanged();

    QSharedPointer<BlocksVisualisationModel> createBlocksVisualisationView (QString const& title);

public slots:

    void tabChanged (int tab_index);
    void closeTab (int tab_index);

    // actions
    void calculateMeanAction();
    void calculateFrequencySpectrumAction();
    void undoViewAction();
    void redoViewAction();
    void undoAction();
    void redoAction();
    void fileOpenAction();
    void fileSaveAction();
    void fileSaveAsAction();
    void fileExportEventsAction();
    void fileImportEventsAction();
    void fileCloseAction();
    void fileInfoAction();
    void fileExitAction();
    void editToAllChannelsAction();
    void editCopyToChannelsAction();
    void editDeleteAction();
    void editChangeChannelAction();
    void editChangeTypeAction();
    void editEventTableAction();
    void mouseModeNewAction();
    void mouseModePointerAction();
    void mouseModeHandAction();
    void mouseModeShiftSignalAction();
//    void mouseModeZoomAction();
    void viewZoomInAction();
    void viewZoomOutAction();
    void viewAutoScaleAction();
    void viewGoToAction();
    void viewShowAndSelectNextEventAction();
    void viewShowAndSelectPreviousEventAction();
    void viewShowEventsOfSelectedTypeAction();
    void viewFitToEventAction();
    void secsPerPageChanged(const QString& secs_per_page);
    void signalsPerPageChanged(const QString& signals_per_page);
    void optionsChannelsAction();
    void optionsShowEventsAction();
    void optionsShowSettingsAction();
    void helpLogAction();
    void helpAboutAction();
    void recentFileActivated(QAction* recent_file_action);
    void pixelPerSecChanged(float64 pixel_per_sec);
    void signalHeightChanged(int32 signal_height);
    void recentFileMenuAboutToShow();

    void openFile(const QString& file_name);


private:

    // not allowed
    MainWindowModel(const MainWindowModel&);
    const MainWindowModel& operator=(const MainWindowModel&);

    void setState(State state);
    bool checkMainWindowPtr(const QString function);
    bool checkNotClosedState(const QString function);
    void channelSelection ();

    MainWindow* main_window_;
    State state_;
    SelectionState selection_state_;
    std::auto_ptr<FileSignalReader> file_signal_reader_;
    QSharedPointer<SignalBrowserModel> signal_browser_model_;
    SignalBrowserView* signal_browser_; // main_window cares for destruction!!
    QTabWidget* tab_widget_;
    QWidget* signal_browser_tab_;
    QSharedPointer<EventTableFileReader> event_table_file_reader_;
    std::auto_ptr<EventColorManager> event_color_manager_;
    QStringList recent_file_list_;
    int32 number_recent_files_;
    std::auto_ptr<QTextStream> log_stream_;
    QString log_string_;
    QString secs_per_page_;
    bool overflow_detection_;
    std::map<int, QSharedPointer<AbstractBrowserModel> > browser_models_;
    std::list<QSharedPointer<BlocksVisualisationModel> > blocks_visualisation_models_;
};

} // namespace BioSig_

#endif

