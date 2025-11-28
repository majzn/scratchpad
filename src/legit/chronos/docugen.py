#!/usr/bin/env python3
"""
API Documentation Generator
Parses Doxygen-style comments from C headers and generates HTML documentation.
"""

import re
import os
import argparse
import json
import base64
from pathlib import Path
from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass, field

# --- Default Content for Generic Use ---
DEFAULT_TITLE = "Project API Documentation"
DEFAULT_SUBTITLE = "Auto-generated reference guide."
DEFAULT_OVERVIEW_TITLE = "Overview"
DEFAULT_OVERVIEW_DESCRIPTION = "This is the API documentation for the project. It provides a detailed reference for all documented functions, structures, and constants defined in the header files."
DEFAULT_FEATURES = [
    "Modular design for flexible architecture",
    "Comprehensive API coverage",
    "Efficient and compact documentation layout",
    "Hierarchical file navigation system",
]
# ---------------------------------------

@dataclass
class Config:
    title: str = DEFAULT_TITLE
    subtitle: str = DEFAULT_SUBTITLE
    output_file: str = "documentation.html"
    css_file: Optional[str] = None
    theme: str = "default"
    include_toc: bool = True
    include_overview: bool = True
    include_stats: bool = True
    section_order: List[str] = field(default_factory=lambda: ["overview", "defines", "structs", "enums", "functions"])
    custom_sections: Dict[str, str] = field(default_factory=dict)
    
    overview_title: str = DEFAULT_OVERVIEW_TITLE
    overview_description: str = DEFAULT_OVERVIEW_DESCRIPTION
    overview_features: List[str] = field(default_factory=lambda: DEFAULT_FEATURES)
    
    include_source_code: bool = True
    
    @classmethod
    def from_file(cls, config_file: str) -> 'Config':
        with open(config_file, 'r') as f:
            data = json.load(f)
        return cls(**data)

@dataclass
class DocComment:
    brief: str = ""
    description: str = ""
    params: List[Tuple[str, str]] = field(default_factory=list)
    returns: str = ""
    tags: Dict[str, str] = field(default_factory=dict)

@dataclass
class Function:
    name: str
    signature: str
    doc: DocComment
    category: str = "General"

@dataclass
class Struct:
    name: str
    doc: DocComment
    members: List[Tuple[str, str, str]] = field(default_factory=list)

@dataclass
class Enum:
    name: str
    doc: DocComment
    values: List[Tuple[str, str]] = field(default_factory=list)

@dataclass
class Define:
    name: str
    value: str
    doc: DocComment

@dataclass
class FileData:
    raw_content: str = ""
    file_doc: DocComment = field(default_factory=DocComment)
    entities: Dict[str, List] = field(default_factory=lambda: {
        "defines": [], "structs": [], "enums": [], "functions": []
    })

class DocParser:
    
    def __init__(self):
        self.functions: List[Function] = []
        self.structs: List[Struct] = []
        self.enums: List[Enum] = []
        self.defines: List[Define] = []
        self.files_map: Dict[str, FileData] = {}
        self.current_file: str = ""
        
    def parse_doc_comment(self, comment: str) -> DocComment:
        doc = DocComment()
        lines = comment.strip().split('\n')
        current_section = None
        description_lines = []
        
        for line in lines:
            line = re.sub(r'^\s*[/*]+\s*', '', line)
            line = re.sub(r'\s*\*+/?\s*$', '', line)
            line = line.strip()
            
            if not line:
                continue
            
            if line.startswith('@brief'):
                doc.brief = line[6:].strip()
                current_section = None
            elif line.startswith('@param'):
                match = re.match(r'@param\s+(\w+)\s+(.+)', line)
                if match:
                    doc.params.append((match.group(1), match.group(2)))
                current_section = 'param'
            elif line.startswith('@return'):
                doc.returns = line[7:].strip()
                current_section = 'return'
            elif line.startswith('@def'):
                doc.tags['def'] = line[4:].strip()
                current_section = None
            elif line.startswith('@file'):
                doc.tags['file'] = line[5:].strip()
                current_section = None
            elif line.startswith('@'):
                tag_match = re.match(r'@(\w+)\s+(.+)', line)
                if tag_match:
                    doc.tags[tag_match.group(1)] = tag_match.group(2)
                current_section = None
            elif current_section == 'param' and doc.params:
                doc.params[-1] = (doc.params[-1][0], doc.params[-1][1] + ' ' + line)
            elif current_section == 'return':
                doc.returns += ' ' + line
            else:
                description_lines.append(line)
        
        doc.description = ' '.join(description_lines)
        return doc
    
    def parse_file(self, filepath: str):
        file_name = os.path.basename(filepath)
        self.current_file = file_name

        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()

        file_data = FileData(raw_content=content)
        self.files_map[file_name] = file_data
        
        # Extract file-level documentation (assumes it's the first block comment)
        file_doc_match = re.match(r'/\*\*(?:.|[\r\n])*?\*/', content, re.MULTILINE)
        if file_doc_match:
            comment_content = file_doc_match.group(0)
            file_data.file_doc = self.parse_doc_comment(comment_content)
        
        # Find all documentation comments with their associated code
        pattern = r'/\*\*\s*\n((?:.*?\n)*?)\s*\*/\s*\n([^/\n][^\n]*)'
        matches = re.finditer(pattern, content, re.MULTILINE)
        
        for match in matches:
            comment = match.group(1)
            code = match.group(2).strip()
            doc = self.parse_doc_comment(comment)
            
            if code.startswith('typedef struct') or code.startswith('struct'):
                struct = self.parse_struct(code, doc, content, match.end())
                if struct:
                    self.structs.append(struct)
                    file_data.entities["structs"].append(struct.name)
            elif code.startswith('typedef enum') or code.startswith('enum'):
                enum = self.parse_enum(code, doc, content, match.end())
                if enum:
                    self.enums.append(enum)
                    file_data.entities["enums"].append(enum.name)
            elif code.startswith('#define'):
                define = self.parse_define(code, doc)
                if define:
                    self.defines.append(define)
                    file_data.entities["defines"].append(define.name)
            elif ';' in code or '(' in code: 
                func = self.parse_function(code, doc)
                if func:
                    self.functions.append(func)
                    file_data.entities["functions"].append(func.name)
    
    def parse_function(self, code: str, doc: DocComment) -> Optional[Function]:
        match = re.search(r'\b(\w+)\s*\([^)]*\)', code)
        if not match:
            return None
        
        name = match.group(1)
        
        if name.startswith('op_handler_'):
            category = "DSP Handlers"
        elif name.startswith('create') or name.startswith('destroy'):
            category = "Lifecycle Management"
        elif name.startswith('get') or name.startswith('set'):
            category = "Accessors"
        else:
            category = "General Utilities"
        
        return Function(name=name, signature=code, doc=doc, category=category)
    
    def parse_struct(self, code: str, doc: DocComment, full_content: str, start_pos: int) -> Optional[Struct]:
        match = re.search(r'(?:typedef\s+)?struct\s+(\w+)', code)
        if not match:
            return None
        
        name = match.group(1)
        
        search_window = full_content[max(0, start_pos-200):min(len(full_content), start_pos+2000)]
        struct_match = re.search(r'struct\s+' + re.escape(name) + r'\s*\{([^}]*)\}', search_window, re.DOTALL)
        members = []
        
        if struct_match:
            body = struct_match.group(1)
            
            member_pattern = r'(\s*/\*\*(?:.|[\r\n])*?\*/\s*)?([^;]+);\s*(\/\*\*<\s*(.*?)\s*\*\/\s*)?'
            
            for member_match in re.finditer(member_pattern, body, re.DOTALL):
                member_decl = member_match.group(2).strip()
                inline_doc = member_match.group(4) or ""
                
                member_doc = inline_doc 
                
                if not member_decl:
                    continue
                
                member_doc = re.sub(r'\s+', ' ', member_doc).strip()
                
                parts = member_decl.split()
                if not parts:
                    continue
                    
                full_name_decl = parts[-1] 
                
                member_name = re.sub(r'(\*|\[|\])+', '', full_name_decl).rstrip(';')
                
                member_type = member_decl[:-len(full_name_decl)].strip() + ' ' + re.sub(r'(\w+)', '', full_name_decl).strip()
                
                if not member_type.strip():
                     member_type = ' '.join(parts[:-1])
                
                members.append((member_type.strip(), member_name, member_doc))
        
        return Struct(name=name, doc=doc, members=members)
    
    def parse_enum(self, code: str, doc: DocComment, full_content: str, start_pos: int) -> Optional[Enum]:
        match = re.search(r'(?:typedef\s+)?enum\s*(\w*)\s*\{', code)
        if not match:
            return None
        
        name = match.group(1) or "Anonymous"
        
        enum_match = re.search(r'enum[^{]*\{([^}]*)\}', full_content[start_pos-50:start_pos+1000])
        values = []
        
        if enum_match:
            body = enum_match.group(1)
            for line in body.split(','):
                line = line.strip()
                if not line:
                    continue
                
                comment_match = re.search(r'/\*\*<\s*(.*?)\s*\*/', line)
                value_match = re.search(r'(\w+)', line)
                
                if value_match:
                    value_name = value_match.group(1)
                    value_doc = comment_match.group(1) if comment_match else ""
                    values.append((value_name, value_doc))
        
        return Enum(name=name, doc=doc, values=values)
    
    def parse_define(self, code: str, doc: DocComment) -> Optional[Define]:
        match = re.match(r'#define\s+(\w+)\s+(.*)', code)
        if not match:
            return None
        
        name = match.group(1)
        value = match.group(2).strip()
        
        return Define(name=name, value=value, doc=doc)

class HTMLGenerator:
    
    def __init__(self, parser: DocParser, config: Config):
        self.parser = parser
        self.config = config
        self.custom_css = self._load_custom_css() if config.css_file else None
        
    def _load_custom_css(self) -> str:
        try:
            with open(self.config.css_file, 'r', encoding='utf-8') as f:
                return f.read()
        except Exception as e:
            print(f"Warning: Could not load CSS file {self.config.css_file}: {e}")
            return ""
    
    def generate(self, output_file: str = None):
        output = output_file or self.config.output_file
        html = self._generate_html()
        
        with open(output, 'w', encoding='utf-8') as f:
            f.write(html)
    
    def _generate_file_menu(self) -> str:
        """Generates the hierarchical navigation menu from parsed files and entities."""
        menu_html = '<h2>API Files</h2><ul class="file-list">'
        
        sorted_files = sorted(self.parser.files_map.keys())
        
        for file_name in sorted_files:
            file_data = self.parser.files_map[file_name].entities
            
            file_id = file_name.replace('.', '_').replace('-', '_')
            
            # Anchor to the top of the file section
            menu_html += f"""
            <li>
                <div class="file-name" onclick="window.location.hash='#file-{file_id}'; toggleMenu('{file_id}')">
                    <span class="file-arrow" id="arrow_{file_id}">▶</span>
                    {file_name}
                </div>
                <ul class="entry-list" id="menu_{file_id}">
            """
            
            entity_types = [
                ("defines", "Constants"), 
                ("structs", "Structures"), 
                ("enums", "Enumerations"), 
                ("functions", "Functions")
            ]
            
            for key, display_name in entity_types:
                if file_data[key]:
                    menu_html += f'<li><div class="entry-type">{display_name}</div><ul class="sub-entry-list">'
                    for name in sorted(file_data[key]):
                        anchor_name = name.replace('-', '_').replace('.', '_')
                        menu_html += f'<li><a href="#{key[:-1]}-{anchor_name}">{name}</a></li>'
                    menu_html += '</ul></li>'
            
            menu_html += '</ul></li>'
        
        menu_html += '</ul>'
        return menu_html
    
    def _generate_file_doc_header(self, file_name: str, file_data: FileData) -> str:
        """Generates a header with file-level documentation and source options."""
        file_id = file_name.replace('.', '_').replace('-', '_')
        doc = file_data.file_doc
        
        html = f"""
        <section class="file-documentation-section" id="file-{file_id}">
            <h2>File: {file_name}</h2>
            """
        
        if doc.brief or doc.description:
            html += f"""
            <div class="file-doc-summary">
                {f'<h3>Brief</h3><p class="description">{doc.brief}</p>' if doc.brief else ''}
                {f'<h3>Description</h3><p class="description">{doc.description}</p>' if doc.description else ''}
            </div>
            """
        
        if self.config.include_source_code:
            encoded_content = base64.b64encode(file_data.raw_content.encode('utf-8')).decode('utf-8')
            
            # Download link using Base64 Data URI for simple file download
            download_href = f"data:application/octet-stream;charset=utf-8;base64,{encoded_content}"

            html += f"""
            <div class="file-source-options">
                <a href="{download_href}" download="{file_name}" class="source-button download-button">Download Source</a>
                <button onclick="toggleSourceCode('{file_id}')" class="source-button show-button">Show Source Code</button>
            </div>
            <pre class="source-code-block" id="source_{file_id}"><code>{self._escape_html(file_data.raw_content)}</code></pre>
            """
        
        html += '<hr/>'
        return html


    def _generate_html(self) -> str:
        sections_html = []
        
        # Prepend the main overview section if enabled
        if self.config.include_overview:
            sections_html.append(self._generate_overview())
        
        # Iterate through files and generate file documentation header + entities
        for file_name, file_data in self.parser.files_map.items():
            sections_html.append(self._generate_file_doc_header(file_name, file_data))

            # Original section logic for the file's entities (functions, structs, etc.)
            for section in self.config.section_order:
                # We skip the main 'overview' when iterating over files to avoid repetition
                if section == "overview":
                    continue 
                elif section == "defines":
                    sections_html.append(self._generate_defines_section(file_name))
                elif section == "structs":
                    sections_html.append(self._generate_structs_section(file_name))
                elif section == "enums":
                    sections_html.append(self._generate_enums_section(file_name))
                elif section == "functions":
                    sections_html.append(self._generate_functions_section(file_name))
                elif section in self.config.custom_sections:
                    sections_html.append(self.config.custom_sections[section])
        
        # FIX: Renamed method call to match definition
        toc_html = self._generate_toc_section() if self.config.include_toc else "" 
        file_menu_html = self._generate_file_menu()

        return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{self.config.title} Documentation</title>
    <style>
        {self._get_css()}
    </style>
</head>
<body>
    <div class="doc-wrapper">
        <aside class="sidebar">
            <header class="sidebar-header">
                <h1>{self.config.title}</h1>
                <p class="subtitle">{self.config.subtitle}</p>
            </header>
            <nav class="file-menu">
                {file_menu_html}
                {toc_html}
            </nav>
        </aside>

        <div class="content">
            <main>
                {''.join(sections_html)}
            </main>
            
            <footer>
                <p>Generated by API Documentation Generator</p>
            </footer>
        </div>
    </div>
    
    <script>
        {self._get_javascript()}
    </script>
</body>
</html>"""
    
    def _generate_toc_section(self) -> str:
        """Generates the main document sections table of contents."""
        toc_items = []
        for section in self.config.section_order:
            if section == "overview" and self.config.include_overview:
                toc_items.append('<li><a href="#overview">Overview</a></li>')
            elif section == "defines" and self.parser.defines:
                toc_items.append('<li><a href="#defines">Constants & Defines</a></li>')
            elif section == "structs" and self.parser.structs:
                toc_items.append('<li><a href="#structs">Data Structures</a></li>')
            elif section == "enums" and self.parser.enums:
                toc_items.append('<li><a href="#enums">Enumerations</a></li>')
            elif section == "functions" and self.parser.functions:
                toc_items.append('<li><a href="#functions">Functions</a></li>')
            elif section in self.config.custom_sections:
                toc_items.append(f'<li><a href="#{section}">{section.title()}</a></li>')
        
        if not toc_items:
            return ""
        
        return f"""
        <nav class="main-toc">
            <h2>Sections</h2>
            <ul>
                {''.join(toc_items)}
            </ul>
        </nav>
        """

    def _get_css(self) -> str:
        if self.custom_css:
            return self.custom_css
        
        if self.config.theme == "dark":
            return self._get_dark_theme()
        elif self.config.theme == "minimal":
            return self._get_minimal_theme()
        else:
            return self._get_default_theme() 

    def _get_default_theme(self) -> str:
        return self._get_dark_theme() 

    def _get_dark_theme(self) -> str:
        # Note: Includes styles for new features: .file-source-options, .source-code-block, .file-doc-summary
        return """
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
            line-height: 1.4;
            color: #e0e0e0;
            background: #1e1e1e;
        }

        /* --- Global Layout (Sidebar & Content) --- */
        .doc-wrapper {
            display: flex;
            min-height: 100vh;
        }

        .sidebar {
            width: 250px;
            flex-shrink: 0;
            background: #252526;
            border-right: 1px solid #3c3c3c;
            position: sticky;
            top: 0;
            height: 100vh;
            overflow-y: auto;
        }

        .sidebar-header {
            padding: 1.2rem 1rem;
            background: #007acc;
            color: white;
            border-bottom: 3px solid #005f99;
        }

        .sidebar-header h1 {
            font-size: 1.4rem;
            margin: 0;
        }

        .sidebar-header .subtitle {
            font-size: 0.8rem;
            opacity: 0.8;
            margin-top: 0.2rem;
        }

        .content {
            flex-grow: 1;
            background: #1e1e1e;
            padding: 0;
            max-width: calc(100% - 250px);
        }

        /* --- File Menu Styles --- */
        .file-menu {
            padding: 1rem 0 2rem 0;
            font-size: 0.9rem;
        }

        .file-menu h2 {
            font-size: 1rem;
            color: #007acc;
            padding: 0.5rem 1rem;
            margin: 0;
            border-bottom: 1px solid #3c3c3c;
            font-weight: 600;
        }

        .file-list, .entry-list, .sub-entry-list, .main-toc ul {
            list-style: none;
            padding: 0;
            margin: 0;
        }

        .file-list > li {
            border-bottom: 1px solid #2d2d30;
        }

        .file-name {
            padding: 0.5rem 1rem;
            cursor: pointer;
            font-weight: 500;
            color: #e0e0e0;
            background: #2d2d30;
            transition: background 0.2s;
        }

        .file-name:hover {
            background: #3c3c3c;
        }

        .file-arrow {
            display: inline-block;
            width: 1em;
            transition: transform 0.2s;
        }
        
        .entry-list {
            display: none;
            background: #333333;
            padding: 0.5rem 0 0.5rem 0;
        }

        .entry-list.open {
            display: block;
        }

        .entry-type {
            font-weight: 600;
            color: #569cd6;
            padding: 0.3rem 1rem 0.3rem 1.5rem;
            font-size: 0.85rem;
        }

        .sub-entry-list a {
            display: block;
            padding: 0.2rem 1rem 0.2rem 2rem;
            color: #c0c0c0;
            text-decoration: none;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 0.85rem;
        }

        .sub-entry-list a:hover {
            background: #3c3c3c;
            color: #fff;
        }

        /* --- Main Content Section (Headers, Cards, etc.) --- */

        main {
            padding: 1.5rem 2rem;
        }
        
        section {
            margin-bottom: 2rem;
        }
        
        h2 {
            color: #007acc;
            border-bottom: 1px solid #3c3c3c;
            padding-bottom: 0.3rem;
            margin: 1.5rem 0 0.8rem 0;
            font-size: 1.5rem;
            font-weight: 600;
        }
        
        h3 {
            color: #ffffff;
            margin: 1.2rem 0 0.4rem 0;
            font-size: 1.2rem;
            font-weight: 500;
        }
        
        h4 {
            color: #ffffff;
            margin: 1rem 0 0.5rem 0;
            font-size: 1.1rem;
        }
        
        .function-card, .struct-card, .enum-card, .define-card {
            background: #2d2d30;
            border-left: 3px solid #569cd6;
            padding: 0.8rem 1rem;
            margin: 0.6rem 0;
            border-radius: 2px;
            transition: background-color 0.2s;
        }
        
        .function-card:hover, .struct-card:hover, .enum-card:hover, .define-card:hover {
            background: #333333;
        }
        
        .function-name, .struct-name, .enum-name, .define-name {
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 1rem;
            color: #569cd6;
            font-weight: bold;
            margin-bottom: 0.2rem;
            display: block;
        }

        /* --- Source Code Block Styles --- */

        .file-documentation-section {
            border-bottom: 1px solid #3c3c3c;
            padding-bottom: 1.5rem;
            margin-bottom: 1.5rem;
        }

        .file-doc-summary {
            padding: 0.5rem 0;
        }

        .file-source-options {
            margin: 1rem 0 0.5rem 0;
        }

        .source-button {
            background: #007acc;
            color: white;
            border: none;
            padding: 0.4rem 0.8rem;
            margin-right: 0.5rem;
            border-radius: 3px;
            cursor: pointer;
            font-size: 0.9rem;
            text-decoration: none;
            display: inline-block;
            transition: background 0.2s;
        }

        .source-button:hover {
            background: #005f99;
        }
        
        .source-code-block {
            display: none; /* Hidden by default */
            background: #1e1e1e;
            color: #b5cea8;
            padding: 1rem;
            border: 1px solid #3c3c3c;
            border-radius: 3px;
            overflow-x: auto;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 0.8rem;
            line-height: 1.2;
            white-space: pre-wrap;
            word-wrap: break-word;
        }
        
        .signature {
            background: #1e1e1e;
            color: #b5cea8;
            padding: 0.6rem;
            border-radius: 3px;
            overflow-x: auto;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 0.8rem;
            margin: 0.5rem 0 0.8rem 0;
            border: 1px solid #3c3c3c;
            position: relative;
        }
        
        .signature button {
            float: none;
            background: #007acc; 
            color: white; 
            border: none; 
            padding: 0.1rem 0.4rem; 
            border-radius: 3px; 
            cursor: pointer; 
            font-size: 0.75rem;
            position: absolute;
            top: 0.3rem; 
            right: 0.3rem;
        }

        .description {
            color: #c0c0c0;
            margin: 0.6rem 0;
            line-height: 1.6;
            font-size: 0.95rem;
        }
        
        .param-title, .returns-title, .members-title {
            font-weight: 600;
            color: #007acc;
            margin-top: 0.8rem;
            margin-bottom: 0.2rem;
            font-size: 0.9rem;
        }
        
        .param-item {
            margin: 0.1rem 0 0.1rem 0; 
            padding-left: 0.75rem;
            border-left: 2px solid #555;
            font-size: 0.9rem;
        }
        
        .param-name {
            font-family: 'Consolas', 'Courier New', monospace;
            background: #3c3c3c;
            padding: 0 0.3rem;
            border-radius: 2px;
            font-weight: 500;
            color: #569cd6;
        }
        
        .returns {
            margin: 0.6rem 0;
            padding: 0.5rem;
            background: #3c3c3c;
            border-left: 4px solid #007acc;
            border-radius: 3px;
            font-size: 0.9rem;
        }
        
        .member-table {
            width: 100%;
            border-collapse: collapse;
            margin: 0.6rem 0;
            font-size: 0.85rem;
        }
        
        .member-table th {
            background: #007acc;
            color: white;
            padding: 0.4rem 0.7rem;
            text-align: left;
            font-weight: 500;
        }
        
        .member-table td {
            padding: 0.3rem 0.7rem;
            border-bottom: 1px solid #3c3c3c;
        }
        
        .member-table tr:hover {
            background: #333333;
        }
        
        .member-type {
            font-family: 'Consolas', 'Courier New', monospace;
            color: #e0e0e0;
            font-weight: 500;
        }
        
        .member-name {
            font-family: 'Consolas', 'Courier New', monospace;
            color: #569cd6;
        }
        
        .category-section {
            margin: 2rem 0;
        }
        
        .category-title {
            background: #444444;
            color: #ffffff;
            padding: 0.5rem 1rem;
            border-radius: 3px;
            font-size: 1.05rem;
            margin: 1.2rem 0 0.6rem 0;
            font-weight: 500;
        }
        
        .stat-box {
            padding: 0.75rem;
            border-radius: 4px;
            text-align: center;
            background: #3c3c3c;
            border: 1px solid #007acc;
        }

        .stat-value {
            font-size: 1.5rem;
            font-weight: bold;
            color: #569cd6;
        }

        .stat-label {
            color: #e0e0e0;
            font-size: 0.9rem;
        }

        .main-toc {
            border-top: 1px solid #3c3c3c;
            padding-top: 1rem;
        }

        .main-toc ul a {
            display: block;
            padding: 0.3rem 1rem;
            font-weight: 500;
            color: #c0c0c0;
            text-decoration: none;
        }

        .main-toc ul a:hover {
            background: #333333;
            color: #fff;
        }

        footer {
            background: #1e1e1e;
            color: #888;
            text-align: center;
            padding: 0.8rem;
            margin-top: 1.5rem;
            font-size: 0.8rem;
        }
        """
    
    def _get_minimal_theme(self) -> str:
        # Simplified minimal theme reset
        return """
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: 'Georgia', serif;
            line-height: 1.8;
            color: #333;
            background: #fff;
            max-width: 900px;
            margin: 0 auto;
            padding: 2rem;
        }
        .doc-wrapper { display: block; min-height: auto; }
        .sidebar { display: none; }
        .content { max-width: 100%; }
        main { padding: 0; }
        header { border-bottom: 3px solid #000; padding-bottom: 2rem; margin-bottom: 3rem; background: #fff; color: #000; }
        .stat-box { background: #f0f0f0; border: 1px solid #000; }
        .source-code-block { background: #f5f5f5; border: 1px solid #ddd; color: #000; }
        """

    def _get_javascript(self) -> str:
        """Return JavaScript for interactivity, menu collapse, and source code toggle."""
        return """
        // Smooth scrolling for anchor links
        document.querySelectorAll('.file-menu a[href^="#"], .main-toc a[href^="#"]').forEach(anchor => {
            anchor.addEventListener('click', function (e) {
                e.preventDefault();
                const targetId = this.getAttribute('href').substring(1);
                const target = document.getElementById(targetId);
                
                if (target) {
                    const offset = 60; 
                    const bodyRect = document.body.getBoundingClientRect().top;
                    const targetRect = target.getBoundingClientRect().top;
                    const targetPosition = targetRect - bodyRect;
                    
                    window.scrollTo({
                        top: targetPosition - offset,
                        behavior: 'smooth'
                    });
                }
            });
        });

        // Add copy button to code blocks
        document.querySelectorAll('.signature').forEach(block => {
            const button = document.createElement('button');
            button.textContent = 'Copy';
            block.style.position = 'relative'; 
            block.insertBefore(button, block.firstChild);
            
            button.addEventListener('click', () => {
                const text = block.textContent.replace(button.textContent, '').trim();
                navigator.clipboard.writeText(text).then(() => {
                    button.textContent = 'Copied!';
                    setTimeout(() => button.textContent = 'Copy', 2000);
                });
            });
        });

        // Toggle hierarchical menu
        window.toggleMenu = function(fileId) {
            const menu = document.getElementById('menu_' + fileId);
            const arrow = document.getElementById('arrow_' + fileId);
            if (menu.classList.contains('open')) {
                menu.classList.remove('open');
                arrow.style.transform = 'rotate(0deg)';
            } else {
                menu.classList.add('open');
                arrow.style.transform = 'rotate(90deg)';
            }
        }

        // Toggle source code visibility
        window.toggleSourceCode = function(fileId) {
            const codeBlock = document.getElementById('source_' + fileId);
            const button = document.querySelector(`#file-${fileId} .show-button`);

            if (codeBlock.style.display === 'block') {
                codeBlock.style.display = 'none';
                button.textContent = 'Show Source Code';
            } else {
                codeBlock.style.display = 'block';
                button.textContent = 'Hide Source Code';
            }
        }
        """
    
    def _generate_overview(self) -> str:
        """Generate overview section using generic config fields."""
        feature_list_html = ''.join(f'<li>{f}</li>' for f in self.config.overview_features)
        
        return f"""
        <section id="overview" class="overview-section">
            <h2>{self.config.overview_title}</h2>
            <p class="description">
                {self.config.overview_description}
            </p>
            <h3>Key Features</h3>
            <ul class="key-features" style="margin-left: 1.5rem; margin-top: 0.8rem;">
                {feature_list_html}
            </ul>
            <h3>Statistics</h3>
            <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 0.8rem; margin: 1rem 0;">
                <div class="stat-box stat-functions">
                    <div class="stat-value">{len(self.parser.functions)}</div>
                    <div class="stat-label">Functions</div>
                </div>
                <div class="stat-box stat-structs">
                    <div class="stat-value">{len(self.parser.structs)}</div>
                    <div class="stat-label">Structures</div>
                </div>
                <div class="stat-box stat-defines">
                    <div class="stat-value">{len(self.parser.defines)}</div>
                    <div class="stat-label">Defines</div>
                </div>
                <div class="stat-box stat-enums">
                    <div class="stat-value">{len(self.parser.enums)}</div>
                    <div class="stat-label">Enumerations</div>
                </div>
            </div>
        </section>
        """
    
    def _generate_defines_section(self, file_name: str) -> str:
        defines = [d for d in self.parser.defines if d.name in self.parser.files_map[file_name].entities['defines']]
        if not defines: return ""
        html = '<h2>Constants & Defines</h2>'
        for define in sorted(defines, key=lambda x: x.name):
            html += f"""
            <div class="define-card" id="define-{define.name.replace('-', '_').replace('.', '_')}">
                <div class="function-name">{define.name}</div>
                <div class="signature">#define {define.name} {define.value}</div>
                {f'<p class="description">{define.doc.brief or define.doc.description}</p>' if define.doc.brief or define.doc.description else ''}
            </div>
            """
        return html

    def _generate_structs_section(self, file_name: str) -> str:
        structs = [s for s in self.parser.structs if s.name in self.parser.files_map[file_name].entities['structs']]
        if not structs: return ""
        html = '<h2>Data Structures</h2>'
        for struct in sorted(structs, key=lambda x: x.name):
            html += f"""
            <div class="struct-card" id="struct-{struct.name.replace('-', '_').replace('.', '_')}">
                <div class="struct-name">{struct.name}</div>
                {f'<p class="description">{struct.doc.brief}</p>' if struct.doc.brief else ''}
                {f'<p class="description">{struct.doc.description}</p>' if struct.doc.description else ''}
                """
            if struct.members:
                html += """
                <div class="members-title">Members:</div>
                <table class="member-table">
                    <tr><th>Type</th><th>Name</th><th>Description</th></tr>
                """
                for member_type, member_name, member_doc in struct.members:
                    html += f"""
                    <tr><td class="member-type">{member_type}</td><td class="member-name">{member_name}</td><td>{member_doc}</td></tr>
                    """
                html += "</table>"
            
            html += "</div>"
        return html
    
    def _generate_enums_section(self, file_name: str) -> str:
        enums = [e for e in self.parser.enums if e.name in self.parser.files_map[file_name].entities['enums']]
        if not enums: return ""
        html = '<h2>Enumerations</h2>'
        for enum in enums:
            html += f"""
            <div class="enum-card" id="enum-{enum.name.replace('-', '_').replace('.', '_')}">
                <div class="enum-name">{enum.name}</div>
                {f'<p class="description">{enum.doc.brief}</p>' if enum.doc.brief else ''}
                """
            if enum.values:
                html += '<div class="members-title">Values:</div><ul style="margin-left: 2rem;">'
                for value_name, value_doc in enum.values:
                    html += f'<li><span class="param-name">{value_name}</span>'
                    if value_doc: html += f' - {value_doc}'
                    html += '</li>'
                html += '</ul>'
            
            html += "</div>"
        return html

    def _generate_functions_section(self, file_name: str) -> str:
        functions = [f for f in self.parser.functions if f.name in self.parser.files_map[file_name].entities['functions']]
        if not functions: return ""
        html = '<h2>Functions</h2>'
        
        categories = {}
        for func in functions:
            if func.category not in categories: categories[func.category] = []
            categories[func.category].append(func)
        
        for category in sorted(categories.keys()):
            html += f'<div class="category-section"><h3 class="category-title">{category}</h3>'
            
            for func in sorted(categories[category], key=lambda x: x.name):
                html += f"""
                <div class="function-card" id="function-{func.name.replace('-', '_').replace('.', '_')}"> 
                    <div class="function-name">{func.name}</div>
                    {f'<div class="signature">{self._escape_html(func.signature)}</div>' if func.signature else ''}
                    {f'<p class="description">{func.doc.brief}</p>' if func.doc.brief else ''}
                    {f'<p class="description">{func.doc.description}</p>' if func.doc.description else ''}
                """
                
                if func.doc.params:
                    html += '<div class="params"><div class="param-title">Parameters:</div>'
                    for param_name, param_desc in func.doc.params:
                        html += f'<div class="param-item"><span class="param-name">{param_name}</span> - {param_desc}</div>'
                    html += '</div>'
                
                if func.doc.returns:
                    html += f'<div class="returns"><div class="returns-title">Returns:</div>{func.doc.returns}</div>'
                
                html += "</div>"
            
            html += "</div>"
        
        return html
    
    def _escape_html(self, text: str) -> str:
        """Escape HTML special characters."""
        return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')

def main():
    parser = argparse.ArgumentParser(
        description='Generate HTML documentation from C header files with Doxygen-style comments',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""
Examples:
  # Basic usage
  %(prog)s file.h
  
  # Set custom title and description from file
  %(prog)s -t "My Audio Library" --description-file README.txt src/
  
  # Include source code download/view options
  %(prog)s --include-source file.h
        """
    )
    
    parser.add_argument('inputs', nargs='+', help='Input header files or directories')
    parser.add_argument('-o', '--output', default='documentation.html',
                        help='Output HTML file (default: documentation.html)')
    
    # Global Title/Subtitle Overrides
    parser.add_argument('-t', '--title', help=f'Documentation title (default: "{DEFAULT_TITLE}")')
    parser.add_argument('-s', '--subtitle', help=f'Documentation subtitle (default: "{DEFAULT_SUBTITLE}")')
    
    # Overview Content Overrides
    parser.add_argument('--custom-overview-title', help=f'Custom title for the Overview section (default: "{DEFAULT_OVERVIEW_TITLE}")')
    parser.add_argument('--description-str', help='Overview description provided directly as a string.')
    parser.add_argument('--description-file', help='File containing the Overview description text.')
    parser.add_argument('--custom-features', nargs='+', help='List of key features for the overview section.')
    
    # Source Code Feature
    parser.add_argument('--include-source', action='store_true', dest='include_source_code',
                        help='Embed the raw source code of input files for download/viewing.')
    
    # Styling and Structure
    parser.add_argument('--css', help='Custom CSS file for styling')
    parser.add_argument('--theme', choices=['default', 'dark', 'minimal'], default='default',
                        help='Built-in theme to use (default: default/compact_dark)')
    parser.add_argument('--config', help='JSON configuration file')
    parser.add_argument('--no-toc', action='store_true', help='Disable table of contents')
    parser.add_argument('--no-overview', action='store_true', help='Disable overview section')
    parser.add_argument('--no-stats', action='store_true', help='Disable statistics in overview')
    parser.add_argument('--section-order', nargs='+',
                        default=['overview', 'defines', 'structs', 'enums', 'functions'],
                        help='Order of sections (default: overview defines structs enums functions)')
    parser.add_argument('-r', '--recursive', action='store_true',
                        help='Recursively search directories for .h files')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Verbose output')
    parser.add_argument('--config-only', metavar='FILE',
                        help='Generate a sample configuration file and exit (renamed from --generate-config)')
    
    args = parser.parse_args()
    
    # --- Step 1: Handle config-only ---
    if args.config_only:
        sample_config = {
            "title": DEFAULT_TITLE,
            "subtitle": DEFAULT_SUBTITLE,
            "output_file": "documentation.html",
            "css_file": None,
            "theme": "default",
            "include_toc": True,
            "include_overview": True,
            "include_stats": True,
            "include_source_code": False,
            "section_order": ["overview", "defines", "structs", "enums", "functions"],
            "custom_sections": {},
            "overview_title": DEFAULT_OVERVIEW_TITLE,
            "overview_description": DEFAULT_OVERVIEW_DESCRIPTION,
            "overview_features": DEFAULT_FEATURES
        }
        with open(args.config_only, 'w') as f:
            json.dump(sample_config, f, indent=2)
        print(f"Sample configuration written to {args.config_only}")
        return
    
    # --- Step 2: Load Config (File or Defaults) ---
    if args.config:
        config = Config.from_file(args.config)
    else:
        config = Config()
    
    # --- Step 3: Command Line Overrides (Precedence) ---

    # 3a. Global Titles
    if args.output != 'documentation.html': config.output_file = args.output
    if args.title: config.title = args.title
    if args.subtitle: config.subtitle = args.subtitle

    # 3b. Overview Content
    if args.custom_overview_title: config.overview_title = args.custom_overview_title
    if args.custom_features: config.overview_features = args.custom_features
    
    # Handle description string/file (String takes precedence over file)
    if args.description_str:
        config.overview_description = args.description_str
    elif args.description_file:
        try:
            with open(args.description_file, 'r', encoding='utf-8') as f:
                config.overview_description = f.read().strip()
        except Exception as e:
            print(f"Error reading description file {args.description_file}: {e}. Using default description.")

    # 3c. Styling and Structure Flags
    if args.css: config.css_file = args.css
    if args.theme != 'default': config.theme = args.theme
    if args.no_toc: config.include_toc = False
    if args.no_overview: config.include_overview = False
    if args.no_stats: config.include_stats = False
    if args.include_source_code: config.include_source_code = True
    if args.section_order != ['overview', 'defines', 'structs', 'enums', 'functions']:
        config.section_order = args.section_order

    # --- Step 4: Parsing and Generation ---
    doc_parser = DocParser()

    input_files = []
    for path in args.inputs:
        path_obj = Path(path)
        if path_obj.is_dir():
            if args.recursive:
                input_files.extend(path_obj.rglob('*.h'))
                input_files.extend(path_obj.rglob('*.c'))
            else:
                input_files.extend(path_obj.glob('*.h'))
                input_files.extend(path_obj.glob('*.c'))
        elif path_obj.is_file() and path_obj.suffix in ('.h', '.c'): # Added .c support
            input_files.append(path_obj)
        else:
            print(f"Warning: {path} not found or not a supported file type (.h, .c)")
    
    if not input_files:
        print("Error: No input files found")
        return 1
    
    for file in input_files:
        if args.verbose:
            print(f"Parsing {file}...")
        try:
            doc_parser.parse_file(str(file))
        except Exception as e:
            print(f"Error parsing {file}: {e}")
    
    generator = HTMLGenerator(doc_parser, config)
    generator.generate()
    
    print(f"\n{'='*60}")
    print(f"Documentation generated: {config.output_file}")
    print(f"{'='*60}")
    print(f"  Title: {config.title}")
    print(f"  Theme: {config.theme}")
    if config.css_file:
        print(f"  Custom CSS: {config.css_file}")
    print(f"\n  Content:")
    print(f"    - {len(doc_parser.functions)} functions")
    print(f"    - {len(doc_parser.structs)} structures")
    print(f"    - {len(doc_parser.defines)} defines")
    print(f"    - {len(doc_parser.enums)} enumerations")
    print(f"  Source Code Included: {config.include_source_code}")
    print(f"{'='*60}\n")
    
    return 0

if __name__ == "__main__":
    import sys
    sys.exit(main() or 0)
