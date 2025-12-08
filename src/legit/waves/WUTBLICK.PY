import sys
import os
import argparse
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QSplitter, QPlainTextEdit, 
    QVBoxLayout, QWidget, QToolBar, QAction, QFileDialog, 
    QListWidget, QListWidgetItem, QLabel, QMessageBox, QFrame,
    QDialog, QGridLayout, QLineEdit, QSpinBox, QCheckBox, 
    QPushButton, QFontDialog, QTabWidget, QGroupBox, QComboBox,
    QHBoxLayout # <-- FIX: This was missing in your original imports!
)
from PyQt5.QtWebEngineWidgets import QWebEngineView, QWebEnginePage
from PyQt5.QtCore import Qt, QUrl
from PyQt5.QtGui import QFont, QColor, QIcon

# --- Configuration & Styles ---
DARK_THEME = """
QMainWindow { background-color: #0f172a; }
QToolBar { background-color: #1e293b; border-bottom: 1px solid #334155; spacing: 10px; padding: 5px; }
QToolButton { color: #e2e8f0; background: transparent; padding: 5px; border-radius: 4px; }
QToolButton:hover { background-color: #334155; }
QPlainTextEdit { background-color: #0d1117; color: #e2e8f0; border: none; font-family: Consolas, 'Courier New', monospace; font-size: 14px; padding: 10px; }
QListWidget { background-color: #0d1117; color: #e2e8f0; border-top: 1px solid #334155; }
QListWidget::item { padding: 3px 5px; }
QSplitter::handle { background-color: #334155; }
QLabel { color: #94a3b8; font-weight: bold; padding: 5px; }
/* Styles for the Settings Dialog */
QDialog { background-color: #0d1117; color: #e2e8f0; }
QGroupBox { 
    border: 1px solid #334155; 
    margin-top: 10px; 
    padding: 10px;
    border-radius: 4px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 0 5px;
    color: #38bdf8;
    background-color: #0d1117;
}
QLineEdit, QSpinBox, QComboBox {
    background-color: #1e293b;
    color: #e2e8f0;
    border: 1px solid #334155;
    padding: 4px;
    border-radius: 3px;
}
QPushButton {
    background-color: #38bdf8;
    color: #0f172a;
    border: none;
    padding: 8px 15px;
    border-radius: 4px;
}
QPushButton:hover {
    background-color: #60a5fa;
}
QCheckBox { color: #e2e8f0; }
"""

HELP_TEXT = """<!DOCTYPE html>
<html>
<head>
<style>
  body { 
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; 
    background: #0f172a; 
    color: #94a3b8; 
    display: flex; 
    flex-direction: column; 
    align-items: center; 
    justify-content: center; 
    height: 100vh; 
    margin: 0; 
  }
  .container { text-align: center; border: 1px solid #334155; padding: 2rem; border-radius: 8px; background: #1e293b; }
  h1 { color: #e2e8f0; margin-bottom: 1rem; }
  code { background: #0f172a; padding: 0.2rem 0.5rem; border-radius: 4px; color: #38bdf8; font-family: monospace; }
</style>
</head>
<body>
  <div class="container">
    <h1>Ready to Render</h1>
    <p>No input file was provided.</p>
    <p>Run: <code>python local_browser.py filename.html</code></p>
    <p>Or use the <b>Open File</b> button above.</p>
  </div>
</body>
</html>
"""

class ConsolePage(QWebEnginePage):
    """Custom Page to intercept JavaScript Console Logs"""
    def __init__(self, console_widget, parent=None):
        super().__init__(parent)
        self.console_widget = console_widget

    def javaScriptConsoleMessage(self, level, message, lineNumber, sourceID):
        # Format logs based on level
        level_str = "LOG"
        color = "#e2e8f0"  # Default white/gray
        
        if level == QWebEnginePage.InfoMessageLevel:
            level_str = "INFO"
            color = "#60a5fa" # Blue
        elif level == QWebEnginePage.WarningMessageLevel:
            level_str = "WARN"
            color = "#facc15" # Yellow
        elif level == QWebEnginePage.ErrorMessageLevel:
            level_str = "ERROR"
            color = "#f87171" # Red

        # Create list item
        item = QListWidgetItem(f"[{level_str}] {message} (Line: {lineNumber})")
        item.setForeground(QColor(color))
        self.console_widget.addItem(item)
        self.console_widget.scrollToBottom()

class SettingsDialog(QDialog):
    """Configuration dialog for the application settings."""
    def __init__(self, current_settings, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Settings")
        self.setStyleSheet(DARK_THEME)
        self.current_settings = current_settings
        self.new_settings = current_settings.copy()
        
        main_layout = QVBoxLayout(self)
        
        self.tab_widget = QTabWidget()
        self.tab_widget.setStyleSheet("QTabWidget::pane { border: 0; }")
        main_layout.addWidget(self.tab_widget)
        
        self.add_gui_tab()
        self.add_editor_tab()
        self.add_render_tab()
        
        # --- Buttons ---
        button_box = QWidget()
        # FIX: The missing import was here (QHBoxLayout was undefined)
        hbox = QHBoxLayout(button_box) 
        
        save_button = QPushButton("Save & Apply")
        save_button.clicked.connect(self.accept)
        
        cancel_button = QPushButton("Cancel")
        cancel_button.clicked.connect(self.reject)
        
        hbox.addWidget(cancel_button)
        hbox.addWidget(save_button)
        main_layout.addWidget(button_box)

    def add_gui_tab(self):
        tab = QWidget()
        layout = QGridLayout(tab)
        
        # Console Height
        console_group = QGroupBox("Console")
        console_layout = QGridLayout(console_group)
        
        console_layout.addWidget(QLabel("Max Console Height (px):"), 0, 0)
        self.console_height_spin = QSpinBox()
        self.console_height_spin.setRange(50, 500)
        self.console_height_spin.setValue(self.new_settings["console_height"])
        self.console_height_spin.valueChanged.connect(lambda v: self.new_settings.update({"console_height": v}))
        console_layout.addWidget(self.console_height_spin, 0, 1)

        layout.addWidget(console_group, 0, 0, 1, 2)
        
        # Window
        window_group = QGroupBox("Window")
        window_layout = QGridLayout(window_group)
        
        window_layout.addWidget(QLabel("Remember Window Size:"), 0, 0)
        self.remember_size_check = QCheckBox()
        self.remember_size_check.setChecked(self.new_settings["remember_window_size"])
        self.remember_size_check.stateChanged.connect(lambda v: self.new_settings.update({"remember_window_size": v == Qt.Checked}))
        window_layout.addWidget(self.remember_size_check, 0, 1)

        window_layout.addWidget(QLabel("Initial Splitter Ratio (Editor:Preview):"), 1, 0)
        self.splitter_ratio_combo = QComboBox()
        self.splitter_ratio_combo.addItems(["1:1", "2:1", "1:2"])
        ratios = {"1:1": [500, 500], "2:1": [667, 333], "1:2": [333, 667]}
        current_ratio = self.new_settings["initial_splitter_ratio"]
        # Find the key (string ratio) that matches the current value ([px, px])
        current_ratio_str = next((k for k, v in ratios.items() if v == current_ratio), "1:1")
        self.splitter_ratio_combo.setCurrentText(current_ratio_str)
        self.splitter_ratio_combo.currentTextChanged.connect(lambda t: self.new_settings.update({"initial_splitter_ratio": ratios[t]}))
        window_layout.addWidget(self.splitter_ratio_combo, 1, 1)
        
        layout.addWidget(window_group, 1, 0, 1, 2)
        
        layout.setRowStretch(2, 1)
        self.tab_widget.addTab(tab, "GUI")

    def add_editor_tab(self):
        tab = QWidget()
        layout = QGridLayout(tab)

        font_group = QGroupBox("Code Editor")
        font_layout = QGridLayout(font_group)
        
        font_layout.addWidget(QLabel("Font:"), 0, 0)
        self.font_line_edit = QLineEdit(self.new_settings["editor_font_family"])
        self.font_line_edit.setReadOnly(True)
        font_layout.addWidget(self.font_line_edit, 0, 1)

        font_button = QPushButton("Change Font")
        font_button.clicked.connect(self.select_font)
        font_layout.addWidget(font_button, 0, 2)

        font_layout.addWidget(QLabel("Size:"), 1, 0)
        self.font_size_spin = QSpinBox()
        self.font_size_spin.setRange(8, 36)
        self.font_size_spin.setValue(self.new_settings["editor_font_size"])
        self.font_size_spin.valueChanged.connect(lambda v: self.new_settings.update({"editor_font_size": v}))
        font_layout.addWidget(self.font_size_spin, 1, 1)

        font_group.setLayout(font_layout)
        layout.addWidget(font_group, 0, 0, 1, 2)
        
        layout.setRowStretch(1, 1)
        self.tab_widget.addTab(tab, "Editor")

    def add_render_tab(self):
        tab = QWidget()
        layout = QGridLayout(tab)

        cache_group = QGroupBox("Web Engine")
        cache_layout = QGridLayout(cache_group)

        cache_layout.addWidget(QLabel("Enable HTTP Cache:"), 0, 0)
        self.cache_check = QCheckBox()
        self.cache_check.setChecked(self.new_settings["enable_http_cache"])
        self.cache_check.stateChanged.connect(lambda v: self.new_settings.update({"enable_http_cache": v == Qt.Checked}))
        cache_layout.addWidget(self.cache_check, 0, 1)
        
        cache_layout.addWidget(QLabel("Javascript Enabled:"), 1, 0)
        self.js_check = QCheckBox()
        self.js_check.setChecked(self.new_settings["enable_javascript"])
        self.js_check.stateChanged.connect(lambda v: self.new_settings.update({"enable_javascript": v == Qt.Checked}))
        cache_layout.addWidget(self.js_check, 1, 1)

        cache_group.setLayout(cache_layout)
        layout.addWidget(cache_group, 0, 0, 1, 2)
        
        layout.setRowStretch(1, 1)
        self.tab_widget.addTab(tab, "Rendering")

    def select_font(self):
        font, ok = QFontDialog.getFont(QFont(self.new_settings["editor_font_family"], self.new_settings["editor_font_size"]), self)
        if ok:
            self.new_settings["editor_font_family"] = font.family()
            self.new_settings["editor_font_size"] = font.pointSize()
            self.font_line_edit.setText(font.family())
            self.font_size_spin.setValue(font.pointSize())


class NativeBrowser(QMainWindow):
    def __init__(self, initial_file=None):
        super().__init__()
        
        # --- Default Settings ---
        self.settings = {
            "console_height": 150,
            "remember_window_size": True,
            "initial_splitter_ratio": [500, 500], # 1:1
            "editor_font_family": "Consolas, 'Courier New', monospace",
            "editor_font_size": 14,
            "enable_http_cache": True,
            "enable_javascript": True
        }

        self.setWindowTitle("Native Render Engine")
        self.resize(1200, 800)
        self.setStyleSheet(DARK_THEME)

        # --- Main Layout ---
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QVBoxLayout(main_widget)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # --- Toolbar ---
        toolbar = QToolBar()
        layout.addWidget(toolbar)

        # Actions
        open_action = QAction(QIcon.fromTheme("document-open"), "Open File", self)
        open_action.triggered.connect(self.open_file)
        toolbar.addAction(open_action)

        save_action = QAction(QIcon.fromTheme("document-save"), "Save", self)
        save_action.triggered.connect(self.save_file)
        toolbar.addAction(save_action)

        toolbar.addSeparator()

        render_action = QAction("▶ Render", self)
        render_action.triggered.connect(self.render_content)
        toolbar.addAction(render_action)
        
        toolbar.addSeparator()
        
        # New: Toggle Editor View
        self.toggle_editor_action = QAction(QIcon.fromTheme("edit-undo"), "Toggle Editor", self)
        self.toggle_editor_action.triggered.connect(self.toggle_editor)
        self.toggle_editor_action.setToolTip("Show/Hide Source Code Editor")
        toolbar.addAction(self.toggle_editor_action)

        # New: Toggle Console/Output View
        self.toggle_console_action = QAction(QIcon.fromTheme("terminal"), "Toggle Console", self)
        self.toggle_console_action.triggered.connect(self.toggle_console)
        self.toggle_console_action.setToolTip("Show/Hide JavaScript Console")
        toolbar.addAction(self.toggle_console_action)
        
        # New: Settings Button
        settings_action = QAction(QIcon.fromTheme("preferences-system"), "Settings", self)
        settings_action.triggered.connect(self.show_settings)
        toolbar.addAction(settings_action)
        
        # --- Splitter (Editor | Preview) ---
        self.splitter = QSplitter(Qt.Horizontal)
        layout.addWidget(self.splitter, 1) # Stretch factor 1
        
        # 1. Editor Panel
        self.editor_container = QWidget()
        editor_layout = QVBoxLayout(self.editor_container)
        editor_layout.setContentsMargins(0,0,0,0)
        editor_layout.setSpacing(0)
        
        editor_label = QLabel(" SOURCE CODE")
        editor_label.setStyleSheet("background: #1e293b; border-bottom: 1px solid #334155;")
        editor_layout.addWidget(editor_label)

        self.editor = QPlainTextEdit()
        
        # Load initial file or Help text
        if initial_file and os.path.exists(initial_file):
            try:
                with open(initial_file, 'r', encoding='utf-8') as f:
                    self.editor.setPlainText(f.read())
            except Exception as e:
                self.editor.setPlainText(f"\n" + HELP_TEXT)
        else:
            self.editor.setPlainText(HELP_TEXT)

        editor_layout.addWidget(self.editor)
        
        self.splitter.addWidget(self.editor_container)

        # 2. Preview Panel
        self.preview_container = QWidget()
        preview_layout = QVBoxLayout(self.preview_container)
        preview_layout.setContentsMargins(0,0,0,0)
        preview_layout.setSpacing(0)

        preview_label = QLabel(" PREVIEW & CONSOLE")
        preview_label.setStyleSheet("background: #1e293b; border-bottom: 1px solid #334155;")
        preview_layout.addWidget(preview_label)

        # Web View
        self.web_view = QWebEngineView()
        # Console Widget (List)
        self.console_list = QListWidget()
        
        # Setup Custom Page for Logging
        self.web_page = ConsolePage(self.console_list, self.web_view)
        self.web_view.setPage(self.web_page)
        
        # Vertical Splitter for Web | Console
        self.preview_splitter = QSplitter(Qt.Vertical)
        self.preview_splitter.addWidget(self.web_view)
        self.preview_splitter.addWidget(self.console_list)
        
        preview_layout.addWidget(self.preview_splitter)
        self.splitter.addWidget(self.preview_container)

        # Apply settings (after widgets are created)
        self.apply_settings()

        # Initial Render
        self.render_content()

    def apply_settings(self):
        """Applies all current settings to the GUI components."""
        
        # GUI Settings
        self.console_list.setMaximumHeight(self.settings["console_height"])
        
        # Set initial splitter sizes (only if editor is visible)
        if self.editor_container.isVisible():
            self.splitter.setSizes(self.settings["initial_splitter_ratio"])
        
        # Editor Settings
        font = QFont(self.settings["editor_font_family"])
        font.setPointSize(self.settings["editor_font_size"])
        self.editor.setFont(font)
        
        # Note on Render Settings: QWebEngineSettings are complex to toggle dynamically.
        # The settings here are currently for user preference display.
        
    def render_content(self):
        """Takes code from editor and pushes it to the WebEngine."""
        self.console_list.clear() # Clear console on run
        html_content = self.editor.toPlainText()
        # QWebEngineView needs a base URL to resolve relative paths if loaded from file
        self.web_view.setHtml(html_content, QUrl("local://"))

    def open_file(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "Open HTML File", "", "HTML Files (*.html *.htm);;Text Files (*.txt)")
        if file_path:
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    self.editor.setPlainText(f.read())
                # Ensure editor is visible after opening a new file
                if not self.editor_container.isVisible():
                    self.toggle_editor()
                self.render_content()
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Could not open file: {e}")

    def save_file(self):
        file_path, _ = QFileDialog.getSaveFileName(self, "Save HTML File", "", "HTML Files (*.html)")
        if file_path:
            try:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(self.editor.toPlainText())
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Could not save file: {e}")
                
    def toggle_editor(self):
        """Toggles the visibility of the source code editor panel."""
        is_visible = self.editor_container.isVisible()
        self.editor_container.setVisible(not is_visible)
        
        # Adjust splitter if the editor is toggled off/on
        if not is_visible:
            # Editor is now visible, restore initial ratio (or the last saved ratio)
            self.splitter.setSizes(self.settings["initial_splitter_ratio"])
        else:
            # Editor is now hidden, give all space to preview
            self.splitter.setSizes([0, 1000])

    def toggle_console(self):
        """Toggles the visibility of the JavaScript console/output list."""
        # Get the current sizes of the vertical splitter (Web View | Console List)
        current_sizes = self.preview_splitter.sizes()
        
        if self.console_list.isVisible():
            # Console is visible -> Hide it
            # The preview splitter has two widgets: web_view and console_list
            # Set the console's height to 0
            self.preview_splitter.setSizes([1000, 0])
            self.console_list.hide()
        else:
            # Console is hidden -> Show it
            # Restore the web view to an appropriate size based on the Max Height setting
            web_height = 1000 - self.settings["console_height"]
            self.preview_splitter.setSizes([web_height, self.settings["console_height"]])
            self.console_list.show()


    def show_settings(self):
        """Opens the settings dialog."""
        dialog = SettingsDialog(self.settings, self)
        if dialog.exec_() == QDialog.Accepted:
            self.settings = dialog.new_settings
            self.apply_settings()
            
            # Since console height can change, apply the toggle logic to ensure correct sizing
            if self.console_list.isVisible():
                self.toggle_console() # Hide then show to reset size
                self.toggle_console()
            

if __name__ == "__main__":
    app = QApplication(sys.argv)
    
    # Optional: Enable high DPI scaling
    os.environ["QT_AUTO_SCREEN_SCALE_FACTOR"] = "1"
    
    # Parse Command Line Arguments
    parser = argparse.ArgumentParser(description="Native HTML Renderer & Editor")
    parser.add_argument("file", nargs="?", help="Path to the HTML file to open", default=None)
    
    # We use parse_known_args because QApplication might use some args (like -style)
    args, unknown = parser.parse_known_args()
    
    # Apply global stylesheet
    app.setStyleSheet(DARK_THEME)

    window = NativeBrowser(initial_file=args.file)
    window.show()
    sys.exit(app.exec_())
