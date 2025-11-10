/**
 * @file ProgressDialog.cpp
 * @brief Implementation of progress dialog - Phase 76
 *
 * Complete progress dialog for async operations with:
 * - Progress bar (0-100%)
 * - Status message display
 * - Cancel button support
 * - Modal dialog behavior
 */

#include "ui/ProgressDialog.h"

#ifdef KOOMESH_HAS_QT

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <Qt>

namespace koomesh {
namespace ui {

ProgressDialog::ProgressDialog(const QString& title, QWidget* parent)
    : QDialog(parent)
    , m_layout(nullptr)
    , m_statusLabel(nullptr)
    , m_progressBar(nullptr)
    , m_cancelButton(nullptr)
    , m_cancelled(false)
{
    setWindowTitle(title);
    setModal(true);  // Block interaction with parent window
    setMinimumWidth(400);

    setupUI();
}

ProgressDialog::~ProgressDialog() = default;

void ProgressDialog::setupUI() {
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(10);
    m_layout->setContentsMargins(20, 20, 20, 20);

    // Status label
    m_statusLabel = new QLabel("Processing...", this);
    m_statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_layout->addWidget(m_statusLabel);

    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_layout->addWidget(m_progressBar);

    // Add spacing
    m_layout->addSpacing(10);

    // Cancel button (centered)
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->setFixedWidth(100);
    connect(m_cancelButton, &QPushButton::clicked,
            this, &ProgressDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addStretch();
    m_layout->addLayout(buttonLayout);

    setLayout(m_layout);
}

void ProgressDialog::setProgress(double value) {
    if (value < 0.0) value = 0.0;
    if (value > 1.0) value = 1.0;

    int percentage = static_cast<int>(value * 100.0);
    m_progressBar->setValue(percentage);

    // Auto-close when complete
    if (value >= 1.0 && !m_cancelled) {
        accept();  // Close dialog with accept status
    }
}

void ProgressDialog::setStatusMessage(const QString& message) {
    m_statusLabel->setText(message);
}

void ProgressDialog::setCancelEnabled(bool enabled) {
    m_cancelButton->setEnabled(enabled);
}

void ProgressDialog::reset() {
    m_cancelled = false;
    m_progressBar->setValue(0);
    m_statusLabel->setText("Processing...");
    m_cancelButton->setEnabled(true);
}

void ProgressDialog::onCancelClicked() {
    m_cancelled = true;
    m_cancelButton->setEnabled(false);
    m_statusLabel->setText("Cancelling...");
    emit cancelRequested();

    // Close dialog after brief delay
    reject();  // Close dialog with reject status
}

} // namespace ui
} // namespace koomesh

#endif // KOOMESH_HAS_QT
