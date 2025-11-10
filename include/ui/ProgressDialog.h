/**
 * @file ProgressDialog.h
 * @brief Progress dialog for long-running operations
 *
 * Shows progress bar and status messages for:
 * - File loading
 * - File saving
 * - File export
 * - Other async operations
 *
 * Uses Qt6 under LGPL v3 license (dynamically linked)
 */

#pragma once

#ifdef KOOMESH_HAS_QT

#include <QDialog>
#include <QString>

// Forward declarations - Qt
class QProgressBar;
class QLabel;
class QPushButton;
class QVBoxLayout;

namespace koomesh {
namespace ui {

/**
 * @brief Progress dialog for async operations
 *
 * Modal dialog showing:
 * ```
 * ┌────────────────────────────┐
 * │ Loading File...            │
 * │                            │
 * │ Reading mesh data: 1.2 MB  │
 * │ [████████████░░░░░] 75%    │
 * │                            │
 * │          [Cancel]          │
 * └────────────────────────────┘
 * ```
 *
 * Features:
 * - Progress bar (0-100%)
 * - Status message updates
 * - Cancel button support
 * - Auto-close on completion
 * - Minimum display time to avoid flashing
 */
class ProgressDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param title Dialog title
     * @param parent Parent widget
     */
    explicit ProgressDialog(const QString& title, QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ProgressDialog() override;

    /**
     * @brief Set progress value
     * @param value Progress (0.0 to 1.0)
     */
    void setProgress(double value);

    /**
     * @brief Set status message
     * @param message Status text to display
     */
    void setStatusMessage(const QString& message);

    /**
     * @brief Enable/disable cancel button
     * @param enabled True to enable cancel
     */
    void setCancelEnabled(bool enabled);

    /**
     * @brief Check if user clicked cancel
     * @return true if cancelled
     */
    bool wasCancelled() const { return m_cancelled; }

    /**
     * @brief Reset dialog state
     *
     * Resets progress to 0 and clears cancelled flag
     */
    void reset();

signals:
    /**
     * @brief Emitted when user clicks cancel
     */
    void cancelRequested();

private slots:
    /**
     * @brief Handle cancel button clicked
     */
    void onCancelClicked();

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();

    // UI components
    QVBoxLayout* m_layout;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QPushButton* m_cancelButton;

    // State
    bool m_cancelled;
};

} // namespace ui
} // namespace koomesh

#else // !KOOMESH_HAS_QT

// Stub when Qt not available
namespace koomesh {
namespace ui {

class ProgressDialog {
public:
    ProgressDialog(const char*, void* = nullptr) {}
    void setProgress(double) {}
    void setStatusMessage(const char*) {}
    void setCancelEnabled(bool) {}
    bool wasCancelled() const { return false; }
    void reset() {}
    int exec() { return 0; }
    void show() {}
    void hide() {}
};

} // namespace ui
} // namespace koomesh

#endif // KOOMESH_HAS_QT
