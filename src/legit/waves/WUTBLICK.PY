import sys
import os
import argparse
import traceback

# --- ARGUMENT PARSING ---
parser = argparse.ArgumentParser(description="Wutblick Engine")
parser.add_argument("file", nargs="?", default=None, help="HTML file to load")
parser.add_argument("--clean", action="store_true", help="Start in distraction-free mode")
parser.add_argument("--geometry", type=str, help="Window geometry", default="1280x800")
parser.add_argument("--fullscreen", action="store_true", help="Start in fullscreen mode")
parser.add_argument("--maximized", action="store_true", help="Start maximized")
parser.add_argument("--debug-port", type=str, help="Enable remote debugging", default=None)
parser.add_argument("--zoom", type=float, help="Initial zoom factor", default=1.0)
parser.add_argument("--no-patch", action="store_true", help="Disable automatic shader patching")
args, unknown = parser.parse_known_args()

try:
    from PyQt5.QtWidgets import (
        QApplication, QMainWindow, QSplitter, QPlainTextEdit, 
        QVBoxLayout, QWidget, QToolBar, QAction, QFileDialog, 
        QListWidget, QListWidgetItem, QLabel, QMessageBox, QDialog, 
        QGridLayout, QLineEdit, QSpinBox, QCheckBox, QPushButton, 
        QFontDialog, QTabWidget, QGroupBox, QComboBox, QHBoxLayout,
        QShortcut, QStatusBar, QTextEdit, QMenu, QInputDialog, QFrame,
        QMenuBar, QDoubleSpinBox
    )
    from PyQt5.QtWebEngineWidgets import QWebEngineView, QWebEnginePage, QWebEngineSettings
    from PyQt5.QtCore import Qt, QUrl, QTimer, QSize, QRect, QRegularExpression, QPoint
    from PyQt5.QtGui import (
        QFont, QColor, QIcon, QKeySequence, QDesktopServices, 
        QSyntaxHighlighter, QTextCharFormat, QPainter, QTextFormat, QPolygon, QTextCursor, QFontMetricsF
    )
except ImportError as e:
    print("CRITICAL ERROR: Missing PyQt5 modules.")
    print("Please run: pip install PyQt5 PyQt5-WebEngine")
    input("Press Enter to exit...")
    sys.exit(1)

DARK_THEME = """
QMainWindow { background-color: #0f172a; }
QMenuBar { background-color: #1e293b; color: #e2e8f0; border-bottom: 1px solid #334155; }
QMenuBar::item { spacing: 3px; padding: 6px 10px; background: transparent; border-radius: 4px; }
QMenuBar::item:selected { background-color: #334155; }
QMenuBar::item:pressed { background-color: #475569; }
QMenu { background-color: #1e293b; color: #e2e8f0; border: 1px solid #334155; padding: 5px; }
QMenu::item { padding: 5px 25px 5px 20px; border-radius: 3px; }
QMenu::item:selected { background-color: #38bdf8; color: #0f172a; }
QMenu::separator { height: 1px; background: #334155; margin: 5px 0; }
QPlainTextEdit, QListWidget { background-color: #0d1117; color: #e2e8f0; border: none; font-family: 'Consolas', 'Courier New', monospace; font-size: 14px; }
QListWidget { border-top: 1px solid #334155; }
QListWidget::item { padding: 3px 5px; }
QSplitter::handle { background-color: #334155; }
QLabel { color: #94a3b8; font-weight: bold; padding: 5px; }
QDialog { background-color: #0d1117; color: #e2e8f0; }
QGroupBox { border: 1px solid #334155; margin-top: 10px; padding: 10px; border-radius: 4px; }
QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 5px; color: #38bdf8; background-color: #0d1117; }
QLineEdit, QSpinBox, QComboBox, QDoubleSpinBox { background-color: #1e293b; color: #e2e8f0; border: 1px solid #334155; padding: 4px; border-radius: 3px; }
QPushButton { background-color: #38bdf8; color: #0f172a; border: none; padding: 8px 15px; border-radius: 4px; }
QPushButton:hover { background-color: #60a5fa; }
QCheckBox { color: #e2e8f0; }
QStatusBar { background: #1e293b; color: #94a3b8; border-top: 1px solid #334155; }
QStatusBar::item { border: none; }
"""

GUI_EDITOR_JS = """
(function() {
    if (document.getElementById('wutblick-gui-root')) {
        const root = document.getElementById('wutblick-gui-root');
        root.style.display = root.style.display === 'none' ? 'block' : 'none';
        return;
    }
    const css = `
        #wutblick-gui-root { position: fixed; right: 0; top: 0; width: 280px; height: 100vh; background: #1e293b; border-left: 1px solid #475569; z-index: 2147483647; color: #e2e8f0; font-family: sans-serif; overflow-y: auto; box-shadow: -5px 0 15px rgba(0,0,0,0.5); display: flex; flex-direction: column; }
        #wutblick-header { padding: 10px; background: #0f172a; border-bottom: 1px solid #475569; display: flex; justify-content: space-between; align-items: center; }
        #wutblick-header h3 { margin: 0; font-size: 14px; color: #38bdf8; text-transform: uppercase; letter-spacing: 1px; }
        #wutblick-close { background: none; border: none; color: #ef4444; cursor: pointer; font-weight: bold; }
        #wutblick-controls { padding: 15px; flex: 1; overflow-y: auto; }
        .wb-group { margin-bottom: 15px; border: 1px solid #334155; padding: 10px; border-radius: 4px; background: #0d1117; }
        .wb-group-title { font-size: 11px; font-weight: bold; color: #94a3b8; margin-bottom: 8px; text-transform: uppercase; }
        .wb-row { display: flex; align-items: center; margin-bottom: 6px; justify-content: space-between; }
        .wb-row label { font-size: 12px; color: #ccc; }
        .wb-input { background: #1e293b; border: 1px solid #475569; color: #fff; padding: 4px; border-radius: 3px; width: 60%; font-size: 12px; }
        .wb-color { width: 40px; height: 25px; border: none; padding: 0; cursor: pointer; }
        #wb-inspect-btn { width: 100%; padding: 10px; background: #38bdf8; border: none; color: #0f172a; font-weight: bold; cursor: pointer; margin-bottom: 15px; border-radius: 4px; }
        #wb-inspect-btn.active { background: #ef4444; color: white; }
        .wb-highlight { outline: 2px solid #38bdf8 !important; cursor: crosshair !important; }
    `;
    const style = document.createElement('style'); style.innerHTML = css; document.head.appendChild(style);
    const root = document.createElement('div'); root.id = 'wutblick-gui-root';
    root.innerHTML = `<div id="wutblick-header"><h3>UI Editor</h3><button id="wutblick-close">X</button></div><div id="wutblick-controls"><button id="wb-inspect-btn">Select Element</button><div id="wb-properties" style="display:none"><div class="wb-group"><div class="wb-group-title">Geometry</div><div class="wb-row"><label>Width</label><input type="text" class="wb-input" data-prop="width"></div><div class="wb-row"><label>Height</label><input type="text" class="wb-input" data-prop="height"></div><div class="wb-row"><label>Padding</label><input type="text" class="wb-input" data-prop="padding"></div><div class="wb-row"><label>Margin</label><input type="text" class="wb-input" data-prop="margin"></div></div><div class="wb-group"><div class="wb-group-title">Colors</div><div class="wb-row"><label>Text</label><input type="color" class="wb-color" data-prop="color"></div><div class="wb-row"><label>Background</label><input type="color" class="wb-color" data-prop="backgroundColor"></div><div class="wb-row"><label>Opacity</label><input type="range" min="0" max="1" step="0.1" class="wb-input" data-prop="opacity"></div></div><div class="wb-group"><div class="wb-group-title">Typography</div><div class="wb-row"><label>Size</label><input type="text" class="wb-input" data-prop="fontSize"></div><div class="wb-row"><label>Weight</label><input type="number" step="100" min="100" max="900" class="wb-input" data-prop="fontWeight"></div><div class="wb-row"><label>Align</label><select class="wb-input" data-prop="textAlign"><option value="left">Left</option><option value="center">Center</option><option value="right">Right</option></select></div></div></div></div>`;
    document.body.appendChild(root);
    let activeEl = null, inspectMode = false;
    const btn = document.getElementById('wb-inspect-btn'), propsDiv = document.getElementById('wb-properties'), inputs = root.querySelectorAll('input, select');
    document.getElementById('wutblick-close').onclick = () => root.style.display = 'none';
    btn.onclick = () => { inspectMode = !inspectMode; btn.textContent = inspectMode ? "Click an Element..." : "Select Element"; btn.classList.toggle('active'); };
    document.addEventListener('mouseover', e => { if (!inspectMode || root.contains(e.target)) return; e.target.classList.add('wb-highlight'); e.stopPropagation(); });
    document.addEventListener('mouseout', e => { if (!inspectMode) return; e.target.classList.remove('wb-highlight'); });
    document.addEventListener('click', e => { if (!inspectMode || root.contains(e.target)) return; e.preventDefault(); e.stopPropagation(); e.target.classList.remove('wb-highlight'); activeEl = e.target; inspectMode = false; btn.textContent = "Select Element"; btn.classList.remove('active'); propsDiv.style.display = 'block'; populateFields(); }, true);
    function rgbToHex(rgb) { if (!rgb || rgb === 'rgba(0, 0, 0, 0)') return '#000000'; if (rgb.startsWith('#')) return rgb; const [r, g, b] = rgb.match(/\d+/g); return "#" + ((1 << 24) + (+r << 16) + (+g << 8) + +b).toString(16).slice(1); }
    function populateFields() { if (!activeEl) return; const style = window.getComputedStyle(activeEl); inputs.forEach(input => { const prop = input.dataset.prop; let val = style[prop]; if (input.type === 'color') val = rgbToHex(val); input.value = val; }); }
    inputs.forEach(input => { input.addEventListener('input', () => { if (activeEl) activeEl.style[input.dataset.prop] = input.value; }); });
})();
"""

DEBUG_HUD_JS = """
(function() {
    if (document.getElementById('wb-debug-hud')) {
        const hud = document.getElementById('wb-debug-hud');
        hud.style.display = hud.style.display === 'none' ? 'flex' : 'none';
        return;
    }
    const css = `
        #wb-debug-hud { position: fixed; top: 10px; left: 10px; width: 300px; background: rgba(15, 23, 42, 0.95); border: 1px solid #334155; color: #fff; font-family: monospace; font-size: 11px; z-index: 999999; border-radius: 6px; box-shadow: 0 4px 12px rgba(0,0,0,0.5); display: flex; flex-direction: column; overflow: hidden; resize: both; min-width: 250px; min-height: 200px; }
        .wb-hud-header { padding: 6px 8px; background: #0f172a; border-bottom: 1px solid #334155; display: flex; justify-content: space-between; align-items: center; cursor: grab; user-select: none; }
        .wb-hud-close { background: none; border: none; color: #ef4444; font-weight: bold; cursor: pointer; font-size: 14px; }
        .wb-hud-content { flex: 1; overflow-y: auto; display: flex; flex-direction: column; }
        .wb-hud-row { padding: 4px 8px; border-bottom: 1px solid #334155; display: flex; justify-content: space-between; }
        .wb-hud-canvas-container { height: 80px; background: #000; position: relative; border-bottom: 1px solid #334155; flex-shrink: 0; }
        #wb-perf-canvas { width: 100%; height: 100%; display: block; }
        .wb-hud-info { pointer-events: auto; padding: 4px 8px; overflow-y: auto; background: rgba(0,0,0,0.2); flex: 1; }
        .wb-error-toast { position: fixed; top: 20px; right: 20px; background: #ef4444; color: white; padding: 10px 15px; border-radius: 4px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); font-family: sans-serif; font-size: 13px; z-index: 999999; pointer-events: auto; opacity: 0; transition: opacity 0.3s; }
    `;
    const style = document.createElement('style'); style.innerHTML = css; document.head.appendChild(style);
    const hud = document.createElement('div'); hud.id = 'wb-debug-hud';
    hud.innerHTML = `<div class="wb-hud-header" id="wb-hud-drag"><span style="color:#38bdf8; font-weight:bold;">Debug HUD</span><button class="wb-hud-close" id="wb-hud-close-btn">×</button></div><div class="wb-hud-content"><div class="wb-hud-row"><span style="color:#4ade80">FPS: <b id="wb-fps">0</b></span> <span style="color:#38bdf8">MEM: <b id="wb-mem">0</b> MB</span></div><div class="wb-hud-canvas-container"><canvas id="wb-perf-canvas"></canvas></div><div class="wb-hud-row" style="background:#1e293b"><b>DOM Inspector</b> (Hover)</div><div id="wb-dom-info" class="wb-hud-info" style="color:#94a3b8; max-height:60px; min-height:40px;">Hover over elements...</div><div class="wb-hud-row" style="background:#1e293b"><b>Error Log</b></div><div id="wb-error-log" class="wb-hud-info" style="color:#f87171; min-height: 60px;"></div></div>`;
    document.body.appendChild(hud);
    const header = document.getElementById('wb-hud-drag'), closeBtn = document.getElementById('wb-hud-close-btn');
    let isDragging = false, startX, startY, initLeft, initTop;
    closeBtn.onclick = () => hud.style.display = 'none';
    header.addEventListener('mousedown', (e) => { isDragging = true; startX = e.clientX; startY = e.clientY; const rect = hud.getBoundingClientRect(); initLeft = rect.left; initTop = rect.top; header.style.cursor = 'grabbing'; });
    document.addEventListener('mousemove', (e) => { if (!isDragging) return; const dx = e.clientX - startX, dy = e.clientY - startY; hud.style.left = (initLeft + dx) + 'px'; hud.style.top = (initTop + dy) + 'px'; hud.style.right = 'auto'; hud.style.bottom = 'auto'; });
    document.addEventListener('mouseup', () => { isDragging = false; header.style.cursor = 'grab'; });
    const canvas = document.getElementById('wb-perf-canvas'), ctx = canvas.getContext('2d');
    const resizeObserver = new ResizeObserver(() => { const dpr = window.devicePixelRatio || 1; const rect = canvas.getBoundingClientRect(); canvas.width = rect.width * dpr; canvas.height = rect.height * dpr; ctx.scale(dpr, dpr); });
    resizeObserver.observe(canvas);
    let frames = 0, lastTime = performance.now(), fpsHistory = new Array(60).fill(0);
    function updateGraph() {
        if (hud.style.display === 'none') { requestAnimationFrame(updateGraph); return; }
        const now = performance.now(); frames++;
        if (now >= lastTime + 1000) {
            const fps = frames; frames = 0; lastTime = now;
            document.getElementById('wb-fps').innerText = fps; fpsHistory.push(fps); fpsHistory.shift();
            document.getElementById('wb-mem').innerText = performance.memory ? (performance.memory.usedJSHeapSize / 1048576).toFixed(1) : 'N/A';
            const rect = canvas.getBoundingClientRect(); ctx.fillStyle = '#000'; ctx.fillRect(0,0, rect.width, rect.height); ctx.strokeStyle = '#4ade80'; ctx.lineWidth = 1; ctx.beginPath();
            const step = rect.width / (fpsHistory.length - 1);
            fpsHistory.forEach((val, i) => { const h = (val / 70) * rect.height; const y = rect.height - h; if(i===0) ctx.moveTo(0, y); else ctx.lineTo(i * step, y); }); ctx.stroke();
        }
        requestAnimationFrame(updateGraph);
    }
    requestAnimationFrame(updateGraph);
    document.addEventListener('mousemove', (e) => { if (hud.style.display === 'none') return; const el = e.target; if (hud.contains(el)) return; let id = el.id ? '#' + el.id : '', cls = el.className && typeof el.className === 'string' ? '.' + el.className.split(' ').join('.') : '', dims = el.getBoundingClientRect(); document.getElementById('wb-dom-info').innerHTML = `<span style="color:#38bdf8">&lt;${el.tagName.toLowerCase()}${id}${cls}&gt;</span><br>${Math.round(dims.width)} x ${Math.round(dims.height)} px`; });
    window.addEventListener('error', (e) => { const div = document.createElement('div'); div.className = 'wb-error-toast'; div.innerText = `JS Error: ${e.message}`; document.body.appendChild(div); setTimeout(() => div.style.opacity = '1', 10); setTimeout(() => { div.style.opacity = '0'; setTimeout(()=>div.remove(), 300); }, 4000); const log = document.getElementById('wb-error-log'), errLine = document.createElement('div'); errLine.innerText = `> ${e.message}`; log.appendChild(errLine); log.scrollTop = log.scrollHeight; });
})();
"""

class HTMLHighlighter(QSyntaxHighlighter):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.rules = []
        fmt_tag = QTextCharFormat()
        fmt_tag.setForeground(QColor("#569cd6"))
        self.rules.append((QRegularExpression(r"</?[a-zA-Z0-9]+"), fmt_tag))
        self.rules.append((QRegularExpression(r">"), fmt_tag))
        fmt_attr = QTextCharFormat()
        fmt_attr.setForeground(QColor("#9cdcfe"))
        self.rules.append((QRegularExpression(r"\b[a-zA-Z-]+(?=\=)"), fmt_attr))
        fmt_string = QTextCharFormat()
        fmt_string.setForeground(QColor("#ce9178"))
        self.rules.append((QRegularExpression(r"\".*\""), fmt_string))
        self.rules.append((QRegularExpression(r"'.*'"), fmt_string))
        fmt_comment = QTextCharFormat()
        fmt_comment.setForeground(QColor("#6a9955"))
        self.rules.append((QRegularExpression(r""), fmt_comment))

    def highlightBlock(self, text):
        for pattern, format in self.rules:
            match_iterator = pattern.globalMatch(text)
            while match_iterator.hasNext():
                match = match_iterator.next()
                self.setFormat(match.capturedStart(), match.capturedLength(), format)

class LineNumberArea(QWidget):
    def __init__(self, editor):
        super().__init__(editor)
        self.editor = editor
        self.setMouseTracking(True)

    def sizeHint(self):
        return QSize(self.editor.line_number_area_width(), 0)

    def paintEvent(self, event):
        self.editor.lineNumberAreaPaintEvent(event)
        
    def mousePressEvent(self, event):
        self.editor.lineNumberAreaMousePressEvent(event)

class CodeEditor(QPlainTextEdit):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.line_number_area = LineNumberArea(self)
        self.blockCountChanged.connect(self.update_line_number_area_width)
        self.updateRequest.connect(self.update_line_number_area)
        self.cursorPositionChanged.connect(self.highlight_current_line)
        self.update_line_number_area_width(0)
        self.highlight_current_line()
        self.setMouseTracking(True)
        self.folded_blocks = set()
        self.tab_width = 4
        self.setTabStopDistance(self.fontMetrics().width(' ') * self.tab_width)

    def line_number_area_width(self):
        digits = 1
        m = max(1, self.blockCount())
        while m >= 10:
            m //= 10
            digits += 1
        return 3 + self.fontMetrics().width('9') * digits + 15

    def update_line_number_area_width(self, _):
        self.setViewportMargins(self.line_number_area_width(), 0, 0, 0)

    def update_line_number_area(self, rect, dy):
        if dy:
            self.line_number_area.scroll(0, dy)
        else:
            self.line_number_area.update(0, rect.y(), self.line_number_area.width(), rect.height())
        if rect.contains(self.viewport().rect()):
            self.update_line_number_area_width(0)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        cr = self.contentsRect()
        self.line_number_area.setGeometry(QRect(cr.left(), cr.top(), self.line_number_area_width(), cr.height()))

    def highlight_current_line(self):
        extra_selections = []
        if not self.isReadOnly():
            selection = QTextEdit.ExtraSelection()
            selection.format.setBackground(QColor("#1e293b"))
            selection.format.setProperty(QTextFormat.FullWidthSelection, True)
            selection.cursor = self.textCursor()
            selection.cursor.clearSelection()
            extra_selections.append(selection)
        self.setExtraSelections(extra_selections)

    def get_indentation(self, text):
        return len(text) - len(text.lstrip())

    def toggle_fold(self, block_number):
        block = self.document().findBlockByNumber(block_number)
        if not block.isValid():
            return
        start_indent = self.get_indentation(block.text())
        next_block = block.next()
        is_folding = block_number not in self.folded_blocks
        
        if is_folding:
            self.folded_blocks.add(block_number)
        else:
            self.folded_blocks.remove(block_number)
            
        while next_block.isValid():
            text = next_block.text()
            if not text.strip():
                next_block.setVisible(False if is_folding else (not is_folding))
                next_block = next_block.next()
                continue
                
            indent = self.get_indentation(text)
            if indent <= start_indent:
                break
                
            if not is_folding:
                if next_block.blockNumber() in self.folded_blocks:
                    next_block.setVisible(True)
                    sub_indent = self.get_indentation(next_block.text())
                    next_block = next_block.next()
                    while next_block.isValid():
                        if not next_block.text().strip():
                            next_block = next_block.next()
                            continue
                        if self.get_indentation(next_block.text()) <= sub_indent:
                            break
                        next_block = next_block.next()
                    continue
            
            next_block.setVisible(not is_folding)
            next_block = next_block.next()
            
        self.line_number_area.update()
        self.viewport().update()
        self.update_line_number_area_width(0)

    def fold_all(self):
        block = self.document().begin()
        while block.isValid():
            if self.is_foldable(block):
                if block.blockNumber() not in self.folded_blocks:
                    self.toggle_fold(block.blockNumber())
            block = block.next()

    def unfold_all(self):
        self.folded_blocks.clear()
        block = self.document().begin()
        while block.isValid():
            block.setVisible(True)
            block = block.next()
        self.line_number_area.update()
        self.viewport().update()
        self.update_line_number_area_width(0)

    def contextMenuEvent(self, event):
        menu = self.createStandardContextMenu()
        menu.addSeparator()
        menu.addAction("Collapse All", self.fold_all)
        menu.addAction("Expand All", self.unfold_all)
        menu.exec_(event.globalPos())

    def lineNumberAreaMousePressEvent(self, event):
        y = event.y()
        block = self.firstVisibleBlock()
        block_number = block.blockNumber()
        top = self.blockBoundingGeometry(block).translated(self.contentOffset()).top()
        bottom = top + self.blockBoundingRect(block).height()
        
        while block.isValid():
            if top <= y <= bottom:
                if self.is_foldable(block):
                    self.toggle_fold(block_number)
                break
            
            block = block.next()
            top = bottom
            bottom = top + self.blockBoundingRect(block).height()
            block_number += 1

    def is_foldable(self, block):
        start_indent = self.get_indentation(block.text())
        next_block = block.next()
        while next_block.isValid() and not next_block.text().strip():
            next_block = next_block.next()
            
        if next_block.isValid():
            return self.get_indentation(next_block.text()) > start_indent
        return False

    def lineNumberAreaPaintEvent(self, event):
        painter = QPainter(self.line_number_area)
        painter.fillRect(event.rect(), QColor("#0d1117"))
        block = self.firstVisibleBlock()
        block_number = block.blockNumber()
        top = self.blockBoundingGeometry(block).translated(self.contentOffset()).top()
        bottom = top + self.blockBoundingRect(block).height()
        height = self.fontMetrics().height()
        
        while block.isValid() and top <= event.rect().bottom():
            if block.isVisible() and bottom >= event.rect().top():
                number = str(block_number + 1)
                painter.setPen(QColor("#555"))
                painter.drawText(0, int(top), self.line_number_area.width() - 15, height, Qt.AlignRight, number)
                
                if self.is_foldable(block):
                    is_folded = block_number in self.folded_blocks
                    marker_x = self.line_number_area.width() - 12
                    marker_y = int(top) + height // 2 - 2
                    
                    painter.setPen(Qt.NoPen)
                    painter.setBrush(QColor("#888"))
                    
                    if is_folded:
                        arrow = QPolygon([
                            QPoint(marker_x, marker_y - 4),
                            QPoint(marker_x + 8, marker_y),
                            QPoint(marker_x, marker_y + 4)
                        ])
                    else:
                        arrow = QPolygon([
                            QPoint(marker_x, marker_y - 2),
                            QPoint(marker_x + 8, marker_y - 2),
                            QPoint(marker_x + 4, marker_y + 5)
                        ])
                    painter.drawPolygon(arrow)

            block = block.next()
            top = bottom
            bottom = top + self.blockBoundingRect(block).height()
            block_number += 1

class FindReplaceBar(QWidget):
    def __init__(self, parent=None, editor=None):
        super().__init__(parent)
        self.editor = editor
        self.layout = QVBoxLayout(self)
        self.layout.setContentsMargins(5, 5, 5, 5)
        self.layout.setSpacing(2)
        self.setStyleSheet("background: #1e293b; border-bottom: 1px solid #334155;")
        
        h1 = QHBoxLayout()
        self.search_input = QLineEdit()
        self.search_input.setPlaceholderText("Find...")
        self.search_input.returnPressed.connect(self.find_next)
        h1.addWidget(self.search_input)
        
        self.btn_next = QPushButton("↓")
        self.btn_next.clicked.connect(self.find_next)
        self.btn_next.setFixedWidth(30)
        
        self.btn_prev = QPushButton("↑")
        self.btn_prev.clicked.connect(self.find_prev)
        self.btn_prev.setFixedWidth(30)
        
        self.btn_toggle_replace = QPushButton("R")
        self.btn_toggle_replace.setCheckable(True)
        self.btn_toggle_replace.clicked.connect(self.toggle_replace)
        self.btn_toggle_replace.setFixedWidth(30)
        
        self.btn_close = QPushButton("X")
        self.btn_close.clicked.connect(self.hide)
        self.btn_close.setFixedWidth(30)
        self.btn_close.setStyleSheet("background: transparent; color: #ef4444; font-weight: bold;")
        
        h1.addWidget(self.btn_next)
        h1.addWidget(self.btn_prev)
        h1.addWidget(self.btn_toggle_replace)
        h1.addWidget(self.btn_close)
        self.layout.addLayout(h1)

        self.replace_container = QWidget()
        h2 = QHBoxLayout(self.replace_container)
        h2.setContentsMargins(0,0,0,0)
        
        self.replace_input = QLineEdit()
        self.replace_input.setPlaceholderText("Replace with...")
        h2.addWidget(self.replace_input)
        
        self.btn_replace = QPushButton("Replace")
        self.btn_replace.clicked.connect(self.do_replace)
        
        self.btn_replace_all = QPushButton("All")
        self.btn_replace_all.clicked.connect(self.do_replace_all)
        
        h2.addWidget(self.btn_replace)
        h2.addWidget(self.btn_replace_all)
        self.layout.addWidget(self.replace_container)
        
        self.replace_container.hide()
        self.hide()

    def toggle_replace(self):
        if self.btn_toggle_replace.isChecked():
            self.replace_container.show()
        else:
            self.replace_container.hide()

    def find_next(self):
        text = self.search_input.text()
        if text:
            self.editor.find(text)

    def find_prev(self):
        text = self.search_input.text()
        if text:
            self.editor.find(text, QPlainTextEdit.FindBackward)

    def do_replace(self):
        if self.editor.textCursor().hasSelection():
            self.editor.textCursor().insertText(self.replace_input.text())
        self.find_next()

    def do_replace_all(self):
        text = self.search_input.text()
        rtext = self.replace_input.text()
        if not text:
            return
        doc = self.editor.toPlainText()
        new_doc = doc.replace(text, rtext)
        self.editor.setPlainText(new_doc)

    def show_find(self):
        self.show()
        self.search_input.setFocus()
        self.search_input.selectAll()

class ConsolePage(QWebEnginePage):
    def __init__(self, console_widget, parent=None):
        super().__init__(parent)
        self.console_widget = console_widget

    def javaScriptConsoleMessage(self, level, message, lineNumber, sourceID):
        level_str = "LOG"
        color = "#e2e8f0"
        if level == QWebEnginePage.InfoMessageLevel:
            level_str = "INFO"
            color = "#60a5fa"
        elif level == QWebEnginePage.WarningMessageLevel:
            level_str = "WARN"
            color = "#facc15"
        elif level == QWebEnginePage.ErrorMessageLevel:
            level_str = "ERROR"
            color = "#f87171"
        item = QListWidgetItem(f"[{level_str}] {message} (Line: {lineNumber})")
        item.setForeground(QColor(color))
        self.console_widget.addItem(item)
        self.console_widget.scrollToBottom()

class DevToolsWindow(QMainWindow):
    def __init__(self, page, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Developer Tools")
        self.resize(800, 600)
        self.view = QWebEngineView()
        self.setCentralWidget(self.view)
        self.view.page().setInspectedPage(page)

class SettingsDialog(QDialog):
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
        self.add_behavior_tab()
        
        button_box = QWidget()
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
        
        console_group = QGroupBox("Console")
        console_layout = QGridLayout(console_group)
        console_layout.addWidget(QLabel("Max Console Height (px):"), 0, 0)
        self.console_height_spin = QSpinBox()
        self.console_height_spin.setRange(50, 500)
        self.console_height_spin.setValue(self.new_settings["console_height"])
        self.console_height_spin.valueChanged.connect(lambda v: self.new_settings.update({"console_height": v}))
        console_layout.addWidget(self.console_height_spin, 0, 1)
        layout.addWidget(console_group, 0, 0, 1, 2)
        
        window_group = QGroupBox("Window")
        window_layout = QGridLayout(window_group)
        window_layout.addWidget(QLabel("Initial Splitter Ratio:"), 0, 0)
        self.splitter_ratio_combo = QComboBox()
        self.splitter_ratio_combo.addItems(["1:1", "2:1", "1:2"])
        ratios = {"1:1": [500, 500], "2:1": [667, 333], "1:2": [333, 667]}
        current_ratio = self.new_settings["initial_splitter_ratio"]
        current_str = next((k for k, v in ratios.items() if v == current_ratio), "1:1")
        self.splitter_ratio_combo.setCurrentText(current_str)
        self.splitter_ratio_combo.currentTextChanged.connect(lambda t: self.new_settings.update({"initial_splitter_ratio": ratios[t]}))
        window_layout.addWidget(self.splitter_ratio_combo, 0, 1)
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
        
        font_layout.addWidget(QLabel("Tab Width:"), 2, 0)
        self.tab_spin = QSpinBox()
        self.tab_spin.setRange(1, 8)
        self.tab_spin.setValue(self.new_settings.get("tab_width", 4))
        self.tab_spin.valueChanged.connect(lambda v: self.new_settings.update({"tab_width": v}))
        font_layout.addWidget(self.tab_spin, 2, 1)
        
        font_group.setLayout(font_layout)
        layout.addWidget(font_group, 0, 0, 1, 2)
        layout.setRowStretch(1, 1)
        self.tab_widget.addTab(tab, "Editor")

    def add_behavior_tab(self):
        tab = QWidget()
        layout = QGridLayout(tab)
        
        beh_group = QGroupBox("Runtime")
        beh_layout = QGridLayout(beh_group)
        
        beh_layout.addWidget(QLabel("Auto-Run Delay (ms):"), 0, 0)
        self.delay_spin = QSpinBox()
        self.delay_spin.setRange(100, 5000)
        self.delay_spin.setValue(self.new_settings.get("auto_run_delay", 1000))
        self.delay_spin.valueChanged.connect(lambda v: self.new_settings.update({"auto_run_delay": v}))
        beh_layout.addWidget(self.delay_spin, 0, 1)
        
        beh_layout.addWidget(QLabel("Default Zoom:"), 1, 0)
        self.zoom_spin = QDoubleSpinBox()
        self.zoom_spin.setRange(0.25, 5.0)
        self.zoom_spin.setSingleStep(0.1)
        self.zoom_spin.setValue(self.new_settings.get("zoom_factor", 1.0))
        self.zoom_spin.valueChanged.connect(lambda v: self.new_settings.update({"zoom_factor": v}))
        beh_layout.addWidget(self.zoom_spin, 1, 1)
        
        beh_group.setLayout(beh_layout)
        layout.addWidget(beh_group, 0, 0, 1, 2)
        
        layout.setRowStretch(1, 1)
        self.tab_widget.addTab(tab, "Behavior")

    def select_font(self):
        font, ok = QFontDialog.getFont(QFont(self.new_settings["editor_font_family"], self.new_settings["editor_font_size"]), self)
        if ok:
            self.new_settings["editor_font_family"] = font.family()
            self.new_settings["editor_font_size"] = font.pointSize()
            self.font_line_edit.setText(font.family())
            self.font_size_spin.setValue(font.pointSize())

class NativeBrowser(QMainWindow):
    def __init__(self, initial_file=None, initial_content=None, clean_mode=False, geometry=None, fullscreen=False, maximized=False, zoom=1.0, no_patch=False):
        super().__init__()
        
        self.settings = {
            "console_height": 150,
            "initial_splitter_ratio": [500, 500],
            "editor_font_family": "Consolas",
            "editor_font_size": 14,
            "tab_width": 4,
            "auto_run_delay": 1000,
            "zoom_factor": zoom
        }
        self.current_file = None
        self.is_modified = False
        self.devtools_window = None
        self.is_cinematic = False
        self.clean_mode = clean_mode
        self.no_patch = no_patch
        self.auto_render_timer = QTimer()
        self.auto_render_timer.setSingleShot(True)
        self.auto_render_timer.setInterval(self.settings["auto_run_delay"])
        self.auto_render_timer.timeout.connect(self.render_content)

        self.setWindowTitle("Wutblick Engine")
        
        if geometry:
            try:
                w, h = map(int, geometry.lower().split('x'))
                self.resize(w, h)
            except:
                self.resize(1280, 800)
        else:
            self.resize(1280, 800)
        
        if maximized:
            self.showMaximized()
        if fullscreen:
            self.showFullScreen()
        
        self.setStyleSheet(DARK_THEME)
        
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QVBoxLayout(main_widget)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        self.setup_menus()

        self.splitter = QSplitter(Qt.Horizontal)
        layout.addWidget(self.splitter, 1)

        self.editor_container = QWidget()
        editor_layout = QVBoxLayout(self.editor_container)
        editor_layout.setContentsMargins(0,0,0,0)
        editor_layout.setSpacing(0)
        editor_label = QLabel(" SOURCE CODE")
        editor_label.setStyleSheet("background: #1e293b; border-bottom: 1px solid #334155;")
        editor_layout.addWidget(editor_label)
        
        self.editor = CodeEditor()
        self.highlighter = HTMLHighlighter(self.editor.document())
        self.editor.textChanged.connect(self.handle_text_change)
        self.editor.cursorPositionChanged.connect(self.update_status)
        
        self.find_bar = FindReplaceBar(self.editor_container, self.editor)
        editor_layout.addWidget(self.find_bar)
        editor_layout.addWidget(self.editor)
        self.splitter.addWidget(self.editor_container)

        self.preview_container = QWidget()
        preview_layout = QVBoxLayout(self.preview_container)
        preview_layout.setContentsMargins(0,0,0,0)
        preview_layout.setSpacing(0)
        self.preview_label = QLabel(" PREVIEW & CONSOLE")
        self.preview_label.setStyleSheet("background: #1e293b; border-bottom: 1px solid #334155;")
        preview_layout.addWidget(self.preview_label)

        self.web_view = QWebEngineView()
        ws = self.web_view.settings()
        ws.setAttribute(QWebEngineSettings.WebGLEnabled, True)
        ws.setAttribute(QWebEngineSettings.LocalStorageEnabled, True)
        ws.setAttribute(QWebEngineSettings.LocalContentCanAccessRemoteUrls, True)
        ws.setAttribute(QWebEngineSettings.LocalContentCanAccessFileUrls, True)
        ws.setAttribute(QWebEngineSettings.Accelerated2dCanvasEnabled, True)
        
        self.console_list = QListWidget()
        self.web_page = ConsolePage(self.console_list, self.web_view)
        self.web_view.setPage(self.web_page)
        
        self.preview_splitter = QSplitter(Qt.Vertical)
        self.preview_splitter.addWidget(self.web_view)
        self.preview_splitter.addWidget(self.console_list)
        preview_layout.addWidget(self.preview_splitter)
        self.splitter.addWidget(self.preview_container)

        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        
        self.zoom_label = QLabel("Zoom: 100%")
        self.zoom_label.setStyleSheet("padding: 0 10px; color: #38bdf8; font-weight: bold;")
        self.status_bar.addPermanentWidget(self.zoom_label)
        
        self.update_status()

        if initial_content:
            self.editor.setPlainText(initial_content)
            self.current_file = None
            self.is_modified = False
            self.update_title()
            self.render_content()
        elif initial_file and os.path.exists(initial_file):
            self.load_file(initial_file)

        self.apply_settings()
        self.render_content()
        
        if self.clean_mode:
            self.toggle_cinematic()
            self.menuBar().hide()

    def setup_menus(self):
        mb = self.menuBar()
        
        file_menu = mb.addMenu("File")
        a_open = QAction("Open...", self)
        a_open.setShortcut("Ctrl+O")
        a_open.triggered.connect(self.open_file)
        file_menu.addAction(a_open)
        
        a_save = QAction("Save", self)
        a_save.setShortcut("Ctrl+S")
        a_save.triggered.connect(self.save_file)
        file_menu.addAction(a_save)
        
        a_save_as = QAction("Save As...", self)
        a_save_as.setShortcut("Ctrl+Shift+S")
        a_save_as.triggered.connect(self.save_file_as)
        file_menu.addAction(a_save_as)
        
        file_menu.addSeparator()
        a_snap = QAction("Snapshot", self)
        a_snap.setShortcut("Ctrl+Alt+S")
        a_snap.triggered.connect(self.take_snapshot)
        file_menu.addAction(a_snap)
        
        file_menu.addSeparator()
        a_exit = QAction("Exit", self)
        a_exit.setShortcut("Ctrl+Q")
        a_exit.triggered.connect(self.close)
        file_menu.addAction(a_exit)

        edit_menu = mb.addMenu("Edit")
        a_undo = QAction("Undo", self)
        a_undo.setShortcut("Ctrl+Z")
        a_undo.triggered.connect(lambda: self.editor.undo())
        edit_menu.addAction(a_undo)
        
        a_redo = QAction("Redo", self)
        a_redo.setShortcut("Ctrl+Y")
        a_redo.triggered.connect(lambda: self.editor.redo())
        edit_menu.addAction(a_redo)
        
        edit_menu.addSeparator()
        
        a_find = QAction("Find/Replace", self)
        a_find.setShortcut("Ctrl+F")
        a_find.triggered.connect(lambda: self.find_bar.show_find())
        edit_menu.addAction(a_find)
        
        a_goto = QAction("Go To Line", self)
        a_goto.setShortcut("Ctrl+G")
        a_goto.triggered.connect(self.goto_line)
        edit_menu.addAction(a_goto)
        
        edit_menu.addSeparator()
        a_settings = QAction("Preferences", self)
        a_settings.setShortcut("Ctrl+,")
        a_settings.triggered.connect(self.show_settings)
        edit_menu.addAction(a_settings)

        code_menu = mb.addMenu("Code")
        a_fold = QAction("Collapse All", self)
        a_fold.setShortcut("Ctrl+Shift+Up")
        a_fold.triggered.connect(lambda: self.editor.fold_all())
        code_menu.addAction(a_fold)
        
        a_unfold = QAction("Expand All", self)
        a_unfold.setShortcut("Ctrl+Shift+Down")
        a_unfold.triggered.connect(lambda: self.editor.unfold_all())
        code_menu.addAction(a_unfold)
        
        a_wrap = QAction("Toggle Wrap", self)
        a_wrap.setCheckable(True)
        a_wrap.setShortcut("Alt+Z")
        a_wrap.triggered.connect(self.toggle_wrap)
        code_menu.addAction(a_wrap)

        view_menu = mb.addMenu("View")
        a_render = QAction("Render", self)
        a_render.setShortcut("F5")
        a_render.triggered.connect(self.render_content)
        view_menu.addAction(a_render)
        
        self.act_autorun = QAction("Auto-Run", self)
        self.act_autorun.setCheckable(True)
        self.act_autorun.setShortcut("Ctrl+Shift+R")
        view_menu.addAction(self.act_autorun)
        
        view_menu.addSeparator()
        
        a_zoom_in = QAction("Zoom In", self)
        a_zoom_in.setShortcut("Ctrl++")
        a_zoom_in.triggered.connect(lambda: self.change_zoom(0.1))
        view_menu.addAction(a_zoom_in)
        
        a_zoom_out = QAction("Zoom Out", self)
        a_zoom_out.setShortcut("Ctrl+-")
        a_zoom_out.triggered.connect(lambda: self.change_zoom(-0.1))
        view_menu.addAction(a_zoom_out)
        
        a_zoom_reset = QAction("Reset Zoom", self)
        a_zoom_reset.setShortcut("Ctrl+0")
        a_zoom_reset.triggered.connect(lambda: self.change_zoom(0, reset=True))
        view_menu.addAction(a_zoom_reset)
        
        view_menu.addSeparator()
        
        a_focus = QAction("Focus Mode", self)
        a_focus.setShortcut("F4")
        a_focus.triggered.connect(self.toggle_cinematic)
        view_menu.addAction(a_focus)

        tools_menu = mb.addMenu("Tools")
        a_ui = QAction("UI Editor", self)
        a_ui.setShortcut("F6")
        a_ui.triggered.connect(self.inject_gui_editor)
        tools_menu.addAction(a_ui)
        
        a_hud = QAction("Debug HUD", self)
        a_hud.setShortcut("F7")
        a_hud.triggered.connect(self.inject_debug_hud)
        tools_menu.addAction(a_hud)
        
        tools_menu.addSeparator()
        
        a_browser = QAction("Open in Browser", self)
        a_browser.setShortcut("Ctrl+B")
        a_browser.triggered.connect(self.open_external)
        tools_menu.addAction(a_browser)
        
        a_dev = QAction("Chrome DevTools", self)
        a_dev.setShortcut("F12")
        a_dev.triggered.connect(self.open_devtools)
        tools_menu.addAction(a_dev)

    def update_status(self):
        cursor = self.editor.textCursor()
        self.status_bar.showMessage(f"Ln {cursor.blockNumber() + 1}, Col {cursor.columnNumber() + 1} | {self.current_file or 'Untitled'}")

    def update_title(self):
        name = os.path.basename(self.current_file) if self.current_file else "Untitled"
        mod = "*" if self.is_modified else ""
        self.setWindowTitle(f"{name}{mod} - Wutblick Engine")

    def handle_text_change(self):
        if not self.is_modified:
            self.is_modified = True
            self.update_title()
        if self.act_autorun.isChecked():
            self.auto_render_timer.start()

    def toggle_wrap(self, checked):
        mode = QPlainTextEdit.WidgetWidth if checked else QPlainTextEdit.NoWrap
        self.editor.setLineWrapMode(mode)

    def goto_line(self):
        num, ok = QInputDialog.getInt(self, "Go To Line", "Line Number:", 1, 1, self.editor.blockCount())
        if ok:
            block = self.editor.document().findBlockByNumber(num - 1)
            cursor = self.editor.textCursor()
            cursor.setPosition(block.position())
            self.editor.setTextCursor(cursor)
            self.editor.centerCursor()

    def apply_settings(self):
        self.console_list.setMaximumHeight(self.settings["console_height"])
        if self.editor_container.isVisible():
            self.splitter.setSizes(self.settings["initial_splitter_ratio"])
        font = QFont(self.settings["editor_font_family"])
        font.setPointSize(self.settings["editor_font_size"])
        self.editor.setFont(font)
        self.editor.setTabStopDistance(self.editor.fontMetrics().width(' ') * self.settings["tab_width"])
        self.auto_render_timer.setInterval(self.settings["auto_run_delay"])
        
        self.web_view.setZoomFactor(self.settings["zoom_factor"])
        self.update_zoom_label()

    def change_zoom(self, delta, reset=False):
        if reset:
            self.settings["zoom_factor"] = 1.0
        else:
            self.settings["zoom_factor"] += delta
            self.settings["zoom_factor"] = max(0.25, min(5.0, self.settings["zoom_factor"]))
        self.web_view.setZoomFactor(self.settings["zoom_factor"])
        self.update_zoom_label()

    def update_zoom_label(self):
        percentage = int(self.settings["zoom_factor"] * 100)
        self.zoom_label.setText(f"Zoom: {percentage}%")

    def render_content(self):
        self.console_list.clear()
        html_content = self.editor.toPlainText()
        if not self.no_patch:
            patch_target = "else if(r2 > 0.5) discard;"
            if patch_target in html_content:
                html_content = html_content.replace(patch_target, "")
                self.console_list.addItem(QListWidgetItem("[SYSTEM] Shader patch applied."))
        self.web_view.setHtml(html_content, QUrl("local://"))

    def inject_gui_editor(self):
        self.web_view.page().runJavaScript(GUI_EDITOR_JS)
        
    def inject_debug_hud(self):
        self.web_view.page().runJavaScript(DEBUG_HUD_JS)

    def toggle_cinematic(self):
        self.is_cinematic = not self.is_cinematic
        visible = not self.is_cinematic
        self.editor_container.setVisible(visible)
        self.console_list.setVisible(visible)
        self.preview_label.setVisible(visible)
        self.status_bar.setVisible(visible)
        if self.is_cinematic:
            self.preview_splitter.setSizes([1000, 0])
        else:
            self.apply_settings()
            
    def take_snapshot(self):
        filename, _ = QFileDialog.getSaveFileName(self, "Save Snapshot", "snapshot.png", "Images (*.png)")
        if filename:
            self.web_view.grab().save(filename)

    def open_external(self):
        temp_path = os.path.abspath("temp_render.html")
        with open(temp_path, "w", encoding="utf-8") as f:
            f.write(self.editor.toPlainText())
        QDesktopServices.openUrl(QUrl.fromLocalFile(temp_path))

    def open_devtools(self):
        if not self.devtools_window:
            self.devtools_window = DevToolsWindow(self.web_view.page(), self)
        self.devtools_window.show()

    def open_file(self):
        if self.is_modified:
            ans = QMessageBox.question(self, "Unsaved Changes", "Save changes before opening?", QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel)
            if ans == QMessageBox.Cancel: return
            if ans == QMessageBox.Yes: self.save_file()
        
        file_path, _ = QFileDialog.getOpenFileName(self, "Open HTML", "", "HTML (*.html);;Txt (*.txt)")
        if file_path:
            self.load_file(file_path)

    def load_file(self, path):
        try:
            with open(path, 'r', encoding='utf-8') as f:
                self.editor.setPlainText(f.read())
            self.current_file = path
            self.is_modified = False
            self.update_title()
            self.render_content()
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Could not load file: {e}")

    def save_file(self):
        if not self.current_file:
            self.save_file_as()
        else:
            try:
                with open(self.current_file, 'w', encoding='utf-8') as f:
                    f.write(self.editor.toPlainText())
                self.is_modified = False
                self.update_title()
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Could not save file: {e}")

    def save_file_as(self):
        path, _ = QFileDialog.getSaveFileName(self, "Save HTML As", "", "HTML (*.html)")
        if path:
            self.current_file = path
            self.save_file()

    def show_settings(self):
        dialog = SettingsDialog(self.settings, self)
        if dialog.exec_() == QDialog.Accepted:
            self.settings = dialog.new_settings
            self.apply_settings()

    def closeEvent(self, event):
        if self.is_modified:
            ans = QMessageBox.question(self, "Unsaved Changes", "Save changes before exiting?", QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel)
            if ans == QMessageBox.Cancel:
                event.ignore()
                return
            if ans == QMessageBox.Yes:
                self.save_file()
        
        self.auto_render_timer.stop()
        self.web_view.stop()
        self.web_view.setPage(None)
        event.accept()

if __name__ == "__main__":
    try:
        QApplication.setAttribute(Qt.AA_UseDesktopOpenGL)
        QApplication.setAttribute(Qt.AA_ShareOpenGLContexts)
        
        flags = [
            "--no-sandbox",
            "--disable-web-security",
            "--ignore-gpu-blacklist",
            "--enable-gpu-rasterization",
            "--enable-webgl-developer-extensions",
            "--enable-webgl-draft-extensions",
            "--disable-gpu-shader-disk-cache"
        ]
        if args.debug_port:
            flags.append(f"--remote-debugging-port={args.debug_port}")
            
        os.environ["QTWEBENGINE_CHROMIUM_FLAGS"] = " ".join(flags)
        
        app = QApplication(sys.argv)
        os.environ["QT_AUTO_SCREEN_SCALE_FACTOR"] = "1"
        
        app.setStyleSheet(DARK_THEME)

        target_file = args.file
        target_content = None
        
        if target_file is None:
            # ROBUST SCANNING LOGIC
            # If running as compiled exe, sys._MEIPASS exists.
            # If running as script, use current directory.
            if hasattr(sys, '_MEIPASS'):
                base_dir = sys._MEIPASS
                print(f"Scanning internal bundle directory: {base_dir}")
            else:
                base_dir = os.path.dirname(os.path.abspath(__file__))
                print(f"Scanning local directory: {base_dir}")

            # Scan for any HTML file in the base directory
            for f in os.listdir(base_dir):
                if f.lower().endswith('.html'):
                    full_path = os.path.join(base_dir, f)
                    try:
                        print(f"Found candidate file: {full_path}")
                        with open(full_path, 'r', encoding='utf-8') as file_obj:
                            target_content = file_obj.read()
                        print("Loaded file content successfully.")
                        break # Stop after finding the first HTML file
                    except Exception as e:
                        print(f"Error reading {f}: {e}")

        print(f"Starting Wutblick Engine (Clean Mode: {args.clean}, Patch: {not args.no_patch})...")
        window = NativeBrowser(
            initial_file=target_file if not target_content else None,
            initial_content=target_content,
            clean_mode=args.clean,
            geometry=args.geometry,
            fullscreen=args.fullscreen,
            maximized=args.maximized,
            zoom=args.zoom,
            no_patch=args.no_patch
        )
        window.show()
        
        sys.exit(app.exec_())
        
    except Exception as e:
        print("\n!!! CRASH DETECTED !!!")
        traceback.print_exc()
        input("Press Enter to exit...")